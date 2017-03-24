#include "Pch.h"
#include "Base.h"
#include "AIController.h"
#include "Game.h"
#include "Quest_Mages.h"
#include "City.h"
#include "InsideLocation.h"
#include "GameGui.h"
#include "Spell.h"
#include "Team.h"

const float JUMP_BACK_MIN_RANGE = 4.f;
const float JUMP_BACK_TIMER = 0.2f;
const float JUMP_BACK_IGNORE_TIMER = 0.3f;
const float BLOCK_TIMER = 0.75f;
// je�li zablokuje to jaki jest czas zanim znowu spr�buje zablokowa�/odskoczy�
const float BLOCK_AFTER_BLOCK_TIMER = 0.2f;

cstring str_ai_state[AIController::State_Max] = {
	"Idle",
	"SeenEnemy",
	"Fighting",
	"SearchEnemy",
	"Dodge",
	"Escape",
	"Block",
	"Cast"
};

cstring str_ai_idle[AIController::Idle_Max] = {
	"None",
	"Animation",
	"Rot",
	"Move",
	"Look",
	"Chat",
	"Use",
	"WalkTo",
	"WalkUse",
	"TrainCombat",
	"TrainBow",
	"MoveRegion",
	"RunRegion",
	"Pray",
	"WalkNearUnit",
	"WalkUseEat",
	"MoveAway"
};

enum MOVE_TYPE
{
	DontMove, // nie rusza si�
	MoveAway, // odchodzi od punktu
	MovePoint, // idzie w stron� punktu
	KeepDistance, // stara si� zachowa� dystans 8-10 metr�w
	KeepDistanceCheck, // jak wy�ej ale sprawdza czy z nowej pozycji mo�e trafi� wroga
	RunAway // ucieka od punktu
};

enum RUN_TYPE
{
	Walk, // zawsze chodzenie
	WalkIfNear, // chodzenie je�li cel jest blisko
	Run, // zawsze bieg
	WalkIfNear2
};

enum LOOK_AT
{
	DontLook, // nie patrz si� nigdzie
	LookAtTarget, // patrz na cel
	LookAtWalk, // patrz na kierunek drogi
	LookAtPoint, // patrz na punkt
	LookAtAngle // patrz na k�t (look_pos.x)
};

inline float random_rot(float base_rot, float random_angle)
{
	if(rand2()%2 == 0)
		base_rot += random(random_angle);
	else
		base_rot -= random(random_angle);
	return clip(base_rot);
}

//=================================================================================================
void Game::UpdateAi(float dt)
{
	PROFILER_BLOCK("UpdateAI");
	static vector<Unit*> close_enemies;

	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		AIController& ai = **it;
		Unit& u = *ai.unit;
		if(u.to_remove)
			continue;

		if(!u.IsStanding())
		{
			UnitTryStandup(u, dt);
			continue;
		}

		LevelContext& ctx = GetContext(u);

		// aktualizuj czas
		u.prev_pos = u.pos;
		ai.timer -= dt;
		ai.last_scan -= dt;
		ai.next_attack -= dt;
		ai.ignore -= dt;
		ai.pf_timer -= dt;
		{
			const float maxm = (IS_SET(u.data->flags, F_COWARD) ? 5.f : 10.f);
			ai.morale += dt;
			if(ai.morale > maxm)
				ai.morale = maxm;
		}
		if(u.data->spells)
		{
			ai.cooldown[0] -= dt;
			ai.cooldown[1] -= dt;
			ai.cooldown[2] -= dt;
		}

		if(u.guard_target && u.dont_attack && !u.guard_target->dont_attack)
			u.dont_attack = false;

		if(u.frozen == 2)
		{
			u.animation = ANI_STAND;
			continue;
		}

		if(u.action == A_ANIMATION)
		{
			if(u.look_target)
			{
				float dir = lookat_angle(u.pos, u.look_target->pos);

				if(!equal(u.rot, dir))
				{
					const float rot_speed = 3.f*dt;
					const float rot_diff = angle_dif(u.rot, dir);
					if(rot_diff < rot_speed)
						u.rot = dir;
					else
					{
						const float d = sign(shortestArc(u.rot, dir)) * rot_speed;
						u.rot = clip(u.rot + d);
					}
				}
			}
			continue;
		}

		// animacja stania
		if(u.animation != ANI_PLAY && u.animation != ANI_IDLE)
			u.animation = ANI_STAND;

		// szukaj wrog�w
		Unit* enemy = nullptr;
		float best_dist = ALERT_RANGE.x+0.1f, dist;
		for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
		{
			if((*it2)->to_remove || !(*it2)->IsAlive() || (*it2)->invisible || !IsEnemy(u, **it2))
				continue;

			dist = distance(u.pos, (*it2)->pos);

			if(dist < best_dist && CanSee(u, **it2))
			{
				best_dist = dist;
				enemy = *it2;
			}
		}

		CheckAutoTalk(u, dt);

		// uciekanie
		if(enemy && ai.state != AIController::Escape && !IS_SET(u.data->flags, F_DONT_ESCAPE))
		{
			float morale = ai.morale,
				hpp = u.GetHpp();

			if(hpp < 0.1f)
				morale -= 3.f;
			else if(hpp < 0.25f)
				morale -= 2.f;
			else if(hpp < 0.5f)
				morale -= 1.f;

			if(morale < 0.f)
			{
				if(u.action == A_BLOCK)
				{
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = &u;
						c.id = AID_StopBlock;
						c.f[1] = 1.f;
					}
				}
				ai.state = AIController::Escape;
				ai.timer = random(2.5f, 5.f);
				ai.escape_room = nullptr;
				ai.ignore = 0.f;
			}
		}

		MOVE_TYPE move_type = DontMove;
		RUN_TYPE run_type = Run;
		LOOK_AT look_at = DontLook;
		VEC3 target_pos, look_pos;
		bool look_pt_valid = false;
		const void* path_obj_ignore = nullptr;
		const Unit* path_unit_ignore = nullptr;
		bool try_phase = false;

		// rzucanie czar�w nie do walki
		if(u.data->spells && u.data->spells->have_non_combat && u.action == A_NONE && ai.state != AIController::Escape && ai.state != AIController::Cast && u.busy == Unit::Busy_No)
		{
			for(int i=0; i<3; ++i)
			{
				if(u.data->spells && u.data->spells->spell[i] && IS_SET(u.data->spells->spell[i]->flags, Spell::NonCombat)
					&& u.level >= u.data->spells->level[i] && ai.cooldown[i] <= 0.f)
				{
					float spell_range = u.data->spells->spell[i]->range,
						best_prio = -999.f, dist;
					Unit* spell_target = nullptr;

					// je�li wrogowie s� w pobli�u to rzucaj tylko gdy nie trzeba chodzi�
					if(best_dist < 3.f)
						spell_range = 2.5f;

					if(IS_SET(u.data->spells->spell[i]->flags, Spell::Raise))
					{
						// o�ywianie trup�w
						for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
						{
							if(!(*it2)->to_remove && (*it2)->live_state == Unit::DEAD && !IsEnemy(u, **it2) && IS_SET((*it2)->data->flags, F_UNDEAD) &&
								(dist = distance(u.pos, (*it2)->pos)) < spell_range && CanSee(u, **it2))
							{
								float prio = (*it2)->hpmax - dist*10;
								if(prio > best_prio)
								{
									spell_target = *it2;
									best_prio = prio;
								}
							}
						}
					}
					else
					{
						// leczenie
						for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
						{
							if(!(*it2)->to_remove && !IsEnemy(u, **it2) && !IS_SET((*it2)->data->flags, F_UNDEAD) && (*it2)->hpmax - (*it2)->hp > 100.f && 
								(dist = distance(u.pos, (*it2)->pos)) < spell_range && CanSee(u, **it2))
							{
								float prio = (*it2)->hpmax - (*it2)->hp;
								if(*it2 == &u)
									prio *= 1.5f;
								prio -= dist*10;
								if(prio > best_prio)
								{
									spell_target = *it2;
									best_prio = prio;
								}
							}
						}
					}

					if(spell_target)
					{
						u.attack_id = i;
						ai.state = AIController::Cast;
						ai.cast_target = spell_target;
						break;
					}
				}
			}
		}

		// stany ai
		bool repeat = true;
		while(repeat)
		{
			repeat = false;

			switch(ai.state)
			{
			//===================================================================================================================
			// brak wrog�w w okolicy
			case AIController::Idle:
				{
					if(u.useable == nullptr)
					{
						if(ai.alert_target)
						{
							if(ai.alert_target->to_remove)
								ai.alert_target = nullptr;
							else
							{
								// kto� inny go powiadomi� okrzykiem o wrogu
								u.talking = false;
								u.ani->need_update = true;
								ai.in_combat = true;
								ai.target = ai.alert_target;
								ai.target_last_pos = ai.alert_target_pos;
								ai.alert_target = nullptr;
								ai.state = AIController::Fighting;
								ai.city_wander = false;
								ai.change_ai_mode = true;
								repeat = true;
								if(IS_SET(u.data->flags2, F2_YELL))
									AI_Shout(ctx, ai);
								break;
							}
						}

						if(enemy)
						{
							// zauwa�y� wroga, zacznij walk�
							u.talking = false;
							u.ani->need_update = true;
							ai.in_combat = true;
							ai.target = enemy;
							ai.target_last_pos = enemy->pos;
							ai.state = AIController::Fighting;
							ai.city_wander = false;
							ai.change_ai_mode = true;
							AI_Shout(ctx, ai);
							repeat = true;
							break;
						}
					}
					else if(enemy || ai.alert_target)
					{
						if(u.action != A_ANIMATION2 || u.animation_state == AS_ANIMATION2_MOVE_TO_ENDPOINT)
							break;
						if(ai.alert_target && ai.alert_target->to_remove)
							ai.alert_target = nullptr;
						else
						{
							// przerwij u�ywanie obiektu
							ai.idle_action = AIController::Idle_None;
							ai.timer = random(2.f,5.f);
							ai.city_wander = false;
							Unit_StopUsingUseable(ctx, u);
						}
					}

					bool chowaj_bron = true;
					if(u.action != A_NONE)
						chowaj_bron = false;
					else if(ai.idle_action == AIController::Idle_TrainCombat)
					{
						if(u.weapon_taken == W_ONE_HANDED)
							chowaj_bron = false;
					}
					else if(ai.idle_action == AIController::Idle_TrainBow)
					{
						if(u.weapon_taken == W_BOW)
							chowaj_bron = false;
					}
					if(chowaj_bron)
					{
						u.HideWeapon();
						if(!u.look_target)
							ai.CheckPotion(true);
					}

					// FIX na bug, przyczyny nie znane, zdarza si� czasem przegranym po walce na arenie
					if(u.weapon_state == WS_HIDING && u.action == A_NONE)
					{
						assert(0);
						u.weapon_state = WS_HIDDEN;
						ai.potion = -1;
					}

					if(ai.in_combat)
						ai.in_combat = false;

					bool wander = true;

					if(u.useable && u.useable->user != &u)
					{
						// naprawa b��du gdy si� on zdarzy a nie rozwi�zanie
						WARN(Format("Invalid useable user: %s is using %s but the user is %s.", u.data->id.c_str(), u.useable->GetBase()->id, u.useable->user ? u.useable->user->data->id.c_str() : "nullptr"));
						u.useable = nullptr;
#ifdef _DEBUG
						AddGameMsg("Invalid useable user!", 5.f);
#endif
					}
					if(u.action == A_BLOCK)
					{
						// mo�e naprawione ale p�ki co niech zostanie
						u.action = A_NONE;
						u.ani->frame_end_info2 = false;
						u.ani->Deactivate(1);
						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = &u;
							c.id = AID_StopBlock;
							c.f[1] = 1.f;
						}
						WARN(Format("Unit %s (%s) blocks in idle.", u.data->id.c_str(), u.GetName()));
#ifdef _DEBUG
						AddGameMsg("Unit blocks in idle!", 5.f);
#endif
					}
					if(u.look_target && !u.useable)
					{
						// patrzenie na posta� w czasie dialogu
						look_at = LookAtPoint;
						look_pos = u.look_target->pos;
						wander = false;
						u.timer = random(1.f,2.f);
					}
					else if(u.IsHero() && u.hero->mode == HeroData::Leave)
					{
						// bohater chce opu�ci� t� lokacj�
						if(u.useable)
						{
							if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
							{
								Unit_StopUsingUseable(ctx, u);
								ai.idle_action = AIController::Idle_None;
								ai.timer = random(1.f, 2.f);
								ai.city_wander = true;
							}
						}
						else if(ai.timer <= 0.f)
						{
							ai.idle_action = AIController::Idle_MoveRegion;
							ai.timer = random(30.f, 40.f);
							ai.idle_data.area.id = -1;
							ai.idle_data.area.pos.Build(u.in_building == -1 ?
								GetExitPos(u) :
								VEC3_x0y(city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint()));
						}
					}
					else
					{
						// chodzenie do karczmy
						if(ai.goto_inn && !(u.IsHero() && tournament_generated))
						{
							if(u.useable)
							{
								if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
								{
									Unit_StopUsingUseable(ctx, u);
									ai.idle_action = AIController::Idle_None;
									ai.timer = random(1.f, 2.f);
								}
							}
							else
							{
								int karczma_id;
								InsideBuilding* karczma = city_ctx->FindInn(karczma_id);
								if(u.in_building == -1)
								{
									// id� do karczmy
									if(ai.timer <= 0.f)
									{
										ai.idle_action = AIController::Idle_MoveRegion;
										ai.idle_data.area.pos.Build(VEC3_x0y(karczma->enter_area.Midpoint()));
										ai.idle_data.area.id = karczma_id;
										ai.timer = random(30.f, 40.f);
										ai.city_wander = true;
									}
								}
								else
								{
									if(u.in_building == karczma_id)
									{
										// jest w karczmie, id� do losowego punktu w karczmie
										ai.goto_inn = false;
										ai.timer = random(5.f,7.5f);
										ai.idle_action = AIController::Idle_Move;
										ai.idle_data.pos.Build(VEC3_x0y(rand2()%5 == 0 ? karczma->arena2.Midpoint() : karczma->arena1.Midpoint()));
									}
									else
									{
										// jest w budynku nie karczmie, wyjd� na zewn�trz
										ai.timer = random(15.f,30.f);
										ai.idle_action = AIController::Idle_MoveRegion;
										ai.idle_data.area.pos.Build(VEC3_x0y(city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint()));
										ai.idle_data.area.id = -1;
									}
								}
							}
						}
						else if(((u.IsFollower() && u.hero->mode == HeroData::Follow) || u.assist) && Team.leader->in_arena == -1 && u.busy != Unit::Busy_Tournament)
						{
							Unit* leader = Team.GetLeader();
							dist = distance(u.pos, leader->pos);
							if(dist >= (u.assist ? 4.f : 2.f))
							{
								// pod��aj za liderem
								if(u.useable)
								{
									if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
									{
										Unit_StopUsingUseable(ctx, u);
										ai.idle_action = AIController::Idle_None;
										ai.timer = random(1.f, 2.f);
										ai.city_wander = false;
									}
								}
								else if(leader->in_building != u.in_building)
								{
									// lider jest w innym obszarze
									ai.idle_action = AIController::Idle_RunRegion;
									ai.timer = random(15.f,30.f);
									
									if(u.in_building == -1)
									{
										// bohater nie jest w budynku, lider jest; id� do wej�cia
										ai.idle_data.area.id = leader->in_building;
										ai.idle_data.area.pos.Build(VEC3_x0y(city_ctx->inside_buildings[leader->in_building]->enter_area.Midpoint()));
									}
									else
									{
										// bohater jest w budynku, lider na zewn�trz lub w innym; opu�� budynek
										ai.idle_data.area.id = -1;
										ai.idle_data.area.pos.Build(VEC3_x0y(city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint()));
									}

									if(u.IsHero())
										try_phase = true;
								}
								else
								{
									// id� do lidera
									if(dist > 8.f)
										look_at = LookAtWalk;
									else
									{
										look_at = LookAtPoint;
										look_pos = leader->pos;
									}
									move_type = MovePoint;
									target_pos = leader->pos;
									run_type = WalkIfNear2;
									ai.idle_action = AIController::Idle_None;
									ai.city_wander = false;
									ai.timer = random(2.f,5.f);
									path_unit_ignore = leader;
									wander = false;
									if(u.IsHero())
										try_phase = true;
								}
							}
							else
							{
								// odsu� si� �eby nie blokowa�
								if(u.useable)
								{
									if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
									{
										Unit_StopUsingUseable(ctx, u);
										ai.idle_action = AIController::Idle_None;
										ai.timer = random(1.f, 2.f);
										ai.city_wander = false;
									}
								}
								else
								{
									for(Unit* unit : Team.members)
									{
										if(unit != &u && distance(unit->pos, u.pos) < 1.f)
										{
											look_at = LookAtPoint;
											look_pos = unit->pos;
											move_type = MoveAway;
											target_pos = unit->pos;
											run_type = Walk;
											ai.idle_action = AIController::Idle_None;
											ai.timer = random(2.f,5.f);
											ai.city_wander = false;
											wander = false;
											break;
										}
									}
								}
							}
						}
					}
					if(wander && (u.action == A_NONE || u.action == A_ANIMATION2 || (u.action == A_SHOOT && ai.idle_action == AIController::Idle_TrainBow) ||
						(u.action == A_ATTACK && ai.idle_action == AIController::Idle_TrainCombat)))
					{
						ai.loc_timer -= dt;
						if(ai.timer <= 0.f)
						{
							if(IS_SET(u.data->flags2, F2_XAR))
							{
								// szukaj o�tarza
								Obj* o = FindObject("bloody_altar");
								Object* obj = nullptr;
								for(vector<Object>::iterator it2 = local_ctx.objects->begin(), end2 = local_ctx.objects->end(); it2 != end2; ++it2)
								{
									if(it2->base == o)
									{
										obj = &*it2;
										break;
									}
								}

								if(obj)
								{
									// m�dl si� do o�tarza
									ai.idle_action = AIController::Idle_Pray;
									ai.idle_data.pos.Build(obj->pos);
									ai.timer = 120.f;
									u.animation = ANI_KNEELS;
								}
								else
								{
									// o�tarza nie ma, st�j w miejscu
									ai.idle_action = AIController::Idle_Animation;
									ai.idle_data.rot = u.rot;
									ai.timer = 120.f;
								}
							}
							else if(ai.idle_action == AIController::Idle_Look)
							{
								// sko�cz patrzy� na posta�, sp�jrz si� gdzie indziej
								ai.idle_action = AIController::Idle_Rot;
								if(IS_SET(u.data->flags, F_AI_GUARD) && angle_dif(u.rot, ai.start_rot) > PI/4)
									ai.idle_data.rot = ai.start_rot;
								else if(IS_SET(u.data->flags2, F2_LIMITED_ROT))
									ai.idle_data.rot = random_rot(ai.start_rot, PI/4);
								else
									ai.idle_data.rot = clip(lookat_angle(u.pos, ai.idle_data.pos)+random(-PI/2,PI/2));
								ai.timer = random(2.f,5.f);
							}
							else if(ai.idle_action == AIController::Idle_Chat)
							{
								u.talking = false;
								u.ani->need_update = true;
								ai.idle_action = AIController::Idle_None;
							}
							else if(ai.idle_action == AIController::Idle_Use)
							{
								if(u.useable->type == U_STOOL && u.in_building != -1)
								{
									int co;
									if(IsDrunkman(u))
										co = rand2()%3;
									else
										co = rand2()%2+1;
									switch(co)
									{
									case 0:
										u.ConsumeItem(FindItem(rand2()%3 == 0 ? "vodka" : "beer")->ToConsumable());
										ai.timer = random(10.f,15.f);
										break;
									case 1:
										u.ConsumeItem(FindItemList("normal_food").lis->Get()->ToConsumable());
										ai.timer = random(10.f,15.f);
										break;
									case 2:
										Unit_StopUsingUseable(ctx, u);
										ai.idle_action = AIController::Idle_None;
										ai.timer = random(2.5f, 5.f);
										break;
									}
								}
								else
								{
									Unit_StopUsingUseable(ctx, u);
									ai.idle_action = AIController::Idle_None;
									ai.timer = random(2.5f, 5.f);
								}
							}
							else if(ai.idle_action == AIController::Idle_WalkUseEat)
							{
								if(u.useable)
								{
									if(u.animation_state != 0)
									{
										int co;
										if(IsDrunkman(u))
											co = rand2()%2;
										else
											co = 1;
										switch(co)
										{
										case 0:
											u.ConsumeItem(FindItem(rand2()%3 == 0 ? "vodka" : "beer")->ToConsumable());
											ai.timer = random(10.f,15.f);
											break;
										case 1:
											u.ConsumeItem(FindItemList("normal_food").lis->Get()->ToConsumable());
											ai.timer = random(10.f,15.f);
											break;
										}
										ai.idle_action = AIController::Idle_Use;
									}
								}
								else
								{
									ai.idle_action = AIController::Idle_None;
									ai.timer = random(2.f,3.f);
								}
							}
							else if(CanWander(u))
							{
								if(u.IsHero())
								{
									if(u.in_building == -1)
									{
										// jest na zewn�trz
										int co = rand2() % (IS_SET(city_ctx->flags, City::HaveTrainingGrounds) ? 3 : 2);
										if(co == 0)
										{
											// id� losowo
											ai.loc_timer = ai.timer = random(30.f,120.f);
											ai.idle_action = AIController::Idle_Move;
											ai.idle_data.pos.Build(city_ctx->buildings[rand2()%city_ctx->buildings.size()].walk_pt
												+ random(VEC3(-1.f,0,-1), VEC3(1,0,1)));
										}
										else if(co == 1)
										{
											// id� do karczmy
											ai.loc_timer = ai.timer = random(75.f,150.f);
											ai.idle_action = AIController::Idle_MoveRegion;
											ai.idle_data.area.pos.Build(VEC3_x0y(city_ctx->FindInn(ai.idle_data.area.id)->enter_area.Midpoint()));
										}
										else if(co == 2)
										{
											// id� na pole treningowe
											ai.loc_timer = ai.timer = random(75.f,150.f);
											ai.idle_action = AIController::Idle_Move;
											ai.idle_data.pos.Build(city_ctx->FindBuilding(content::BG_TRAINING_GROUNDS)->walk_pt + random(VEC3(-1.f,0,-1), VEC3(1,0,1)));
										}
									}
									else
									{
										// opu�� budynek
										ai.loc_timer = ai.timer = random(15.f,30.f);
										ai.idle_action = AIController::Idle_MoveRegion;
										ai.idle_data.area.pos.Build(VEC3_x0y(city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint()));
										ai.idle_data.area.id = -1;
									}
									ai.city_wander = true;
								}
								else
								{
									// id� do losowego budynku
									ai.loc_timer = ai.timer = random(30.f,120.f);
									ai.idle_action = AIController::Idle_Move;
									ai.idle_data.pos.Build(city_ctx->buildings[rand2()%city_ctx->buildings.size()].walk_pt + random(VEC3(-1.f,0,-1), VEC3(1,0,1)));
									ai.city_wander = true;
								}
							}
							else if(u.guard_target && distance(u.pos, u.guard_target->pos) > 5.f)
							{
								ai.timer = random(2.f, 4.f);
								ai.idle_action = AIController::Idle_WalkNearUnit;
								ai.idle_data.unit = u.guard_target;
								ai.city_wander = false;
							}
							else if(IS_SET(u.data->flags3, F3_MINER) && rand2()%2 == 0)
							{
								// check if unit have required item
								const Item* req_item = g_base_usables[U_IRON_VAIN].item;
								if(req_item && !u.HaveItem(req_item) && u.slots[SLOT_WEAPON] != req_item)
									goto normal_idle_action;
								// find closest ore vain
								Useable* useable = nullptr;
								float range = 20.1f;
								for(vector<Useable*>::iterator it2 = ctx.useables->begin(), end2 = ctx.useables->end(); it2 != end2; ++it2)
								{
									Useable& use = **it2;
									if(!use.user && (use.type == U_IRON_VAIN || use.type == U_GOLD_VAIN))
									{
										float dist = distance(use.pos, u.pos);
										if(dist < range)
										{
											range = dist;
											useable = &use;
										}
									}
								}
								// start mining if there is anything to mine
								if(useable)
								{
									ai.idle_action = AIController::Idle_WalkUse;
									ai.idle_data.useable = useable;
									ai.timer = random(5.f,10.f);
								}
								else
									goto normal_idle_action;
							}
							else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE)
								&& quest_mages2->mages_state >= Quest_Mages2::State::OldMageJoined
								&& quest_mages2->mages_state < Quest_Mages2::State::MageCured
								&& rand2()%3 == 0)
							{
								// drink something
								u.ConsumeItem(FindItem(rand2()%3 == 0 ? "vodka" : "beer")->ToConsumable(), true);
								ai.idle_action = AIController::Idle_None;
								ai.timer = random(3.f,6.f);
							}
							else
							{
normal_idle_action:
#define I_ANIMACJA 0
#define I_OBROT 1
#define I_PATRZ 2
#define I_IDZ 3
#define I_GADAJ 4
#define I_UZYJ 5
#define I_CWICZ 6
#define I_CWICZ_LUK 7
#define I_JEDZ 8

								// losowa czynno��
								int co;
								if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
									co = rand2()%3;
								else if((u.busy == Unit::Busy_Tournament || (u.IsHero() && !u.IsFollowingTeamMember() && tournament_generated)) && tournament_master &&
									((dist = distance2d(u.pos, tournament_master->pos)) > 16.f || dist < 4.f))
								{
									co = -1;
									if(dist > 16.f)
									{
										ai.timer = random(5.f,10.f);
										ai.idle_action = AIController::Idle_WalkNearUnit;
										ai.idle_data.unit = tournament_master;
									}
									else
									{
										ai.timer = random(4.f,8.f);
										ai.idle_action = AIController::Idle_Move;
										ai.idle_data.pos.Build(tournament_master->pos + random(VEC3(-10,0,-10), VEC3(10,0,10)));
									}
								}
								else if(IS_SET(u.data->flags2, F2_SIT_ON_THRONE) && !u.IsFollower())
									co = I_UZYJ;
								else if(u.type == Unit::HUMAN)
								{
									if(!IS_SET(u.data->flags, F_AI_GUARD))
									{
										if(IS_SET(u.data->flags2, F2_AI_TRAIN) && rand2()%5 == 0)
										{
											static vector<Object*> do_cw;
											Obj* manekin = FindObject("melee_target"),
											   * tarcza = FindObject("bow_target");
											for(vector<Object>::iterator it2 = ctx.objects->begin(), end2 = ctx.objects->end(); it2 != end2; ++it2)
											{
												if((it2->base == manekin || (it2->base == tarcza && u.HaveBow())) && distance(it2->pos, u.pos) < 10.f)
													do_cw.push_back(&*it2);
											}
											if(!do_cw.empty())
											{
												Object& o = *do_cw[rand2()%do_cw.size()];
												if(o.base == manekin)
												{
													ai.timer = random(10.f,30.f);
													ai.idle_action = AIController::Idle_TrainCombat;
													ai.idle_data.pos.Build(o.pos);
												}
												else
												{
													ai.timer = random(15.f,35.f);
													ai.idle_action = AIController::Idle_TrainBow;
													ai.idle_data.obj.pos.Build(o.pos);
													ai.idle_data.obj.rot = o.rot.y;
													ai.idle_data.obj.ptr = &o;
												}
												do_cw.clear();
												co = -1;
											}
											else
											{
												if(IS_SET(u.data->flags3, F3_DONT_EAT))
													co = rand2()%6;
												else
												{
													co = rand2()%7;
													if(co == 6)
														co = I_JEDZ;
												}
											}
										}
										else
										{
											if(IS_SET(u.data->flags3, F3_DONT_EAT))
												co = rand2()%6;
											else
											{
												co = rand2()%7;
												if(co == 6)
													co = I_JEDZ;
											}
										}
									}
									else
									{
										if(IS_SET(u.data->flags3, F3_DONT_EAT))
											co = rand2()%5;
										else
										{
											co = rand2()%6;
											if(co == 5)
												co = I_JEDZ;
										}
									}
								}
								else
								{
									if(IS_SET(u.data->flags3, F3_DONT_EAT))
										co = rand2()%4;
									else
									{
										co = rand2()%5;
										if(co == 4)
											co = I_JEDZ;
									}
								}

								// nie gl�dzenie przez karczmarza/mistrza w czasie zawod�w
								if(co == I_GADAJ && IS_SET(u.data->flags3, F3_TALK_AT_COMPETITION) && (contest_state >= CONTEST_STARTING || tournament_state >= TOURNAMENT_STARTING))
									co = I_PATRZ;

								//								NORMAL STOI STRA�
								// 0 - animacja idle			 X      X    X
								// 1 - patrz na poblisk� posta�  X      X    X
								// 2 - losowy obr�t              X      X    X
								// 3 - gadaj z postaci�          X      X    X
								// 4 - losowy ruch               X      X
								// 5 - u�yj obiektu              X      X
								switch(co)
								{
								case -1:
									break;
								case I_UZYJ:
									// u�yj pobliskiego obiektu
									if(ctx.useables)
									{
										static vector<Useable*> uses;
										for(vector<Useable*>::iterator it2 = ctx.useables->begin(), end2 = ctx.useables->end(); it2 != end2; ++it2)
										{
											Useable& use = **it2;
											if(!use.user && (use.type != U_THRONE || IS_SET(u.data->flags2, F2_SIT_ON_THRONE)) && distance(use.pos, u.pos) < 10.f
												/*CanSee - niestety nie ma takiej funkcji wi�c trudno :p*/)
											{
												const Item* needed_item = g_base_usables[use.type].item;
												if(!needed_item || u.HaveItem(needed_item) || u.slots[SLOT_WEAPON] == needed_item)
													uses.push_back(*it2);
											}
										}
										if(!uses.empty())
										{
											ai.idle_action = AIController::Idle_WalkUse;
											ai.idle_data.useable = uses[rand2()%uses.size()];
											ai.timer = random(3.f,6.f);
											if(ai.idle_data.useable->type == U_STOOL && rand2()%3 == 0)
												ai.idle_action = AIController::Idle_WalkUseEat;
											uses.clear();
											break;
										}
									}
									// brak pobliskich obiekt�w, odtw�rz jak�� animacj�
								case I_ANIMACJA:
									// animacja idle
									{
										int id = rand2() % u.data->idles->size();
										ai.timer = random(2.f,5.f);
										ai.idle_action = AIController::Idle_Animation;
										u.ani->Play(u.data->idles->at(id).c_str(), PLAY_ONCE, 0);
										u.ani->frame_end_info = false;
										u.animation = ANI_IDLE;
										if(IsOnline())
										{
											NetChange& c = Add1(net_changes);
											c.type = NetChange::IDLE;
											c.unit = &u;
											c.id = id;
										}
									}
									break;
								case I_PATRZ:
									//patrz na poblisk� posta�
									if(u.busy == Unit::Busy_Tournament && rand2()%2 == 0 && tournament_master)
									{
										ai.timer = random(1.5f,2.5f);
										ai.idle_action = AIController::Idle_Look;
										ai.idle_data.unit = tournament_master;
										break;
									}
									else
									{
										close_enemies.clear();
										for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
										{
											if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->invisible || *it2 == &u)
												continue;

											if(distance(u.pos, (*it2)->pos) < 10.f && CanSee(u, **it2))
												close_enemies.push_back(*it2);
										}
										if(!close_enemies.empty())
										{
											ai.timer = random(1.5f,2.5f);
											ai.idle_action = AIController::Idle_Look;
											ai.idle_data.unit = close_enemies[rand2()%close_enemies.size()];
											break;
										}
									}
									// brak pobliskich jednostek, patrz si� losowo
								case I_OBROT:
									// losowy obr�t
									ai.timer = random(2.f,5.f);
									ai.idle_action = AIController::Idle_Rot;
									if(IS_SET(u.data->flags, F_AI_GUARD) && angle_dif(u.rot, ai.start_rot) > PI/4)
										ai.idle_data.rot = ai.start_rot;
									else if(IS_SET(u.data->flags2, F2_LIMITED_ROT))
										ai.idle_data.rot = random_rot(ai.start_rot, PI/4);
									else
										ai.idle_data.rot = random(MAX_ANGLE);
									break;
								case I_GADAJ:
									// podejd� do postaci i gadaj
									if(u.type == Unit::HUMAN)
									{
										const float d = ((IS_SET(u.data->flags, F_AI_GUARD) || IS_SET(u.data->flags, F_AI_STAY)) ? 1.5f : 10.f);
										close_enemies.clear();
										for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
										{
											if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->invisible || *it2 == &u)
												continue;

											if(distance(u.pos, (*it2)->pos) < d && CanSee(u, **it2))
												close_enemies.push_back(*it2);
										}
										if(!close_enemies.empty())
										{
											ai.timer = random(3.f,6.f);
											ai.idle_action = AIController::Idle_WalkTo;
											ai.idle_data.unit = close_enemies[rand2()%close_enemies.size()];
											ai.target_last_pos = ai.idle_data.unit->pos;
											break;
										}
									}
									// brak pobliskich jednostek, id� losowo
									if(IS_SET(u.data->flags, F_AI_GUARD))
										break;
								case I_IDZ:
									if(IS_SET(u.data->flags, F_AI_GUARD))
										break;
									// ruch w losowe miejsce
									ai.timer = random(3.f,6.f);
									ai.idle_action = AIController::Idle_Move;
									ai.city_wander = false;
									if(IS_SET(u.data->flags, F_AI_STAY))
									{
										if(distance(u.pos, ai.start_pos) > 2.f)
											ai.idle_data.pos.Build(ai.start_pos);
										else
											ai.idle_data.pos.Build(u.pos + random(VEC3(-2.f,0,-2.f), VEC3(2.f,0,2.f)));
									}
									else
										ai.idle_data.pos.Build(u.pos + random(VEC3(-5.f,0,-5.f), VEC3(5.f,0,5.f)));
									if(city_ctx && !city_ctx->IsInsideCity(ai.idle_data.pos))
									{
										ai.timer = random(2.f,4.f);
										ai.idle_action = AIController::Idle_None;
									}
									else if(!location->outside)
									{
										InsideLocation* inside = (InsideLocation*)location;
										if(!inside->GetLevelData().IsValidWalkPos(ai.idle_data.pos, u.GetUnitRadius()))
										{
											ai.timer = random(2.f, 4.f);
											ai.idle_action = AIController::Idle_None;
										}
									}
									break;
								case I_JEDZ:
									// jedzenie lub picie
									ai.timer = random(3.f,5.f);
									ai.idle_action = AIController::Idle_None;
									u.ConsumeItem(FindItemList(IS_SET(u.data->flags3, F3_ORC_FOOD) ? "orc_food" : "normal_food").lis->Get()->ToConsumable());
									break;
								default:
									assert(0);
									break;
								}
							}
						}
						else
						{
							switch(ai.idle_action)
							{
							case AIController::Idle_None:
							case AIController::Idle_Animation:
								break;
							case AIController::Idle_Rot:
								if(equal(u.rot, ai.idle_data.rot))
									ai.idle_action = AIController::Idle_None;
								else
								{
									look_at = LookAtAngle;
									look_pos.x = ai.idle_data.rot;
								}
								break;
							case AIController::Idle_Move:
								if(distance2d(u.pos, ai.idle_data.pos) < u.GetUnitRadius()*2)
								{
									if(ai.city_wander)
									{
										ai.city_wander = false;
										ai.timer = 0.f;
									}
									ai.idle_action = AIController::Idle_None;
								}
								else
								{
									move_type = MovePoint;
									target_pos = ai.idle_data.pos;
									look_at = LookAtWalk;
									run_type = Walk;
								}
								break;
							case AIController::Idle_Look:
								if(ai.idle_data.unit->to_remove || distance2d(u.pos, ai.idle_data.unit->pos) > 10.f || !CanSee(u, *ai.idle_data.unit))
								{
									// sko�cz si� patrzy�
									ai.idle_action = AIController::Idle_Rot;
									if(IS_SET(u.data->flags, F_AI_GUARD) && angle_dif(u.rot, ai.start_rot) > PI/4)
										ai.idle_data.rot = ai.start_rot;
									else if(IS_SET(u.data->flags2, F2_LIMITED_ROT))
										ai.idle_data.rot = random_rot(ai.start_rot, PI/4);
									else
										ai.idle_data.rot = random(MAX_ANGLE);
									ai.timer = random(2.f,5.f);
								}
								else
								{
									look_at = LookAtPoint;
									look_pos = ai.idle_data.unit->pos;
								}
								break;
							case AIController::Idle_WalkTo:
								if(ai.idle_data.unit->IsStanding() && !ai.idle_data.unit->to_remove)
								{
									if(CanSee(u, *ai.idle_data.unit))
									{
										if(distance2d(u.pos, ai.idle_data.unit->pos) < 1.5f)
										{
											// sko�cz podchodzi�, zacznij gada�
											ai.idle_action = AIController::Idle_Chat;
											ai.timer = random(2.f,3.f);

											cstring msg = GetRandomIdleText(u);

											int ani = 0;
											game_gui->AddSpeechBubble(&u, msg);
											if(rand2()%3 != 0)
											{
												ani = rand2()%2+1;
												u.ani->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
												u.animation = ANI_PLAY;
												u.action = A_ANIMATION;
											}

											if(ai.idle_data.unit->IsStanding() && ai.idle_data.unit->IsAI())
											{
												AIController& ai2 = *ai.idle_data.unit->ai;
												if(ai2.state == AIController::Idle && OR3_EQ(ai2.idle_action, AIController::Idle_None ,AIController::Idle_Rot, AIController::Idle_Look))
												{
													// odwzajemnij patrzenie si�
													ai2.idle_action = AIController::Idle_Chat;
													ai2.timer = ai.timer + random(1.f);
													ai2.idle_data.unit = &u;
												}
											}

											if(IsOnline())
											{
												NetChange& c = Add1(net_changes);
												c.type = NetChange::TALK;
												c.unit = &u;
												c.str = StringPool.Get();
												*c.str = msg;
												c.id = ani;
												c.ile = 0;
												net_talk.push_back(c.str);
											}
										}
										else
										{
											if(IS_SET(u.data->flags, F_AI_GUARD))
												ai.idle_action = AIController::Idle_None;
											else
											{
												ai.target_last_pos = ai.idle_data.unit->pos;
												move_type = MovePoint;
												run_type = Walk;
												look_at = LookAtWalk;
												target_pos = ai.target_last_pos;
												path_unit_ignore = ai.idle_data.unit;
											}
										}
									}
									else
									{
										if(distance2d(u.pos, ai.target_last_pos) < 1.5f)
											ai.idle_action = AIController::Idle_None;
										else
										{
											if(IS_SET(u.data->flags, F_AI_GUARD))
												ai.idle_action = AIController::Idle_None;
											else
											{
												move_type = MovePoint;
												run_type = Walk;
												look_at = LookAtWalk;
												target_pos = ai.target_last_pos;
												path_unit_ignore = ai.idle_data.unit;
											}
										}
									}
								}
								else
									ai.idle_action = AIController::Idle_None;
								break;
							case AIController::Idle_WalkNearUnit:
								if(ai.idle_data.unit->IsStanding() && !ai.idle_data.unit->to_remove)
								{
									if(distance2d(u.pos, ai.idle_data.unit->pos) < 4.f)
										ai.idle_action = AIController::Idle_None;
									else
									{
										ai.target_last_pos = ai.idle_data.unit->pos;
										move_type = MovePoint;
										run_type = Walk;
										look_at = LookAtWalk;
										target_pos = ai.target_last_pos;
										path_unit_ignore = ai.idle_data.unit;
									}
								}
								else
									ai.idle_action = AIController::Idle_None;
								break;
							case AIController::Idle_Chat:
								if(ai.idle_data.unit->to_remove)
									ai.idle_action = AIController::Idle_None;
								else
								{
									look_at = LookAtPoint;
									look_pos = ai.idle_data.unit->pos;
								}
								break;
							case AIController::Idle_WalkUse:
							case AIController::Idle_WalkUseEat:
								{
									Useable& use = *ai.idle_data.useable;
									if(use.user || u.frozen)
										ai.idle_action = AIController::Idle_None;
									else if(distance2d(u.pos, use.pos) < PICKUP_RANGE)
									{
										if(angle_dif(clip(u.rot+PI/2), clip(-angle2d(u.pos, ai.idle_data.useable->pos))) < PI/4)
										{
											BaseUsable& base = g_base_usables[use.type];
											const Item* needed_item = base.item;
											if(!needed_item || u.HaveItem(needed_item) || u.slots[SLOT_WEAPON] == needed_item)
											{
												u.action = A_ANIMATION2;
												u.animation = ANI_PLAY;
												bool czyta_papiery = false;
												if(use.type == U_CHAIR && IS_SET(u.data->flags, F_AI_CLERK))
												{
													czyta_papiery = true;
													u.ani->Play("czyta_papiery", PLAY_PRIO3, 0);
												}
												else
													u.ani->Play(base.anim, PLAY_PRIO1, 0);
												u.useable = &use;
												u.target_pos = u.pos;
												u.target_pos2 = use.pos;
												if(g_base_usables[use.type].limit_rot == 4)
													u.target_pos2 -= VEC3(sin(use.rot)*1.5f,0,cos(use.rot)*1.5f);
												u.timer = 0.f;
												u.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
												u.use_rot = lookat_angle(u.pos, u.useable->pos);
												u.used_item = needed_item;
												if(ai.idle_action == AIController::Idle_WalkUseEat)
													ai.timer = -1.f;
												else
												{
													ai.idle_action = AIController::Idle_Use;
													if(u.useable->type == U_STOOL && u.in_building != -1 && IsDrunkman(u))
														ai.timer = random(10.f,20.f);
													else if(u.useable->type == U_THRONE)
														ai.timer = 120.f;
													else if(u.useable->type == U_IRON_VAIN || u.useable->type == U_GOLD_VAIN)
														ai.timer = random(20.f,30.f);
													else
														ai.timer = random(5.f,10.f);
												}
												use.user = &u;
												if(IsOnline())
												{
													NetChange& c = Add1(net_changes);
													c.type = NetChange::USE_USEABLE;
													c.unit = &u;
													c.id = use.netid;
													c.ile = (czyta_papiery ? 2 : 1);
												}
											}
											else
												ai.idle_action = AIController::Idle_None;
										}
										else
										{
											// obr�� si� w strone u�ywalnego
											look_at = LookAtPoint;
											look_pos = use.pos;
										}
									}
									else
									{
										move_type = MovePoint;
										target_pos = use.pos;
										run_type = Walk;
										look_at = LookAtWalk;
										path_obj_ignore = &use;
										if(use.GetBase()->limit_rot == 4)
											target_pos -= VEC3(sin(use.rot), 0, cos(use.rot));
									}
								}
								break;
							case AIController::Idle_Use:
								break;
							case AIController::Idle_TrainCombat:
								if(distance2d(u.pos, ai.idle_data.pos) < u.GetUnitRadius()*2+0.25f)
								{
									if(ai.timer < 1.f)
									{
										u.HideWeapon();
										ai.in_combat = false;
									}
									else
									{
										u.TakeWeapon(W_ONE_HANDED);
										AI_DoAttack(ai, nullptr, false);
										ai.in_combat = true;
										u.hitted = true;
										look_at = LookAtPoint;
										look_pos = ai.idle_data.pos;
									}
								}
								else
								{
									move_type = MovePoint;
									target_pos = ai.idle_data.pos;
									look_at = LookAtWalk;
									run_type = Walk;
									// path_obj_ignore = ?
								}
								break;
							case AIController::Idle_TrainBow:
								{
									VEC3 pt = ai.idle_data.obj.pos;
									pt -= VEC3(sin(ai.idle_data.obj.rot)*5, 0, cos(ai.idle_data.obj.rot)*5);
									if(distance2d(u.pos, pt) < u.GetUnitRadius()*2)
									{
										if(ai.timer < 1.f)
										{
											if(u.action == A_NONE)
											{
												u.HideWeapon();
												ai.in_combat = false;
											}
										}
										else
										{
											u.TakeWeapon(W_BOW);
											float dir = lookat_angle(u.pos, ai.idle_data.obj.pos);
											if(angle_dif(u.rot, dir) < PI / 4 && u.action == A_NONE && u.weapon_taken == W_BOW && ai.next_attack <= 0.f && u.frozen == 0 &&
												CanShootAtLocation2(u, ai.idle_data.obj.ptr, ai.idle_data.obj.pos))
											{
												// strzelanie z �uku
												float speed = u.GetBowAttackSpeed();
												u.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
												u.ani->groups[1].speed = speed;
												u.action = A_SHOOT;
												u.animation_state = 1;
												u.hitted = false;
												u.bow_instance = GetBowInstance(u.GetBow().mesh);
												u.bow_instance->Play(&u.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND|PLAY_RESTORE, 0);
												u.bow_instance->groups[0].speed = speed;

												if(IsOnline())
												{
													NetChange& c = Add1(net_changes);
													c.type = NetChange::ATTACK;
													c.unit = &u;
													c.id = AID_Shoot;
													c.f[1] = speed;
												}
											}
											look_at = LookAtPoint;
											look_pos = ai.idle_data.obj.pos;
											ai.in_combat = true;
											if(u.action == A_SHOOT)
												ai.shoot_yspeed = (ai.idle_data.obj.pos.y-u.pos.y)/(distance(u.pos, ai.idle_data.obj.pos)/u.GetArrowSpeed());
										}
									}
									else
									{
										move_type = MovePoint;
										target_pos = pt;
										look_at = LookAtWalk;
										run_type = Walk;
									}
								}
								break;
							case AIController::Idle_MoveRegion:
							case AIController::Idle_RunRegion:
								if(distance2d(u.pos, ai.idle_data.area.pos) < u.GetUnitRadius()*2)
								{
									if(city_ctx && !IS_SET(city_ctx->flags, City::HaveExit) && ai.idle_data.area.id == -1 && u.in_building == -1)
									{
										// in exit area, go to border
										ai.idle_data.area.pos.Build(GetExitPos(u, true));
										ai.idle_data.area.id = -2;
									}
									else
									{
										if(ai.idle_data.area.id == -2)
											ai.idle_data.area.id = -1;
										ai.idle_action = AIController::Idle_None;
										UnitWarpData& warp = Add1(unit_warp_data);
										warp.unit = &u;
										warp.where = ai.idle_data.area.id;
										if(ai.idle_data.area.id == -1 || (u.IsFollower() && u.hero->mode == HeroData::Follow))
										{
											ai.loc_timer = -1.f;
											ai.timer = -1.f;
										}
										else
										{
											// sta� gdzie� w karczmie
											ai.idle_action = AIController::Idle_Move;
											ai.timer = random(5.f, 15.f);
											ai.loc_timer = random(60.f, 120.f);
											InsideBuilding* inside = city_ctx->inside_buildings[ai.idle_data.area.id];
											ai.idle_data.pos.Build((rand2()%5 == 0 ? inside->arena2 : inside->arena1).GetRandomPos3());
										}
									}
								}
								else
								{
									move_type = MovePoint;
									target_pos = ai.idle_data.area.pos;
									look_at = LookAtWalk;
									run_type = (ai.idle_action == AIController::Idle_MoveRegion ? Walk : WalkIfNear);
								}
								break;
							case AIController::Idle_Pray:
								if(distance2d(u.pos, u.ai->start_pos) > 1.f)
								{
									look_at = LookAtWalk;
									move_type = MovePoint;
									run_type = Walk;
									target_pos = ai.start_pos;
								}
								else
								{
									look_at = LookAtPoint;
									look_pos = ai.idle_data.pos;
									u.animation = ANI_KNEELS;
								}
								break;
							case AIController::Idle_MoveAway:
								look_at = LookAtPoint;
								look_pos = ai.idle_data.unit->pos;
								if(distance2d(u.pos, ai.idle_data.unit->pos) < 2.f)
								{
									move_type = MoveAway;
									target_pos = ai.idle_data.unit->pos;
									run_type = Walk;
								}
								break;
							default:
								assert(0);
								break;
							}
						}
					}
				}
				break;

			//===================================================================================================================
			// walka
			case AIController::Fighting:
				{
					ai.in_combat = true;

					if(!enemy)
					{
						// straci� wroga z widoku
						if(ai.target && (!ai.target->IsAlive() || ai.target->to_remove))
						{
							// cel umar�, koniec walki
							ai.state = AIController::Idle;
							ai.idle_action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = random(5.f,10.f);
							ai.timer = random(1.f,2.f);
						}
						else
						{
							// id� w ostatnio widziane miejsce
							ai.state = AIController::SeenEnemy;
							ai.timer = random(10.f, 15.f);
						}
						repeat = true;
						break;
					}

					// ignoruj alert target
					if(ai.alert_target)
						ai.alert_target = nullptr;

					// drink healing potion
					if(rand2()%4 == 0)
						ai.CheckPotion(true);

					// chowanie/wyjmowanie broni
					if(u.action == A_NONE)
					{
						// co wyj���? bro� do walki wr�cz czy �uk?
						WeaponType bron = W_NONE;

						if(u.PreferMelee() || IS_SET(u.data->flags, F_MAGE))
							bron = W_ONE_HANDED;
						else if(IS_SET(u.data->flags, F_ARCHER))
						{
							if(best_dist > 1.5f && u.HaveBow())
								bron = W_BOW;
							else if(u.HaveWeapon())
								bron = W_ONE_HANDED;
						}
						else
						{
							if(best_dist > (u.IsHoldingMeeleWeapon() ? 5.f : 2.5f))
							{
								if(u.HaveBow())
									bron = W_BOW;
								else if(u.HaveWeapon())
									bron = W_ONE_HANDED;
							}
							else
							{
								if(u.HaveWeapon())
									bron = W_ONE_HANDED;
								else if(u.HaveBow())
									bron = W_BOW;
							}
						}

						// ma co wyj��� ?
						if(bron != W_NONE && u.weapon_taken != bron)
							u.TakeWeapon(bron);
					}

					if(u.data->spells && u.action == A_NONE && u.frozen == 0)
					{
						bool break_action = false;

						// rzucanie czar�w
						for(int i=2; i>=0; --i)
						{
							if(u.data->spells->spell[i] && u.level >= u.data->spells->level[i] && ai.cooldown[i] <= 0.f)
							{
								Spell& s = *u.data->spells->spell[i];

								if(IS_SET(s.flags, Spell::NonCombat))
									continue;
								
								if(best_dist < s.range)
								{
									bool ok = true;

									if(IS_SET(s.flags, Spell::Drain))
									{
										// nie mo�na rzuca� wyssania na wrog�w bez krwii
										if(IS_SET(enemy->data->flags2, F2_BLOODLESS))
											ok = false;
									}

									if(!CanShootAtLocation(u, *enemy, enemy->pos))
										break;

									if(ok)
									{
										ai.cooldown[i] = random(u.data->spells->spell[i]->cooldown);
										u.action = A_CAST;
										u.attack_id = i;
										u.animation_state = 0;

										if(u.ani->ani->head.n_groups == 2)
										{
											u.ani->frame_end_info2 = false;
											u.ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 1);
										}
										else
										{
											u.ani->frame_end_info = false;
											u.animation = ANI_PLAY;
											u.ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 0);
										}

										if(IsOnline())
										{
											NetChange& c = Add1(net_changes);
											c.type = NetChange::CAST_SPELL;
											c.unit = &u;
											c.id = i;
										}

										break;
									}
								}
							}
						}

						if(break_action)
							break;
					}

					if(u.IsHoldingBow() || IS_SET(u.data->flags, F_MAGE))
					{
						// trzymanie dystansu przez �ucznik�w i mag�w
						move_type = KeepDistanceCheck;
						look_at = LookAtTarget;
						target_pos = enemy->pos;
					}
					if(u.action == A_CAST)
					{
						// strzela z czara
						look_pos = PredictTargetPos(u, *enemy, u.data->spells->spell[u.attack_id]->speed);
						look_at = LookAtPoint;
						u.target_pos = look_pos;
					}
					else if(u.action == A_SHOOT)
					{
						// strzela z �uku
						float arrow_speed = u.GetArrowSpeed();
						look_pos = PredictTargetPos(u, *enemy, arrow_speed);
						look_at = LookAtPoint;
						u.target_pos = look_pos;
						ai.shoot_yspeed = (enemy->pos.y - u.pos.y) / (distance(u.pos, enemy->pos) / arrow_speed);
					}

					if(u.IsHoldingBow())
					{
						if(u.action == A_NONE && ai.next_attack <= 0.f && u.frozen == 0)
						{
							// sprawd� czy mo�esz strzela� we wroga
							look_pos = PredictTargetPos(u, *enemy, u.GetArrowSpeed());

							if(CanShootAtLocation(u, *enemy, enemy->pos))
							{
								// strzelanie z �uku
								float speed = u.GetBowAttackSpeed();
								u.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
								u.ani->groups[1].speed = speed;
								u.action = A_SHOOT;
								u.animation_state = 1;
								u.hitted = false;
								u.bow_instance = GetBowInstance(u.GetBow().mesh);
								u.bow_instance->Play(&u.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND|PLAY_RESTORE, 0);
								u.bow_instance->groups[0].speed = speed;

								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::ATTACK;
									c.unit = &u;
									c.id = AID_Shoot;
									c.f[1] = speed;
								}
							}
							else
							{
								move_type = MovePoint;
								look_pos = enemy->pos;
								target_pos = enemy->pos;
							}
						}
					}
					else
					{
						if(!IS_SET(u.data->flags, F_MAGE) && u.action != A_CAST && u.action != A_SHOOT)
						{
							look_at = LookAtTarget;
							move_type = MovePoint;
							target_pos = enemy->pos;
							run_type = WalkIfNear;
							path_unit_ignore = enemy;
						}

						// zaatakuj
						if(u.action == A_NONE && ai.next_attack <= 0.f && u.frozen == 0)
						{
							if(best_dist <= u.GetAttackRange())
								AI_DoAttack(ai, enemy);
							else if(best_dist <= u.GetAttackRange()*3)
								AI_DoAttack(ai, enemy, true);
						}

						// blok/odskok
						if(ai.ignore <= 0.f)
						{
							// wybierz najbli�sza atakuj�c� posta�
							Unit* top = nullptr;
							float best_dist = JUMP_BACK_MIN_RANGE;

							for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
							{
								if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && IsEnemy(u, **it2) && (*it2)->action == A_ATTACK && !(*it2)->hitted &&
									(*it2)->animation_state < 2)
								{
									float dist = distance(u.pos, (*it2)->pos);
									if(dist < best_dist)
									{
										top = *it2;
										best_dist = dist;
									}
								}
							}

							if(top)
							{
								if(u.CanBlock() && u.action == A_NONE && rand2()%3 > 0)
								{
									// blokowanie
									float speed = u.GetBlockSpeed();
									ai.state = AIController::Block;
									ai.timer = BLOCK_TIMER;
									ai.ignore = 0.f;
									u.action = A_BLOCK;
									u.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, 1);
									u.ani->groups[1].blend_max = speed;
									u.animation_state = 0;

									if(IsOnline())
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::ATTACK;
										c.unit = &u;
										c.id = AID_Block;
										c.f[1] = speed;
									}
								}
								else if(rand2()%3 == 0)
								{
									// odskok
									ai.state = AIController::Dodge;
									ai.timer = JUMP_BACK_TIMER;

									if(rand2()%2 == 0)
										ai.CheckPotion();

									move_type = MoveAway;
								}
								else
									ai.ignore = JUMP_BACK_IGNORE_TIMER;
							}
						}
					}
				}
				break;

			//===================================================================================================================
			// id� do ostatnio znanego miejsca w kt�rym by� wr�g
			case AIController::SeenEnemy:
				{
					if(enemy)
					{
						// zauwa�y� wroga
						ai.target = enemy;
						ai.target_last_pos = enemy->pos;
						ai.state = AIController::Fighting;
						repeat = true;
						break;
					}

					if(ai.alert_target)
					{
						if(ai.alert_target->to_remove)
							ai.alert_target = nullptr;
						else
						{
							// kto� inny go zauwa�y�
							ai.target = ai.alert_target;
							ai.target_last_pos = ai.alert_target_pos;
							ai.state = AIController::Fighting;
							ai.alert_target = nullptr;
							repeat = true;
							break;
						}
					}

					move_type = MovePoint;
					target_pos = ai.target_last_pos;
					look_at = LookAtWalk;
					run_type = Run;

					if(distance(u.pos, ai.target_last_pos) < 1.f || ai.timer <= 0.f)
					{
						// doszed� do ostatniego widzianego punktu
						if(location->outside)
						{
							// jest na zewn�trz wi�c nie ma co robi�
							ai.state = AIController::Idle;
							ai.idle_action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = random(5.f,10.f);
							ai.timer = random(1.f,2.f);
						}
						else
						{
							InsideLocation* inside = (InsideLocation*)location;
							Room* room = inside->FindChaseRoom(u.pos);
							if(room)
							{
								// jest w podziemiach, id� do losowego pobliskiego pokoju
								ai.timer = u.IsFollower() ? random(1.f,2.f) : random(15.f, 30.f);
								ai.state = AIController::SearchEnemy;
								int gdzie = room->connected[rand2() % room->connected.size()];
								InsideLocationLevel& lvl = inside->GetLevelData();
								ai.escape_room = &lvl.rooms[gdzie];
								ai.target_last_pos = ai.escape_room->GetRandomPos(u.GetUnitRadius());
							}
							else
							{
								// w jaskini/labiryncie nie ma pokoi wi�c nie dzia�a
								ai.state = AIController::Idle;
								ai.idle_action = AIController::Idle_None;
								ai.in_combat = false;
								ai.change_ai_mode = true;
								ai.loc_timer = random(5.f,10.f);
								ai.timer = random(1.f,2.f);
							}
						}
					}
				}
				break;

			//===================================================================================================================
			// szukaj w okolicznych pokojach wrog�w
			case AIController::SearchEnemy:
				{
					if(enemy)
					{
						// zauwa�y� wroga
						ai.target = enemy;
						ai.target_last_pos = enemy->pos;
						ai.state = AIController::Fighting;
						AI_Shout(ctx, ai);
						repeat = true;
						break;
					}

					if(ai.alert_target)
					{
						if(ai.alert_target->to_remove)
							ai.alert_target = nullptr;
						else
						{
							// kto� inny zauwa�y� wroga
							ai.target = ai.alert_target;
							ai.target_last_pos = ai.alert_target_pos;
							ai.state = AIController::Fighting;
							ai.alert_target = nullptr;
							repeat = true;
							break;
						}
					}

					move_type = MovePoint;
					target_pos = ai.target_last_pos;
					look_at = LookAtWalk;
					run_type = Run;

					if(ai.timer <= 0.f || u.IsFollower()) // herosi w dru�ynie nie szukaj� wrog�w na w�asn� r�ke
					{
						// koniec po�cigu, nie znaleziono wroga
						ai.state = AIController::Idle;
						ai.idle_action = AIController::Idle_None;
						ai.in_combat = false;
						ai.change_ai_mode = true;
						ai.loc_timer = random(5.f,10.f);
						ai.timer = random(1.f,2.f);
					}
					else if(distance(u.pos, ai.target_last_pos) < 1.f)
					{
						// szukaj kolejnego pokoju
						InsideLocation* inside = (InsideLocation*)location;
						InsideLocationLevel& lvl = inside->GetLevelData();
						Room* room = lvl.GetNearestRoom(u.pos);
						int gdzie = room->connected[rand2() % room->connected.size()];
						ai.escape_room = &lvl.rooms[gdzie];
						ai.target_last_pos = ai.escape_room->GetRandomPos(u.GetUnitRadius());
					}
				}
				break;

			//===================================================================================================================
			// uciekaj
			case AIController::Escape:
				{
					// sprawd� czy mo�esz wypi� miksturk�
					ai.CheckPotion();

					if(enemy)
					{
						if(location->outside)
						{
							// zaawansowane uciekanie tylko w podziemiach, na zewn�trz uciekaj przed siebie
							if(distance2d(enemy->pos, u.pos) < 3.f)
							{
								look_at = LookAtTarget;
								move_type = MoveAway;
							}
							else
							{
								move_type = RunAway;
								look_at = LookAtWalk;
							}
							target_pos = enemy->pos;
						}
						else if(!ai.escape_room)
						{
							// jeszcze nie ma ustalonego gdzie ucieka�
							if(best_dist < 1.5f)
							{
								// lepiej nie odwraca� plecami do wroga w tym momencie
								move_type = MoveAway;
								target_pos = enemy->pos;
							}
							else if(ai.ignore <= 0.f)
							{
								// szukaj pokoju

								// zbierz pobliskich wrog�w
								close_enemies.clear();
								for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
								{
									if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->invisible || !IsEnemy(u, **it2))
										continue;

									if(distance(u.pos, (*it2)->pos) < ALERT_RANGE.x+0.1f)
										close_enemies.push_back(*it2);
								}

								// ustal w kt�rym pokoju s� wrogowie
								VEC3 mid = close_enemies.front()->pos;
								for(vector<Unit*>::iterator it2 = close_enemies.begin()+1, end2 = close_enemies.end(); it2 != end2; ++it2)
									mid += (*it2)->pos;
								mid /= (float)close_enemies.size();

								// kt�re to pomieszczenie?
								Room* room = ((InsideLocation*)location)->GetLevelData().FindEscapeRoom(u.pos, mid);

								if(room)
								{
									// uciekaj do pokoju
									ai.escape_room = room;
									move_type = MovePoint;
									look_at = LookAtWalk;
									target_pos = ai.escape_room->Center();
								}
								else
								{
									// nie ma drogi, po prostu uciekaj do ty�u
									move_type = MoveAway;
									target_pos = enemy->pos;
									ai.ignore = 0.5f;
								}
							}
							else
							{
								// czekaj przed nast�pnym szukaniem
								ai.ignore -= dt;
								move_type = MoveAway;
								target_pos = enemy->pos;
							}
						}
						else
						{
							if(best_dist < 1.f)
							{
								// kto� go zap�dzi� do rogu albo dogoni�
								ai.escape_room = nullptr;
								move_type = MoveAway;
								target_pos = enemy->pos;
							}
							else
							{
								// ju� ma pok�j do uciekania
								move_type = MovePoint;
								look_at = LookAtWalk;
								target_pos = ai.escape_room->Center();
							}
						}

						// koniec uciekania
						ai.timer -= dt;
						if(ai.timer <= 0.f)
						{
							ai.state = AIController::Fighting;
							ai.escape_room = nullptr;
							//repeat = true;
						}
					}
					else
					{
						// nie ma ju� wrog�w
						if(ai.escape_room)
						{
							// dobiegnij na miejsce
							if(ai.escape_room->IsInside(u.pos))
							{
								ai.escape_room = nullptr;
								ai.state = AIController::Fighting;
								//repeat = true;
							}
							else
							{
								move_type = MovePoint;
								look_at = LookAtWalk;
								target_pos = ai.escape_room->Center();
							}
						}
						else
						{
							ai.escape_room = nullptr;
							ai.state = AIController::Fighting;
							//repeat = true;
						}
					}

					// ignoruj alert target
					if(ai.alert_target)
						ai.alert_target = nullptr;
				}
				break;

				//===================================================================================================================
				// blokuj ciosy
				case AIController::Block:
					{
						// wybierz najbli�sza atakuj�c� posta�
						Unit* top = nullptr;
						float best_dist = 5.f;

						for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
						{
							if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && IsEnemy(u, **it2) && (*it2)->action == A_ATTACK && !(*it2)->hitted && (*it2)->animation_state < 2)
							{
								float dist = distance(u.pos, (*it2)->pos);
								if(dist < best_dist)
								{
									top = *it2;
									best_dist = dist;
								}
							}
						}

						if(top)
						{
							look_at = LookAtTarget;
							target_pos = top->pos;

							// uderz tarcz�
							if(best_dist <= u.GetAttackRange() && !u.ani->groups[1].IsBlending() && ai.ignore <= 0.f)
							{
								if(rand2()%2 == 0)
								{
									u.action = A_BASH;
									u.animation_state = 0;
									u.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, 1);
									u.ani->groups[1].speed = 2.f;
									u.ani->frame_end_info2 = false;
									u.hitted = false;

									if(IsOnline())
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::ATTACK;
										c.unit = &u;
										c.id = AID_Bash;
										c.f[1] = 2.f;
									}
								}
								else
									ai.ignore = 1.f;
							}
						}

						if(!top || ai.timer <= 0.f)
						{
							// brak cios�w do zablokowania lub czas min��
							u.action = A_NONE;
							u.ani->frame_end_info2 = false;
							u.ani->Deactivate(1);
							ai.state = AIController::Fighting;
							ai.ignore = BLOCK_AFTER_BLOCK_TIMER;
							repeat = true;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::ATTACK;
								c.unit = &u;
								c.id = AID_StopBlock;
								c.f[1] = 1.f;
							}
							break;
						}
					}
					break;

				//===================================================================================================================
				// odskakuj przed atakami
				case AIController::Dodge:
					{
						// wybierz najbli�sza atakuj�c� posta�
						Unit* top = nullptr;
						float best_dist = JUMP_BACK_MIN_RANGE;

						for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
						{
							if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && IsEnemy(u, **it2) /*&& (*it2)->action == A_ATTACK && !(*it2)->trafil && (*it2)->animation_state < 2*/)
							{
								float dist = distance(u.pos, (*it2)->pos);
								if(dist < best_dist)
								{
									top = *it2;
									best_dist = dist;
								}
							}
						}

						if(top)
						{
							look_at = LookAtTarget;
							move_type = MoveAway;
							target_pos = top->pos;

							// zaatakuj
							if(u.action == A_NONE && best_dist <= u.GetAttackRange() && ai.next_attack <= 0.f && u.frozen == 0)
								AI_DoAttack(ai, enemy);
							else if(u.action == A_CAST)
							{
								// strzela z czara
								look_pos = PredictTargetPos(u, *top, u.data->spells->spell[u.attack_id]->speed);
								look_at = LookAtPoint;
								u.target_pos = look_pos;
							}
							else if(u.action == A_SHOOT)
							{
								// strzela z �uku
								look_pos = PredictTargetPos(u, *top, u.GetArrowSpeed());
								look_at = LookAtPoint;
								u.target_pos = look_pos;
							}
						}

						// koniec odskoku, czas min�� lub brak wrog�w
						if((!top || ai.timer <= 0.f) && ai.potion == -1 && u.action != A_DRINK)
						{
							ai.state = AIController::Fighting;
							ai.ignore = BLOCK_AFTER_BLOCK_TIMER;
							repeat = true;
						}
					}
					break;

				//===================================================================================================================
				// rzucanie czaru na cel
				case AIController::Cast:
					if(ai.cast_target == &u)
					{
						move_type = DontMove;
						look_at = DontLook;

						if(u.action == A_NONE)
						{
							Spell& s = *u.data->spells->spell[u.attack_id];

							ai.cooldown[u.attack_id] = random(s.cooldown);
							u.action = A_CAST;
							u.animation_state = 0;
							u.target_pos = u.pos;

							if(u.ani->ani->head.n_groups == 2)
							{
								u.ani->frame_end_info2 = false;
								u.ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 1);
							}
							else
							{
								u.ani->frame_end_info = false;
								u.animation = ANI_PLAY;
								u.ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 0);
							}
						}
					}
					else
					{
						if(!ValidateTarget(u, ai.cast_target))
						{
							ai.state = AIController::Idle;
							ai.idle_action = AIController::Idle_None;
							ai.timer = random(1.f,2.f);
							ai.loc_timer = random(5.f,10.f);
							ai.timer = random(1.f,2.f);
							break;
						}

						move_type = MovePoint;
						run_type = WalkIfNear;
						look_at = LookAtPoint;
						target_pos = look_pos = ai.cast_target->pos;

						if(distance(u.pos, target_pos) <= 2.5f)
						{
							move_type = DontMove;

							if(u.action == A_NONE)
							{
								Spell& s = *u.data->spells->spell[u.attack_id];

								ai.cooldown[u.attack_id] = random(s.cooldown);
								u.action = A_CAST;
								u.animation_state = 0;
								u.target_pos = target_pos;

								if(u.ani->ani->head.n_groups == 2)
								{
									u.ani->frame_end_info2 = false;
									u.ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 1);
								}
								else
								{
									u.ani->frame_end_info = false;
									u.animation = ANI_PLAY;
									u.ani->Play("cast", PLAY_ONCE|PLAY_PRIO1, 0);
								}
							}
						}

						if(u.action == A_CAST)
							u.target_pos = target_pos;
					}
					break;
			}
		}

		if(u.IsFollower() && !try_phase)
		{
			u.hero->phase_timer = 0.f;
			u.hero->phase = false;
		}
		
		// animacja
		if(u.animation != ANI_PLAY && u.animation != ANI_KNEELS)
		{
			if(ai.in_combat || ((ai.idle_action == AIController::Idle_TrainBow || ai.idle_action == AIController::Idle_TrainCombat) && u.weapon_state == WS_TAKEN))
			{
				if(u.IsHoldingBow())
					u.animation = ANI_BATTLE_BOW;
				else
					u.animation = ANI_BATTLE;
			}
			else if(u.animation != ANI_IDLE)
				u.animation = ANI_STAND;
		}

		// ruch postaci
		if(move_type != DontMove && u.frozen == 0)
		{
			int move;

			if(move_type == KeepDistanceCheck)
			{
				if(u.action == A_TAKE_WEAPON || CanShootAtLocation(u, *enemy, look_pos))
				{
					if(best_dist < 8.f)
						move = -1;
					else if(best_dist > 10.f)
						move = 1;
					else
						move = 0;
				}
				else
					move = 1;
			}
			else if(move_type == KeepDistance)
			{
				if(best_dist < 8.f)
					move = -1;
				else if(best_dist > 10.f)
					move = 1;
				else
					move = 0;
			}
			else if(move_type == MoveAway)
				move = -1;
			else if(move_type == RunAway)
			{
				move = 1;
				target_pos = u.pos - target_pos;
				target_pos.y = 0.f;
				D3DXVec3Normalize(&target_pos);
				target_pos = u.pos + target_pos*20;
			}
			else
			{
				if(ai.state == AIController::Fighting)
				{
					if(best_dist < u.GetAttackRange()*0.9f)
						move = -1;
					else if(best_dist > u.GetAttackRange())
						move = 1;
					else
						move = 0;
				}
				else if(run_type == WalkIfNear2 && look_at != LookAtWalk)
				{
					if(angle_dif(u.rot, lookat_angle(u.pos, target_pos)) > PI/4)
					{
						move = 0;
						if(look_at == LookAtWalk)
						{
							look_at = LookAtPoint;
							look_pos = target_pos;
						}
					}
					else
						move = 1;
				}
				else
					move = 1;
			}

			if(move == -1)
			{
				// ruch do ty�u
				u.speed = u.GetWalkSpeed();
				u.prev_speed = (u.prev_speed + (u.speed - u.prev_speed)*dt*3);
				float speed = u.prev_speed * dt;
				const float kat = lookat_angle(u.pos, target_pos);

				u.prev_pos = u.pos;

				const VEC3 dir(sin(kat)*speed, 0, cos(kat)*speed);
				bool small;

				if(move_type == KeepDistanceCheck)
				{
					u.pos += dir;
					if(u.action != A_TAKE_WEAPON && !CanShootAtLocation(u, *enemy, look_pos))
						move = 0;
					u.pos = u.prev_pos;
				}

				if(move != 0 && CheckMove(u.pos, dir, u.GetUnitRadius(), &u, &small))
				{
					MoveUnit(u);

					if(!small && u.animation != ANI_PLAY)
						u.animation = ANI_WALK_TYL;
				}
			}
			else if(move == 1)
			{
				// ruch do punku (pathfinding)
				const INT2 my_tile(u.pos/2);
				const INT2 target_tile(target_pos/2);
				const INT2 my_local_tile(u.pos.x*4, u.pos.z*4);

				if(my_tile != target_tile)
				{
					// check is it time to use pathfinding
					if(ai.pf_state == AIController::PFS_NOT_USING ||
						distance(ai.pf_target_tile, target_tile) > 1 ||
						((ai.pf_state == AIController::PFS_WALKING || ai.pf_state == AIController::PFS_MANUAL_WALK) && target_tile != ai.pf_target_tile && ai.pf_timer <= 0.f))
					{
						ai.pf_timer = random(0.2f, 0.4f);
						if(FindPath(ctx, my_tile, target_tile, ai.pf_path, !IS_SET(u.data->flags, F_DONT_OPEN), ai.city_wander && city_ctx != nullptr))
						{
							// path found
							ai.pf_state = AIController::PFS_GLOBAL_DONE;
						}
						else
						{
							// path not found, use local search
							ai.pf_state = AIController::PFS_GLOBAL_NOT_USED;
						}
						ai.pf_target_tile = target_tile;
					}
				}
				else
				{
					// start tile and target tile is same, use local pathfinding
					ai.pf_state = AIController::PFS_GLOBAL_NOT_USED;
				}

				// otwieranie drzwi
				if(ctx.doors && !IS_SET(u.data->flags, F_DONT_OPEN))
				{
					for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
					{
						Door& door = **it;
						if(door.IsBlocking() && door.state == Door::Closed && door.locked == LOCK_NONE && distance(door.pos, u.pos) < 1.f)
						{
							// otw�rz magicznie drzwi :o
							if(!location->outside)
								minimap_opened_doors = true;
							door.state = Door::Opening;
							door.ani->Play(&door.ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
							door.ani->frame_end_info = false;

							if(nosound && rand2()%2 == 0)
							{
								// skrzypienie
								VEC3 pos = door.pos;
								pos.y += 1.5f;
								PlaySound3d(sDoor[rand2()%3], pos, 2.f, 5.f);
							}

							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::USE_DOOR;
								c.id = door.netid;
								c.ile = 0;
							}
						}
					}
				}

				// local pathfinding
				VEC3 move_target;
				if(ai.pf_state == AIController::PFS_GLOBAL_DONE ||
					ai.pf_state == AIController::PFS_GLOBAL_NOT_USED ||
					ai.pf_state == AIController::PFS_LOCAL_TRY_WALK)
				{
					INT2 local_tile;
					bool is_end_point;

					if(ai.pf_state == AIController::PFS_GLOBAL_DONE)
					{
						const INT2& pt = ai.pf_path.back();
						local_tile.x = (2*pt.x+1)*4;
						local_tile.y = (2*pt.y+1)*4;
						is_end_point = (ai.pf_path.size() == 1);
					}
					else if(ai.pf_state == AIController::PFS_GLOBAL_NOT_USED)
					{
						local_tile.x = int(target_pos.x*4);
						local_tile.y = int(target_pos.z*4);
						if(distance(my_local_tile, local_tile) > 32)
						{
							move_target = VEC3(target_pos.x, 0, target_pos.z);
							goto skip_localpf;
						}
						else
							is_end_point = true;
					}
					else
					{
						const INT2& pt = ai.pf_path[ai.pf_path.size()-ai.pf_local_try-1];
						local_tile.x = (2*pt.x+1)*4;
						local_tile.y = (2*pt.y+1)*4; 
						is_end_point = (ai.pf_local_try+1 == ai.pf_path.size());
					}
					
					int ret = FindLocalPath(ctx, ai.pf_local_path, my_local_tile, local_tile, &u, path_unit_ignore, path_obj_ignore, is_end_point);
					ai.pf_local_target_tile = local_tile;
					if(ret == 0)
					{
						// local path found
						assert(!ai.pf_local_path.empty());
						if(ai.pf_state == AIController::PFS_LOCAL_TRY_WALK)
						{
							for(int i=0; i<ai.pf_local_try; ++i)
								ai.pf_path.pop_back();
							ai.pf_state = AIController::PFS_WALKING;
						}
						else if(ai.pf_state == AIController::PFS_GLOBAL_DONE)
							ai.pf_state = AIController::PFS_WALKING;
						else
							ai.pf_state = AIController::PFS_WALKING_LOCAL;
						ai.pf_local_target_tile = local_tile;
						const INT2& pt = ai.pf_local_path.back();
						move_target = VEC3(0.25f*pt.x+0.125f, 0, 0.25f*pt.y+0.125f);
					}
					else if(ret == 4)
					{
						// walk point is blocked, at next frame try to skip it and use next path tile
						// if that fails mark tile as blocked and recalculate global path
						if(ai.pf_state == AIController::PFS_LOCAL_TRY_WALK)
						{
							++ai.pf_local_try;
							if(ai.pf_local_try == 4 || ai.pf_path.size() == ai.pf_local_try)
							{
								ai.pf_state = AIController::PFS_MANUAL_WALK;
								move_target = VEC3(0.25f*local_tile.x+0.125f, 0, 0.25f*local_tile.y+0.125f);
							}
							else
								move = 0;
						}
						else if(ai.pf_path.size() > 1u)
						{
							ai.pf_state = AIController::PFS_LOCAL_TRY_WALK;
							ai.pf_local_try = 1;
						}
						else
						{
							ai.pf_state = AIController::PFS_MANUAL_WALK;
							move_target = VEC3(0.25f*local_tile.x+0.125f, 0, 0.25f*local_tile.y+0.125f);
						}
					}
					else
					{
						ai.pf_state = AIController::PFS_MANUAL_WALK;
						move_target = VEC3(0.25f*local_tile.x+0.125f, 0, 0.25f*local_tile.y+0.125f);
					}
				}
				else if(ai.pf_state == AIController::PFS_MANUAL_WALK)
					move_target = target_pos;
				else if(ai.pf_state == AIController::PFS_WALKING ||
					ai.pf_state == AIController::PFS_WALKING_LOCAL)
				{
					const INT2& pt = ai.pf_local_path.back();
					move_target = VEC3(0.25f*pt.x+0.125f, 0, 0.25f*pt.y+0.125f);
				}

skip_localpf:

				if(move != 0)
				{
					// ruch postaci
					bool run;
					if(!u.CanRun())
						run = false;
					else if(u.run_attack)
						run = true;
					else
					{
						switch(run_type)
						{
						case Walk:
							run = false;
							break;
						case Run:
							run = true;
							break;
						case WalkIfNear:
							run = (distance(u.pos, target_pos) >= 1.5f);
							break;
						case WalkIfNear2:
							run = (distance(u.pos, target_pos) >= 3.f);
							break;
						default:
							assert(0);
							run = true;
							break;
						}
					}

					u.speed = run ? u.GetRunSpeed() : u.GetWalkSpeed();
					u.prev_speed = (u.prev_speed + (u.speed - u.prev_speed)*dt*3);
					float speed = u.prev_speed * dt;
					const float kat = lookat_angle(u.pos, move_target)+PI;

					u.prev_pos = u.pos;

					const VEC3 dir(sin(kat)*speed, 0, cos(kat)*speed);
					bool small;

					if(move_type == KeepDistanceCheck)
					{
						u.pos += dir;
						if(!CanShootAtLocation(u, *enemy, look_pos))
							move = 0;
						u.pos = u.prev_pos;
					}

					if(move != 0)
					{
						int move_state = 0;
						//VEC3 prev = u.pos;
						if(CheckMove(u.pos, dir, u.GetUnitRadius(), &u, &small))
							move_state = 1;
						else if(try_phase && u.hero->phase /*&& CheckMovePhase(u.pos, dir, u.GetUnitRadius(), &u, &small))*/)
						{
							move_state = 2;
							u.pos += dir;
							small = false;
						}

						if(move_state)
						{
							u.changed = true;
							
							MoveUnit(u);

							if(look_at == LookAtWalk)
							{
								look_pos = u.pos + dir;
								look_pt_valid = true;
							}

							if(ai.pf_state == AIController::PFS_WALKING)
							{
								const INT2 new_tile(u.pos/2);
								if(new_tile != my_tile)
								{
									if(new_tile == ai.pf_path.back())
									{
										ai.pf_path.pop_back();
										if(ai.pf_path.empty())
											ai.pf_state = AIController::PFS_NOT_USING;
									}
									else
										ai.pf_state = AIController::PFS_NOT_USING;
								}
							}

							if(ai.pf_state == AIController::PFS_WALKING_LOCAL || ai.pf_state == AIController::PFS_WALKING)
							{
								const INT2 new_tile(u.pos.x*4, u.pos.z*4);
								if(new_tile != my_local_tile)
								{
									if(new_tile == ai.pf_local_path.back())
									{
										ai.pf_local_path.pop_back();
										if(ai.pf_local_path.empty())
											ai.pf_state = AIController::PFS_NOT_USING;
									}
									else
										ai.pf_state = AIController::PFS_NOT_USING;
								}
							}

							if(u.animation != ANI_PLAY && !small)
							{
								if(run)
									u.animation = ANI_RUN;
								else
									u.animation = ANI_WALK;
							}

							if(try_phase)
							{
								if(move_state == 1)
								{
									u.hero->phase = false;
									u.hero->phase_timer = 0.f;
								}
							}
						}
						else if(try_phase)
						{
							u.hero->phase_timer += dt;
							if(u.hero->phase_timer >= 2.f)
								u.hero->phase = true;
						}
					}
				}
			}
		}

		// obr�t
		VEC3 look_pos2;
		if(look_at == LookAtTarget || look_at == LookAtPoint || (look_at == LookAtWalk && look_pt_valid))
		{
			VEC3 look_pt;
			if(look_at == LookAtTarget)
				look_pt = target_pos;
			else
				look_pt = look_pos;

			// patrz na cel
			float dir = lookat_angle(u.pos, look_pt),
				dif = angle_dif(u.rot, dir);

			if(not_zero(dif))
			{
				const float rot_speed = u.GetRotationSpeed() * dt;
				const float arc = shortestArc(u.rot, dir);

				if(u.animation == ANI_STAND || u.animation == ANI_BATTLE || u.animation == ANI_BATTLE_BOW)
				{
					if(arc > 0.f)
						u.animation = ANI_RIGHT;
					else
						u.animation = ANI_LEFT;
				}

				if(dif <= rot_speed)
					u.rot = dir;
				else
					u.rot = clip(u.rot + sign(arc) * rot_speed);

				u.changed = true;
			}
		}
		else if(look_at == LookAtAngle)
		{
			float dif = angle_dif(u.rot, look_pos.x);
			if(not_zero(dif))
			{
				const float rot_speed = u.GetRotationSpeed() * dt;
				const float arc = shortestArc(u.rot, look_pos.x);

				if(u.animation == ANI_STAND || u.animation == ANI_BATTLE || u.animation == ANI_BATTLE_BOW)
				{
					if(arc > 0.f)
						u.animation = ANI_RIGHT;
					else
						u.animation = ANI_LEFT;
				}

				if(dif <= rot_speed)
					u.rot = look_pos.x;
				else
					u.rot = clip(u.rot + sign(arc) * rot_speed);

				u.changed = true;
			}
		}
	}
}

//=================================================================================================
void Game::AI_Shout(LevelContext& ctx, AIController& ai)
{
	Unit& unit = *ai.unit;

	if(!unit.data->sounds->sound[SOUND_SEE_ENEMY])
		return;

	if(sound_volume)
		PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_SEE_ENEMY], 3.f, 20.f);

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::SHOUT;
		c.unit = &unit;
	}

	// alarm near allies
	for(Unit* u : *ctx.units)
	{
		if(u->to_remove || &unit == u || !u->IsStanding() || u->IsPlayer() || !IsFriend(unit, *u) || u->ai->state == AIController::Fighting
			|| u->ai->alert_target || u->dont_attack)
			continue;

		if(distance(unit.pos, u->pos) <= 20.f && CanSee(unit, *u))
		{
			u->ai->alert_target = ai.target;
			u->ai->alert_target_pos = ai.target_last_pos;
		}
	}
}

//=================================================================================================
// je�li target jest nullptr to atak nic nie zadaje (trenuje walk� na manekinie)
void Game::AI_DoAttack(AIController& ai, Unit* target, bool w_biegu)
{
	Unit& u = *ai.unit;

	if(u.action == A_NONE && (u.ani->ani->head.n_groups == 1 || u.weapon_state == WS_TAKEN) && ai.next_attack <= 0.f)
	{
		if(sound_volume && u.data->sounds->sound[SOUND_ATTACK] && rand2()%4 == 0)
			PlayAttachedSound(u, u.data->sounds->sound[SOUND_ATTACK], 1.f, 10.f);
		u.action = A_ATTACK;
		u.attack_id = u.GetRandomAttack();

		bool do_power_attack;
		if(!IS_SET(u.data->flags, F_NO_POWER_ATTACK))
		{
			if(target && target->action == A_BLOCK)
				do_power_attack = (rand2()%2 == 0);
			else
				do_power_attack = (rand2()%5 == 0);
		}
		else
			do_power_attack = false;
		u.attack_power = 1.f;

		if(w_biegu)
		{
			u.attack_power = 1.5f;
			u.run_attack = true;
			do_power_attack = false;
		}

		float speed (do_power_attack ? ai.unit->GetPowerAttackSpeed() : ai.unit->GetAttackSpeed());

		if(u.ani->ani->head.n_groups > 1)
		{
			u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
			u.ani->groups[1].speed = speed;
		}
		else
		{
			u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 0);
			u.ani->groups[0].speed = speed;
			u.animation = ANI_PLAY;
		}
		u.animation_state = (do_power_attack ? 0 : 1);
		u.hitted = !target;

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::ATTACK;
			c.unit = &u;
			c.id = (do_power_attack ? AID_PowerAttack : (w_biegu ? AID_RunningAttack : AID_Attack));
			c.f[1] = speed;
		}
	}
}

//=================================================================================================
void Game::AI_HitReaction(Unit& unit, const VEC3& pos)
{
	AIController& ai = *unit.ai;
	switch(ai.state)
	{
	case AIController::Idle:
	case AIController::SearchEnemy:
		ai.target = nullptr;
		ai.target_last_pos = pos;
		ai.state = AIController::SeenEnemy;
		if(ai.state == AIController::Idle)
			ai.change_ai_mode = true;
		ai.timer = random(10.f, 15.f);
		ai.city_wander = false;
		if(!unit.data->sounds->sound[SOUND_SEE_ENEMY])
			return;

		if(sound_volume)
			PlayAttachedSound(unit, unit.data->sounds->sound[SOUND_SEE_ENEMY], 3.f, 20.f);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SHOUT;
			c.unit = &unit;
		}

		// alarm near allies
		for(Unit* u : *local_ctx.units)
		{
			if(u->to_remove || &unit == u || !u->IsStanding() || u->IsPlayer() || !IsFriend(unit, *u) || u->dont_attack)
				continue;

			if((u->ai->state == AIController::Idle || u->ai->state == AIController::SearchEnemy) && distance(unit.pos, u->pos) <= 20.f && CanSee(unit, *u))
			{
				AIController* ai2 = u->ai;
				ai2->target_last_pos = pos;
				ai2->state = AIController::SeenEnemy;
				ai2->timer = random(5.f, 10.f);
			}
		}
		break;
	}
}

//=================================================================================================
void Game::CheckAutoTalk(Unit& unit, float dt)
{
	if(unit.auto_talk == AutoTalkMode::No || (unit.action != A_NONE && unit.action != A_ANIMATION2))
	{
		if(unit.auto_talk == AutoTalkMode::Wait)
		{
			unit.auto_talk = AutoTalkMode::Yes;
			unit.auto_talk_timer = Unit::AUTO_TALK_WAIT;
		}
		return;
	}

	// timer to not check everything every frame
	unit.auto_talk_timer -= dt;
	if(unit.auto_talk_timer > 0.f)
		return;
	unit.auto_talk_timer = Unit::AUTO_TALK_WAIT;

	// find near players
	struct NearUnit
	{
		Unit* unit;
		float dist;
	};
	static vector<NearUnit> near_units;

	const bool leader_mode = (unit.auto_talk == AutoTalkMode::Leader);

	for(Unit* u : Team.members)
	{
		if(u->IsPlayer())
		{
			// if not leader (in leader mode) or busy - don't check this unit
			if((leader_mode && u != Team.leader)
				|| (u->player->dialog_ctx->dialog_mode || u->busy != Unit::Busy_No
				|| !u->IsStanding() || u->player->action != PlayerController::Action_None))
				continue;
			float dist = distance(unit.pos, u->pos);
			if(dist <= 8.f || leader_mode)
				near_units.push_back({ u, dist });
		}
	}
	
	// if no nearby available players found, return
	if(near_units.empty())
	{
		if(unit.auto_talk == AutoTalkMode::Wait)
		{
			unit.auto_talk = AutoTalkMode::Yes;
			unit.auto_talk_timer = Unit::AUTO_TALK_WAIT;
		}
		return;
	}

	// sort by distance
	std::sort(near_units.begin(), near_units.end(), [](const NearUnit& nu1, const NearUnit& nu2) { return nu1.dist < nu2.dist; });

	// get near player that don't have enemies nearby
	PlayerController* talk_player = nullptr;
	for(auto& near_unit : near_units)
	{
		Unit& talk_target = *near_unit.unit;
		if(CanSee(unit, talk_target))
		{
			bool ok = true;
			for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& check_unit = **it2;
				if(&talk_target == &check_unit || &unit == &check_unit)
					continue;

				if(check_unit.IsAlive() && IsEnemy(talk_target, check_unit) && check_unit.IsAI() && !check_unit.dont_attack
					&& distance2d(talk_target.pos, check_unit.pos) < ALERT_RANGE.x && CanSee(check_unit, talk_target))
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				talk_player = talk_target.player;
				break;
			}
		}
	}

	// start dialog or wait
	if(talk_player)
	{
		if(unit.auto_talk == AutoTalkMode::Yes)
		{
			unit.auto_talk = AutoTalkMode::Wait;
			unit.auto_talk_timer = 1.f;
		}
		else
		{
			unit.auto_talk = AutoTalkMode::No;
			StartDialog2(talk_player, &unit, unit.auto_talk_dialog);
		}
	}
	else if(unit.auto_talk == AutoTalkMode::Wait)
	{
		unit.auto_talk = AutoTalkMode::Yes;
		unit.auto_talk_timer = Unit::AUTO_TALK_WAIT;
	}

	near_units.clear();
}
