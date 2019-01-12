#include "Pch.h"
#include "GameCore.h"
#include "AIController.h"
#include "Game.h"
#include "Quest_Mages.h"
#include "City.h"
#include "InsideLocation.h"
#include "GameGui.h"
#include "Spell.h"
#include "Team.h"
#include "SoundManager.h"
#include "Profiler.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Tournament.h"
#include "Pathfinding.h"
#include "GameMessages.h"
#include "GlobalGui.h"

const float JUMP_BACK_MIN_RANGE = 4.f;
const float JUMP_BACK_TIMER = 0.2f;
const float JUMP_BACK_IGNORE_TIMER = 0.3f;
const float BLOCK_TIMER = 0.75f;
const float BLOCK_AFTER_BLOCK_TIMER = 0.2f;

cstring str_ai_state[AIController::State_Max] = {
	"Idle",
	"SeenEnemy",
	"Fighting",
	"SearchEnemy",
	"Dodge",
	"Escape",
	"Block",
	"Cast",
	"Wait"
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

enum AI_ACTION
{
	AI_NOT_SET = -2,
	AI_NONE = -1,
	AI_ANIMATION = 0,
	AI_ROTATE,
	AI_LOOK,
	AI_MOVE,
	AI_TALK,
	AI_USE,
	AI_EAT
};

inline float random_rot(float base_rot, float random_angle)
{
	if(Rand() % 2 == 0)
		base_rot += Random(random_angle);
	else
		base_rot -= Random(random_angle);
	return Clip(base_rot);
}

//=================================================================================================
void Game::UpdateAi(float dt)
{
	PROFILER_BLOCK("UpdateAI");
	static vector<Unit*> close_enemies;

	auto stool = BaseUsable::Get("stool"),
		chair = BaseUsable::Get("chair"),
		throne = BaseUsable::Get("throne"),
		iron_vein = BaseUsable::Get("iron_vein"),
		gold_vein = BaseUsable::Get("gold_vein");
	Quest_Tournament* tournament = QM.quest_tournament;

	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		AIController& ai = **it;
		Unit& u = *ai.unit;
		if(u.to_remove)
			continue;

		if(!u.IsStanding())
		{
			u.TryStandup(dt);
			continue;
		}
		else if(u.HaveEffect(EffectId::Stun) || u.action == A_STAND_UP)
			continue;

		LevelContext& ctx = L.GetContext(u);

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

		if(u.frozen >= FROZEN::YES)
		{
			if(u.frozen == FROZEN::YES)
				u.animation = ANI_STAND;
			continue;
		}

		if(u.look_target && u.look_target->to_remove)
			u.look_target = nullptr;

		if(u.action == A_ANIMATION)
		{
			if(u.look_target)
			{
				float dir = Vec3::LookAtAngle(u.pos, u.look_target->pos);

				if(!Equal(u.rot, dir))
				{
					const float rot_speed = 3.f*dt;
					const float rot_diff = AngleDiff(u.rot, dir);
					if(rot_diff < rot_speed)
						u.rot = dir;
					else
					{
						const float d = Sign(ShortestArc(u.rot, dir)) * rot_speed;
						u.rot = Clip(u.rot + d);
					}
					u.changed = true;
				}
			}
			continue;
		}

		// animacja stania
		if(u.animation != ANI_PLAY && u.animation != ANI_IDLE)
			u.animation = ANI_STAND;

		// szukaj wrog�w
		Unit* enemy = nullptr;
		float best_dist = ALERT_RANGE.x + 0.1f, dist;
		for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
		{
			if((*it2)->to_remove || !(*it2)->IsAlive() || (*it2)->invisible || !u.IsEnemy(**it2))
				continue;

			dist = Vec3::Distance(u.pos, (*it2)->pos);

			if(dist < best_dist && L.CanSee(u, **it2))
			{
				best_dist = dist;
				enemy = *it2;
			}
		}

		CheckAutoTalk(u, dt);

		// ai escape
		if(enemy && ai.state != AIController::Escape && !IS_SET(u.data->flags, F_DONT_ESCAPE) && ai.GetMorale() < 0.f)
		{
			if(u.action == A_BLOCK)
			{
				u.action = A_NONE;
				u.mesh_inst->frame_end_info2 = false;
				u.mesh_inst->Deactivate(1);
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::ATTACK;
					c.unit = &u;
					c.id = AID_StopBlock;
					c.f[1] = 1.f;
				}
			}
			ai.state = AIController::Escape;
			ai.timer = Random(2.5f, 5.f);
			ai.escape_room = nullptr;
			ai.ignore = 0.f;
			ai.target_last_pos = enemy->pos;
		}

		MOVE_TYPE move_type = DontMove;
		RUN_TYPE run_type = Run;
		LOOK_AT look_at = DontLook;
		Vec3 target_pos, look_pos;
		bool look_pt_valid = false;
		const void* path_obj_ignore = nullptr;
		const Unit* path_unit_ignore = nullptr;
		bool try_phase = false;

		// rzucanie czar�w nie do walki
		if(u.data->spells && u.data->spells->have_non_combat && u.action == A_NONE && ai.state != AIController::Escape && ai.state != AIController::Cast
			&& u.busy == Unit::Busy_No)
		{
			for(int i = 0; i < 3; ++i)
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
							if(!(*it2)->to_remove && (*it2)->live_state == Unit::DEAD && !u.IsEnemy(**it2) && IS_SET((*it2)->data->flags, F_UNDEAD)
								&& (dist = Vec3::Distance(u.pos, (*it2)->pos)) < spell_range && L.CanSee(u, **it2))
							{
								float prio = (*it2)->hpmax - dist * 10;
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
							if(!(*it2)->to_remove && !u.IsEnemy(**it2) && !IS_SET((*it2)->data->flags, F_UNDEAD) && (*it2)->hpmax - (*it2)->hp > 100.f
								&& (dist = Vec3::Distance(u.pos, (*it2)->pos)) < spell_range && L.CanSee(u, **it2))
							{
								float prio = (*it2)->hpmax - (*it2)->hp;
								if(*it2 == &u)
									prio *= 1.5f;
								prio -= dist * 10;
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
					if(u.usable == nullptr)
					{
						if(ai.alert_target)
						{
							if(ai.alert_target->to_remove)
								ai.alert_target = nullptr;
							else
							{
								// kto� inny go powiadomi� okrzykiem o wrogu
								u.talking = false;
								u.mesh_inst->need_update = true;
								ai.in_combat = true;
								ai.target = ai.alert_target;
								ai.target_last_pos = ai.alert_target_pos;
								ai.alert_target = nullptr;
								ai.state = AIController::Fighting;
								ai.timer = 0.f;
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
							u.mesh_inst->need_update = true;
							ai.in_combat = true;
							ai.target = enemy;
							ai.target_last_pos = enemy->pos;
							ai.state = AIController::Fighting;
							ai.timer = 0.f;
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
							ai.timer = Random(2.f, 5.f);
							ai.city_wander = false;
							Unit_StopUsingUsable(ctx, u);
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

					if(u.usable && u.usable->user != &u)
					{
						// naprawa b��du gdy si� on zdarzy a nie rozwi�zanie
						Warn("Invalid usable user: %s is using %s but the user is %s.", u.data->id.c_str(), u.usable->base->id.c_str(),
							u.usable->user ? u.usable->user->data->id.c_str() : "nullptr");
						u.usable = nullptr;
#ifdef _DEBUG
						gui->messages->AddGameMsg("Invalid usable user!", 5.f);
#endif
					}
					if(u.action == A_BLOCK)
					{
						// mo�e naprawione ale p�ki co niech zostanie
						u.action = A_NONE;
						u.mesh_inst->frame_end_info2 = false;
						u.mesh_inst->Deactivate(1);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = &u;
							c.id = AID_StopBlock;
							c.f[1] = 1.f;
						}
						Warn("Unit %s (%s) blocks in idle.", u.data->id.c_str(), u.GetName());
#ifdef _DEBUG
						gui->messages->AddGameMsg("Unit blocks in idle!", 5.f);
#endif
					}
					if(u.look_target && !u.usable)
					{
						// patrzenie na posta� w czasie dialogu
						look_at = LookAtPoint;
						look_pos = u.look_target->pos;
						wander = false;
						u.timer = Random(1.f, 2.f);
					}
					else if(u.IsHero() && u.hero->mode == HeroData::Leave)
					{
						// bohater chce opu�ci� t� lokacj�
						if(u.usable)
						{
							if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
							{
								Unit_StopUsingUsable(ctx, u);
								ai.idle_action = AIController::Idle_None;
								ai.timer = Random(1.f, 2.f);
								ai.city_wander = true;
							}
						}
						else if(ai.timer <= 0.f)
						{
							ai.idle_action = AIController::Idle_MoveRegion;
							ai.timer = Random(30.f, 40.f);
							ai.idle_data.area.id = -1;
							ai.idle_data.area.pos = (u.in_building == -1 ?
								L.GetExitPos(u) :
								L.city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint().XZ());
						}
					}
					else
					{
						// chodzenie do karczmy
						if(ai.goto_inn && !(u.IsHero() && tournament->IsGenerated()))
						{
							if(u.usable)
							{
								if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
								{
									Unit_StopUsingUsable(ctx, u);
									ai.idle_action = AIController::Idle_None;
									ai.timer = Random(1.f, 2.f);
								}
							}
							else
							{
								int karczma_id;
								InsideBuilding* karczma = L.city_ctx->FindInn(karczma_id);
								if(u.in_building == -1)
								{
									// id� do karczmy
									if(ai.timer <= 0.f)
									{
										ai.idle_action = AIController::Idle_MoveRegion;
										ai.idle_data.area.pos = karczma->enter_area.Midpoint().XZ();
										ai.idle_data.area.id = karczma_id;
										ai.timer = Random(30.f, 40.f);
										ai.city_wander = true;
									}
								}
								else
								{
									if(u.in_building == karczma_id)
									{
										// jest w karczmie, id� do losowego punktu w karczmie
										ai.goto_inn = false;
										ai.timer = Random(5.f, 7.5f);
										ai.idle_action = AIController::Idle_Move;
										ai.idle_data.pos = (Rand() % 5 == 0 ? karczma->arena2.Midpoint() : karczma->arena1.Midpoint()).XZ();
									}
									else
									{
										// jest w budynku nie karczmie, wyjd� na zewn�trz
										ai.timer = Random(15.f, 30.f);
										ai.idle_action = AIController::Idle_MoveRegion;
										ai.idle_data.area.pos = L.city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint().XZ();
										ai.idle_data.area.id = -1;
									}
								}
							}
						}
						else if(((u.IsFollower() && u.hero->mode == HeroData::Follow) || u.assist) && Team.leader->in_arena == -1 && u.busy != Unit::Busy_Tournament)
						{
							Unit* leader = Team.GetLeader();
							dist = Vec3::Distance(u.pos, leader->pos);
							if(dist >= (u.assist ? 4.f : 2.f))
							{
								// pod��aj za liderem
								if(u.usable)
								{
									if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
									{
										Unit_StopUsingUsable(ctx, u);
										ai.idle_action = AIController::Idle_None;
										ai.timer = Random(1.f, 2.f);
										ai.city_wander = false;
									}
								}
								else if(leader->in_building != u.in_building)
								{
									// lider jest w innym obszarze
									ai.idle_action = AIController::Idle_RunRegion;
									ai.timer = Random(15.f, 30.f);

									if(u.in_building == -1)
									{
										// bohater nie jest w budynku, lider jest; id� do wej�cia
										ai.idle_data.area.id = leader->in_building;
										ai.idle_data.area.pos = L.city_ctx->inside_buildings[leader->in_building]->enter_area.Midpoint().XZ();
									}
									else
									{
										// bohater jest w budynku, lider na zewn�trz lub w innym; opu�� budynek
										ai.idle_data.area.id = -1;
										ai.idle_data.area.pos = L.city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint().XZ();
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
									ai.timer = Random(2.f, 5.f);
									path_unit_ignore = leader;
									wander = false;
									if(u.IsHero())
										try_phase = true;
								}
							}
							else
							{
								// odsu� si� �eby nie blokowa�
								if(u.usable)
								{
									if(u.busy != Unit::Busy_Talking && (u.action != A_ANIMATION2 || u.animation_state != AS_ANIMATION2_MOVE_TO_ENDPOINT))
									{
										Unit_StopUsingUsable(ctx, u);
										ai.idle_action = AIController::Idle_None;
										ai.timer = Random(1.f, 2.f);
										ai.city_wander = false;
									}
								}
								else
								{
									for(Unit* unit : Team.members)
									{
										if(unit != &u && Vec3::Distance(unit->pos, u.pos) < 1.f)
										{
											look_at = LookAtPoint;
											look_pos = unit->pos;
											move_type = MoveAway;
											target_pos = unit->pos;
											run_type = Walk;
											ai.idle_action = AIController::Idle_None;
											ai.timer = Random(2.f, 5.f);
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
								BaseObject* base_obj = BaseObject::Get("bloody_altar");
								Object* obj = L.local_ctx.FindObject(base_obj);

								if(obj)
								{
									// m�dl si� do o�tarza
									ai.idle_action = AIController::Idle_Pray;
									ai.idle_data.pos = obj->pos;
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
								if(IS_SET(u.data->flags, F_AI_GUARD) && AngleDiff(u.rot, ai.start_rot) > PI / 4)
									ai.idle_data.rot = ai.start_rot;
								else if(IS_SET(u.data->flags2, F2_LIMITED_ROT))
									ai.idle_data.rot = random_rot(ai.start_rot, PI / 4);
								else
									ai.idle_data.rot = Clip(Vec3::LookAtAngle(u.pos, ai.idle_data.pos) + Random(-PI / 2, PI / 2));
								ai.timer = Random(2.f, 5.f);
							}
							else if(ai.idle_action == AIController::Idle_Chat)
							{
								u.talking = false;
								u.mesh_inst->need_update = true;
								ai.idle_action = AIController::Idle_None;
							}
							else if(ai.idle_action == AIController::Idle_Use)
							{
								if(u.usable->base == stool && u.in_building != -1)
								{
									int what;
									if(u.IsDrunkman())
										what = Rand() % 3;
									else
										what = Rand() % 2 + 1;
									switch(what)
									{
									case 0: // drink
										u.ConsumeItem(Item::Get(Rand() % 3 == 0 ? "vodka" : "beer")->ToConsumable());
										ai.timer = Random(10.f, 15.f);
										break;
									case 1: // eat
										u.ConsumeItem(ItemList::GetItem("normal_food")->ToConsumable());
										ai.timer = Random(10.f, 15.f);
										break;
									case 2: // stop sitting
										Unit_StopUsingUsable(ctx, u);
										ai.idle_action = AIController::Idle_None;
										ai.timer = Random(2.5f, 5.f);
										break;
									}
								}
								else
								{
									Unit_StopUsingUsable(ctx, u);
									ai.idle_action = AIController::Idle_None;
									ai.timer = Random(2.5f, 5.f);
								}
							}
							else if(ai.idle_action == AIController::Idle_WalkUseEat)
							{
								if(u.usable)
								{
									if(u.animation_state != 0)
									{
										int what;
										if(u.IsDrunkman())
											what = Rand() % 2;
										else
											what = 1;
										switch(what)
										{
										case 0: // drink
											u.ConsumeItem(Item::Get(Rand() % 3 == 0 ? "vodka" : "beer")->ToConsumable());
											ai.timer = Random(10.f, 15.f);
											break;
										case 1: // eat
											u.ConsumeItem(ItemList::GetItem("normal_food")->ToConsumable());
											ai.timer = Random(10.f, 15.f);
											break;
										}
										ai.idle_action = AIController::Idle_Use;
									}
								}
								else
								{
									ai.idle_action = AIController::Idle_None;
									ai.timer = Random(2.f, 3.f);
								}
							}
							else if(CanWander(u))
							{
								if(u.IsHero())
								{
									if(u.in_building == -1)
									{
										// unit is outside
										int where = Rand() % (IS_SET(L.city_ctx->flags, City::HaveTrainingGrounds) ? 3 : 2);
										if(where == 0)
										{
											// go to random position
											ai.loc_timer = ai.timer = Random(30.f, 120.f);
											ai.idle_action = AIController::Idle_Move;
											ai.idle_data.pos = L.city_ctx->buildings[Rand() % L.city_ctx->buildings.size()].walk_pt
												+ Vec3::Random(Vec3(-1.f, 0, -1), Vec3(1, 0, 1));
										}
										else if(where == 1)
										{
											// go to inn
											ai.loc_timer = ai.timer = Random(75.f, 150.f);
											ai.idle_action = AIController::Idle_MoveRegion;
											ai.idle_data.area.pos = L.city_ctx->FindInn(ai.idle_data.area.id)->enter_area.Midpoint().XZ();
										}
										else if(where == 2)
										{
											// go to training grounds
											ai.loc_timer = ai.timer = Random(75.f, 150.f);
											ai.idle_action = AIController::Idle_Move;
											ai.idle_data.pos = L.city_ctx->FindBuilding(BuildingGroup::BG_TRAINING_GROUNDS)->walk_pt
												+ Vec3::Random(Vec3(-1.f, 0, -1), Vec3(1, 0, 1));
										}
									}
									else
									{
										// leave building
										ai.loc_timer = ai.timer = Random(15.f, 30.f);
										ai.idle_action = AIController::Idle_MoveRegion;
										ai.idle_data.area.pos = L.city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint().XZ();
										ai.idle_data.area.id = -1;
									}
									ai.city_wander = true;
								}
								else
								{
									// go near random building
									ai.loc_timer = ai.timer = Random(30.f, 120.f);
									ai.idle_action = AIController::Idle_Move;
									ai.idle_data.pos = L.city_ctx->buildings[Rand() % L.city_ctx->buildings.size()].walk_pt + Vec3::Random(Vec3(-1.f, 0, -1), Vec3(1, 0, 1));
									ai.city_wander = true;
								}
							}
							else if(u.guard_target && Vec3::Distance(u.pos, u.guard_target->pos) > 5.f)
							{
								ai.timer = Random(2.f, 4.f);
								ai.idle_action = AIController::Idle_WalkNearUnit;
								ai.idle_data.unit = u.guard_target;
								ai.city_wander = false;
							}
							else if(IS_SET(u.data->flags3, F3_MINER) && Rand() % 2 == 0)
							{
								// check if unit have required item
								const Item* req_item = iron_vein->item;
								if(req_item && !u.HaveItem(req_item) && u.slots[SLOT_WEAPON] != req_item)
									goto normal_idle_action;
								// find closest ore vein
								Usable* usable = nullptr;
								float range = 20.1f;
								for(vector<Usable*>::iterator it2 = ctx.usables->begin(), end2 = ctx.usables->end(); it2 != end2; ++it2)
								{
									Usable& use = **it2;
									if(!use.user && (use.base == iron_vein || use.base == gold_vein))
									{
										float dist = Vec3::Distance(use.pos, u.pos);
										if(dist < range)
										{
											range = dist;
											usable = &use;
										}
									}
								}
								// start mining if there is anything to mine
								if(usable)
								{
									ai.idle_action = AIController::Idle_WalkUse;
									ai.idle_data.usable = usable;
									ai.timer = Random(5.f, 10.f);
								}
								else
									goto normal_idle_action;
							}
							else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE)
								&& QM.quest_mages2->mages_state >= Quest_Mages2::State::OldMageJoined
								&& QM.quest_mages2->mages_state < Quest_Mages2::State::MageCured
								&& Rand() % 3 == 0)
							{
								// drink something
								u.ConsumeItem(Item::Get(Rand() % 3 == 0 ? "vodka" : "beer")->ToConsumable(), true);
								ai.idle_action = AIController::Idle_None;
								ai.timer = Random(3.f, 6.f);
							}
							else
							{
							normal_idle_action:
								// random ai action
								int what = AI_NOT_SET;
								if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
									what = Rand() % 3; // animation, rotate, look
								else if((u.busy == Unit::Busy_Tournament || (u.IsHero() && !u.IsFollowingTeamMember() && tournament->IsGenerated()))
									&& tournament->GetMaster()
									&& ((dist = Vec3::Distance2d(u.pos, tournament->GetMaster()->pos)) > 16.f || dist < 4.f))
								{
									what = AI_NONE;
									if(dist > 16.f)
									{
										ai.timer = Random(5.f, 10.f);
										ai.idle_action = AIController::Idle_WalkNearUnit;
										ai.idle_data.unit = tournament->GetMaster();
									}
									else
									{
										ai.timer = Random(4.f, 8.f);
										ai.idle_action = AIController::Idle_Move;
										ai.idle_data.pos = tournament->GetMaster()->pos + Vec3::Random(Vec3(-10, 0, -10), Vec3(10, 0, 10));
									}
								}
								else if(IS_SET(u.data->flags2, F2_SIT_ON_THRONE) && !u.IsFollower())
									what = AI_USE;
								else if(u.data->type == UNIT_TYPE::HUMAN)
								{
									if(!IS_SET(u.data->flags, F_AI_GUARD))
									{
										if(IS_SET(u.data->flags2, F2_AI_TRAIN) && Rand() % 5 == 0)
										{
											static vector<Object*> do_cw;
											BaseObject* manekin = BaseObject::Get("melee_target"),
												*tarcza = BaseObject::Get("bow_target");
											for(vector<Object*>::iterator it2 = ctx.objects->begin(), end2 = ctx.objects->end(); it2 != end2; ++it2)
											{
												Object& obj = **it2;
												if((obj.base == manekin || (obj.base == tarcza && u.HaveBow())) && Vec3::Distance(obj.pos, u.pos) < 10.f)
													do_cw.push_back(&obj);
											}
											if(!do_cw.empty())
											{
												Object& o = *do_cw[Rand() % do_cw.size()];
												if(o.base == manekin)
												{
													ai.timer = Random(10.f, 30.f);
													ai.idle_action = AIController::Idle_TrainCombat;
													ai.idle_data.pos = o.pos;
												}
												else
												{
													ai.timer = Random(15.f, 35.f);
													ai.idle_action = AIController::Idle_TrainBow;
													ai.idle_data.obj.pos = o.pos;
													ai.idle_data.obj.rot = o.rot.y;
													ai.idle_data.obj.ptr = &o;
												}
												do_cw.clear();
												what = AI_NONE;
											}
										}
										if(what != AI_NONE)
										{
											if(IS_SET(u.data->flags3, F3_DONT_EAT))
												what = Rand() % 6;
											else
												what = Rand() % 7;
										}
									}
									else // guard (don't move/use objects)
									{
										if(IS_SET(u.data->flags3, F3_DONT_EAT))
											what = Rand() % 5;
										else
											what = Rand() % 7;
										if(what == AI_MOVE)
											what = Rand() % 3;
									}
								}
								else // not human
								{
									if(IS_SET(u.data->flags3, F3_DONT_EAT))
										what = Rand() % 5;
									else
									{
										what = Rand() % 7;
										if(what == AI_USE)
											what = Rand() % 3;
									}
								}

								// don't talk when have flag F2_DONT_TALK
								// or have flag F3_TALK_AT_COMPETITION and there is competition started
								if(what == AI_TALK)
								{
									if(IS_SET(u.data->flags2, F2_DONT_TALK))
										what = Rand() % 3;
									else if(IS_SET(u.data->flags3, F3_TALK_AT_COMPETITION)
										&& (QM.quest_contest->state >= Quest_Contest::CONTEST_STARTING
										|| tournament->GetState() >= Quest_Tournament::TOURNAMENT_STARTING))
									{
										what = AI_LOOK;
									}
								}

								assert(what != AI_NOT_SET);
								switch(what)
								{
								case AI_NONE:
									break;
								case AI_USE:
									if(ctx.usables)
									{
										static vector<Usable*> uses;
										for(vector<Usable*>::iterator it2 = ctx.usables->begin(), end2 = ctx.usables->end(); it2 != end2; ++it2)
										{
											Usable& use = **it2;
											if(!use.user && (use.base != throne || IS_SET(u.data->flags2, F2_SIT_ON_THRONE))
												&& Vec3::Distance(use.pos, u.pos) < 10.f && !use.base->IsContainer()
												/*CanSee - niestety nie ma takiej funkcji wi�c trudno :p*/)
											{
												const Item* needed_item = use.base->item;
												if(!needed_item || u.HaveItem(needed_item) || u.slots[SLOT_WEAPON] == needed_item)
													uses.push_back(*it2);
											}
										}
										if(!uses.empty())
										{
											ai.idle_action = AIController::Idle_WalkUse;
											ai.idle_data.usable = uses[Rand() % uses.size()];
											ai.timer = Random(3.f, 6.f);
											if(ai.idle_data.usable->base == stool && Rand() % 3 == 0)
												ai.idle_action = AIController::Idle_WalkUseEat;
											uses.clear();
											break;
										}
									}
									// nothing to use, play animation
								case AI_ANIMATION:
									{
										int id = Rand() % u.data->idles->anims.size();
										ai.timer = Random(2.f, 5.f);
										ai.idle_action = AIController::Idle_Animation;
										u.mesh_inst->Play(u.data->idles->anims[id].c_str(), PLAY_ONCE, 0);
										u.mesh_inst->groups[0].speed = 1.f;
										u.mesh_inst->frame_end_info = false;
										u.animation = ANI_IDLE;
										if(Net::IsOnline())
										{
											NetChange& c = Add1(Net::changes);
											c.type = NetChange::IDLE;
											c.unit = &u;
											c.id = id;
										}
									}
									break;
								case AI_LOOK:
									if(u.busy == Unit::Busy_Tournament && Rand() % 2 == 0 && tournament->GetMaster())
									{
										ai.timer = Random(1.5f, 2.5f);
										ai.idle_action = AIController::Idle_Look;
										ai.idle_data.unit = tournament->GetMaster();
										break;
									}
									else
									{
										close_enemies.clear();
										for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
										{
											if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->invisible || *it2 == &u)
												continue;

											if(Vec3::Distance(u.pos, (*it2)->pos) < 10.f && L.CanSee(u, **it2))
												close_enemies.push_back(*it2);
										}
										if(!close_enemies.empty())
										{
											ai.timer = Random(1.5f, 2.5f);
											ai.idle_action = AIController::Idle_Look;
											ai.idle_data.unit = close_enemies[Rand() % close_enemies.size()];
											break;
										}
									}
									// no close units, rotate
								case AI_ROTATE:
									ai.timer = Random(2.f, 5.f);
									ai.idle_action = AIController::Idle_Rot;
									if(IS_SET(u.data->flags, F_AI_GUARD) && AngleDiff(u.rot, ai.start_rot) > PI / 4)
										ai.idle_data.rot = ai.start_rot;
									else if(IS_SET(u.data->flags2, F2_LIMITED_ROT))
										ai.idle_data.rot = random_rot(ai.start_rot, PI / 4);
									else
										ai.idle_data.rot = Random(MAX_ANGLE);
									break;
								case AI_TALK:
									{
										const float d = ((IS_SET(u.data->flags, F_AI_GUARD) || IS_SET(u.data->flags, F_AI_STAY)) ? 1.5f : 10.f);
										close_enemies.clear();
										for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
										{
											if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->invisible || *it2 == &u)
												continue;

											if(Vec3::Distance(u.pos, (*it2)->pos) < d && L.CanSee(u, **it2))
												close_enemies.push_back(*it2);
										}
										if(!close_enemies.empty())
										{
											ai.timer = Random(3.f, 6.f);
											ai.idle_action = AIController::Idle_WalkTo;
											ai.idle_data.unit = close_enemies[Rand() % close_enemies.size()];
											ai.target_last_pos = ai.idle_data.unit->pos;
											break;
										}
									}
									// brak pobliskich jednostek, id� losowo
									if(IS_SET(u.data->flags, F_AI_GUARD))
										break;
								case AI_MOVE:
									if(IS_SET(u.data->flags, F_AI_GUARD))
										break;
									ai.timer = Random(3.f, 6.f);
									ai.idle_action = AIController::Idle_Move;
									ai.city_wander = false;
									if(IS_SET(u.data->flags, F_AI_STAY))
									{
										if(Vec3::Distance(u.pos, ai.start_pos) > 2.f)
											ai.idle_data.pos = ai.start_pos;
										else
											ai.idle_data.pos = u.pos + Vec3::Random(Vec3(-2.f, 0, -2.f), Vec3(2.f, 0, 2.f));
									}
									else
										ai.idle_data.pos = u.pos + Vec3::Random(Vec3(-5.f, 0, -5.f), Vec3(5.f, 0, 5.f));
									if(L.city_ctx && !L.city_ctx->IsInsideCity(ai.idle_data.pos))
									{
										ai.timer = Random(2.f, 4.f);
										ai.idle_action = AIController::Idle_None;
									}
									else if(!L.location->outside)
									{
										InsideLocation* inside = (InsideLocation*)L.location;
										if(!inside->GetLevelData().IsValidWalkPos(ai.idle_data.pos, u.GetUnitRadius()))
										{
											ai.timer = Random(2.f, 4.f);
											ai.idle_action = AIController::Idle_None;
										}
									}
									break;
								case AI_EAT:
									ai.timer = Random(3.f, 5.f);
									ai.idle_action = AIController::Idle_None;
									u.ConsumeItem(ItemList::GetItem(IS_SET(u.data->flags3, F3_ORC_FOOD) ? "orc_food" : "normal_food")->ToConsumable());
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
								if(Equal(u.rot, ai.idle_data.rot))
									ai.idle_action = AIController::Idle_None;
								else
								{
									look_at = LookAtAngle;
									look_pos.x = ai.idle_data.rot;
								}
								break;
							case AIController::Idle_Move:
								if(Vec3::Distance2d(u.pos, ai.idle_data.pos) < u.GetUnitRadius() * 2)
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
								if(ai.idle_data.unit->to_remove || Vec3::Distance2d(u.pos, ai.idle_data.unit->pos) > 10.f || !L.CanSee(u, *ai.idle_data.unit))
								{
									// sko�cz si� patrzy�
									ai.idle_action = AIController::Idle_Rot;
									if(IS_SET(u.data->flags, F_AI_GUARD) && AngleDiff(u.rot, ai.start_rot) > PI / 4)
										ai.idle_data.rot = ai.start_rot;
									else if(IS_SET(u.data->flags2, F2_LIMITED_ROT))
										ai.idle_data.rot = random_rot(ai.start_rot, PI / 4);
									else
										ai.idle_data.rot = Random(MAX_ANGLE);
									ai.timer = Random(2.f, 5.f);
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
									if(L.CanSee(u, *ai.idle_data.unit))
									{
										if(Vec3::Distance2d(u.pos, ai.idle_data.unit->pos) < 1.5f)
										{
											// sko�cz podchodzi�, zacznij gada�
											ai.idle_action = AIController::Idle_Chat;
											ai.timer = Random(2.f, 3.f);

											cstring msg = GetRandomIdleText(u);

											int ani = 0;
											gui->game_gui->AddSpeechBubble(&u, msg);
											if(u.data->type == UNIT_TYPE::HUMAN && Rand() % 3 != 0)
											{
												ani = Rand() % 2 + 1;
												u.mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
												u.mesh_inst->groups[0].speed = 1.f;
												u.animation = ANI_PLAY;
												u.action = A_ANIMATION;
											}

											if(ai.idle_data.unit->IsStanding() && ai.idle_data.unit->IsAI())
											{
												AIController& ai2 = *ai.idle_data.unit->ai;
												if(ai2.state == AIController::Idle
													&& OR3_EQ(ai2.idle_action, AIController::Idle_None, AIController::Idle_Rot, AIController::Idle_Look))
												{
													// odwzajemnij patrzenie si�
													ai2.idle_action = AIController::Idle_Chat;
													ai2.timer = ai.timer + Random(1.f);
													ai2.idle_data.unit = &u;
												}
											}

											if(Net::IsOnline())
											{
												NetChange& c = Add1(Net::changes);
												c.type = NetChange::TALK;
												c.unit = &u;
												c.str = StringPool.Get();
												*c.str = msg;
												c.id = ani;
												c.count = 0;
												N.net_strs.push_back(c.str);
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
										if(Vec3::Distance2d(u.pos, ai.target_last_pos) < 1.5f)
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
									if(Vec3::Distance2d(u.pos, ai.idle_data.unit->pos) < 4.f)
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
									Usable& use = *ai.idle_data.usable;
									if(use.user || u.frozen != FROZEN::NO)
										ai.idle_action = AIController::Idle_None;
									else if(Vec3::Distance2d(u.pos, use.pos) < PICKUP_RANGE)
									{
										if(AngleDiff(Clip(u.rot + PI / 2), Clip(-Vec3::Angle2d(u.pos, ai.idle_data.usable->pos))) < PI / 4)
										{
											BaseUsable& base = *use.base;
											const Item* needed_item = base.item;
											if(!needed_item || u.HaveItem(needed_item) || u.slots[SLOT_WEAPON] == needed_item)
											{
												u.action = A_ANIMATION2;
												u.animation = ANI_PLAY;
												bool read_papers = false;
												if(use.base == chair && IS_SET(u.data->flags, F_AI_CLERK))
												{
													read_papers = true;
													u.mesh_inst->Play("czyta_papiery", PLAY_PRIO3, 0);
												}
												else
													u.mesh_inst->Play(base.anim.c_str(), PLAY_PRIO1, 0);
												u.mesh_inst->groups[0].speed = 1.f;
												u.UseUsable(&use);
												u.target_pos = u.pos;
												u.target_pos2 = use.pos;
												if(use.base->limit_rot == 4)
													u.target_pos2 -= Vec3(sin(use.rot)*1.5f, 0, cos(use.rot)*1.5f);
												u.timer = 0.f;
												u.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
												u.use_rot = Vec3::LookAtAngle(u.pos, u.usable->pos);
												u.used_item = needed_item;
												if(ai.idle_action == AIController::Idle_WalkUseEat)
													ai.timer = -1.f;
												else
												{
													ai.idle_action = AIController::Idle_Use;
													if(u.usable->base == stool && u.in_building != -1 && u.IsDrunkman())
														ai.timer = Random(10.f, 20.f);
													else if(u.usable->base == throne)
														ai.timer = 120.f;
													else if(u.usable->base == iron_vein || u.usable->base == gold_vein)
														ai.timer = Random(20.f, 30.f);
													else
														ai.timer = Random(5.f, 10.f);
												}
												if(Net::IsOnline())
												{
													NetChange& c = Add1(Net::changes);
													c.type = NetChange::USE_USABLE;
													c.unit = &u;
													c.id = use.netid;
													c.count = (read_papers ? USE_USABLE_START_SPECIAL : USE_USABLE_START);
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
										if(use.base->limit_rot == 4)
											target_pos -= Vec3(sin(use.rot), 0, cos(use.rot));
									}
								}
								break;
							case AIController::Idle_Use:
								break;
							case AIController::Idle_TrainCombat:
								if(Vec3::Distance2d(u.pos, ai.idle_data.pos) < u.GetUnitRadius() * 2 + 0.25f)
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
									Vec3 pt = ai.idle_data.obj.pos;
									pt -= Vec3(sin(ai.idle_data.obj.rot) * 5, 0, cos(ai.idle_data.obj.rot) * 5);
									if(Vec3::Distance2d(u.pos, pt) < u.GetUnitRadius() * 2)
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
											float dir = Vec3::LookAtAngle(u.pos, ai.idle_data.obj.pos);
											if(AngleDiff(u.rot, dir) < PI / 4 && u.action == A_NONE && u.weapon_taken == W_BOW && ai.next_attack <= 0.f
												&& u.frozen == FROZEN::NO && u.stamina > 0 && CanShootAtLocation2(u, ai.idle_data.obj.ptr, ai.idle_data.obj.pos))
											{
												// strzelanie z �uku
												float speed = u.GetBowAttackSpeed();
												u.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
												u.mesh_inst->groups[1].speed = speed;
												u.action = A_SHOOT;
												u.animation_state = 1;
												u.hitted = false;
												u.bow_instance = GetBowInstance(u.GetBow().mesh);
												u.bow_instance->Play(&u.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND | PLAY_RESTORE, 0);
												u.bow_instance->groups[0].speed = speed;
												u.RemoveStamina(Unit::STAMINA_BOW_ATTACK);

												if(Net::IsOnline())
												{
													NetChange& c = Add1(Net::changes);
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
												ai.shoot_yspeed = (ai.idle_data.obj.pos.y - u.pos.y) / (Vec3::Distance(u.pos, ai.idle_data.obj.pos) / u.GetArrowSpeed());
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
								if(Vec3::Distance2d(u.pos, ai.idle_data.area.pos) < u.GetUnitRadius() * 2)
								{
									if(L.city_ctx && !IS_SET(L.city_ctx->flags, City::HaveExit) && ai.idle_data.area.id == -1 && u.in_building == -1)
									{
										// in exit area, go to border
										ai.idle_data.area.pos = L.GetExitPos(u, true);
										ai.idle_data.area.id = -2;
									}
									else
									{
										if(ai.idle_data.area.id == -2)
											ai.idle_data.area.id = -1;
										ai.idle_action = AIController::Idle_None;
										L.WarpUnit(&u, ai.idle_data.area.id);
										if(ai.idle_data.area.id == -1 || (u.IsFollower() && u.hero->mode == HeroData::Follow))
										{
											ai.loc_timer = -1.f;
											ai.timer = -1.f;
										}
										else
										{
											// sta� gdzie� w karczmie
											ai.idle_action = AIController::Idle_Move;
											ai.timer = Random(5.f, 15.f);
											ai.loc_timer = Random(60.f, 120.f);
											InsideBuilding* inside = L.city_ctx->inside_buildings[ai.idle_data.area.id];
											ai.idle_data.pos = (Rand() % 5 == 0 ? inside->arena2 : inside->arena1).GetRandomPos3();
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
								if(Vec3::Distance2d(u.pos, u.ai->start_pos) > 1.f)
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
								if(Vec3::Distance2d(u.pos, ai.idle_data.unit->pos) < 2.f)
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
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
						}
						else
						{
							// id� w ostatnio widziane miejsce
							ai.state = AIController::SeenEnemy;
							ai.timer = Random(10.f, 15.f);
						}
						repeat = true;
						break;
					}

					// ignoruj alert target
					if(ai.alert_target)
						ai.alert_target = nullptr;

					bool tick = false;
					if(ai.timer <= 0.f)
					{
						tick = true;
						ai.timer = 0.25f;
					}

					// drink healing potion
					if(tick && Rand() % 4 == 0)
						ai.CheckPotion(true);

					// wait for stamina
					float stamina = u.GetStaminap();
					if(tick && u.action == A_NONE && (stamina < 0.25f || Rand() % 6 == 0))
					{
						if(Rand() % 4 == 0 || stamina <= 0.f)
						{
							ai.timer = Random(0.5f, 1.f);
							if(stamina <= 0.f)
								ai.timer += 1.f;
							ai.state = AIController::Wait;
							break;
						}
						else
							ai.timer = 0.25f;
					}

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

					if(u.data->spells && u.action == A_NONE && u.frozen == FROZEN::NO)
					{
						bool break_action = false;

						// rzucanie czar�w
						for(int i = 2; i >= 0; --i)
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
										ai.cooldown[i] = u.data->spells->spell[i]->cooldown.Random();
										u.action = A_CAST;
										u.attack_id = i;
										u.animation_state = 0;

										if(u.mesh_inst->mesh->head.n_groups == 2)
										{
											u.mesh_inst->frame_end_info2 = false;
											u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
										}
										else
										{
											u.mesh_inst->frame_end_info = false;
											u.animation = ANI_PLAY;
											u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
											u.mesh_inst->groups[0].speed = 1.f;
										}

										if(Net::IsOnline())
										{
											NetChange& c = Add1(Net::changes);
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
						ai.shoot_yspeed = (enemy->pos.y - u.pos.y) / (Vec3::Distance(u.pos, enemy->pos) / arrow_speed);
					}

					if(u.IsHoldingBow())
					{
						if(u.action == A_NONE && ai.next_attack <= 0.f && u.frozen == FROZEN::NO && u.stamina > 0)
						{
							// sprawd� czy mo�esz strzela� we wroga
							look_pos = PredictTargetPos(u, *enemy, u.GetArrowSpeed());

							if(CanShootAtLocation(u, *enemy, enemy->pos))
							{
								// strzelanie z �uku
								float speed = u.GetBowAttackSpeed();
								u.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
								u.mesh_inst->groups[1].speed = speed;
								u.action = A_SHOOT;
								u.animation_state = 1;
								u.hitted = false;
								u.bow_instance = GetBowInstance(u.GetBow().mesh);
								u.bow_instance->Play(&u.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND | PLAY_RESTORE, 0);
								u.bow_instance->groups[0].speed = speed;
								u.RemoveStamina(Unit::STAMINA_BOW_ATTACK);

								if(Net::IsOnline())
								{
									NetChange& c = Add1(Net::changes);
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
						// attack
						if(u.action == A_NONE && ai.next_attack <= 0.f && u.frozen == FROZEN::NO)
						{
							if(best_dist <= u.GetAttackRange() + 1.f)
								AI_DoAttack(ai, enemy);
							else if(u.CanRun() && best_dist <= u.GetAttackRange() * 3 + 1.f)
								AI_DoAttack(ai, enemy, true);
						}

						// stay close but not too close
						if(ai.state == AIController::Fighting && enemy && !(u.IsHoldingBow() || IS_SET(u.data->flags, F_MAGE)))
						{
							look_at = LookAtTarget;
							target_pos = enemy->pos;
							path_unit_ignore = enemy;
							run_type = WalkIfNear;
							if(u.action == A_ATTACK)
							{
								if(best_dist > u.GetAttackRange())
									move_type = MovePoint;
								else if(best_dist < u.GetAttackRange() / 2)
									move_type = MoveAway;
							}
							else
							{
								if(best_dist < u.GetAttackRange() + 0.5f)
									move_type = MoveAway;
								else if(best_dist > u.GetAttackRange() + 1.f)
									move_type = MovePoint;
							}
						}

						// block/jump back
						if(ai.ignore <= 0.f)
						{
							// wybierz najbli�sza atakuj�c� posta�
							Unit* top = nullptr;
							float best_dist = JUMP_BACK_MIN_RANGE;

							for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
							{
								if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && u.IsEnemy(**it2) && (*it2)->action == A_ATTACK && !(*it2)->hitted
									&& (*it2)->animation_state < 2)
								{
									float dist = Vec3::Distance(u.pos, (*it2)->pos);
									if(dist < best_dist)
									{
										top = *it2;
										best_dist = dist;
									}
								}
							}

							if(top)
							{
								if(u.CanBlock() && u.action == A_NONE && Rand() % 3 > 0)
								{
									// blokowanie
									float speed = u.GetBlockSpeed();
									ai.state = AIController::Block;
									ai.timer = BLOCK_TIMER;
									ai.ignore = 0.f;
									u.action = A_BLOCK;
									u.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END | PLAY_RESTORE, 1);
									u.mesh_inst->groups[1].blend_max = speed;
									u.animation_state = 0;

									if(Net::IsOnline())
									{
										NetChange& c = Add1(Net::changes);
										c.type = NetChange::ATTACK;
										c.unit = &u;
										c.id = AID_Block;
										c.f[1] = speed;
									}
								}
								else if(Rand() % 3 == 0)
								{
									// odskok
									ai.state = AIController::Dodge;
									ai.timer = JUMP_BACK_TIMER;

									if(Rand() % 2 == 0)
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
						ai.timer = 0.f;
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
							ai.timer = 0.f;
							ai.alert_target = nullptr;
							repeat = true;
							break;
						}
					}

					move_type = MovePoint;
					target_pos = ai.target_last_pos;
					look_at = LookAtWalk;
					run_type = Run;

					if(Vec3::Distance(u.pos, ai.target_last_pos) < 1.f || ai.timer <= 0.f)
					{
						// doszed� do ostatniego widzianego punktu
						if(L.location->outside)
						{
							// jest na zewn�trz wi�c nie ma co robi�
							ai.state = AIController::Idle;
							ai.idle_action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
						}
						else
						{
							InsideLocation* inside = (InsideLocation*)L.location;
							Room* room = inside->FindChaseRoom(u.pos);
							if(room)
							{
								// jest w podziemiach, id� do losowego pobliskiego pokoju
								ai.timer = u.IsFollower() ? Random(1.f, 2.f) : Random(15.f, 30.f);
								ai.state = AIController::SearchEnemy;
								int gdzie = room->connected[Rand() % room->connected.size()];
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
								ai.loc_timer = Random(5.f, 10.f);
								ai.timer = Random(1.f, 2.f);
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
						ai.timer = 0.f;
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
							ai.timer = 0.f;
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
						ai.loc_timer = Random(5.f, 10.f);
						ai.timer = Random(1.f, 2.f);
					}
					else if(Vec3::Distance(u.pos, ai.target_last_pos) < 1.f)
					{
						// szukaj kolejnego pokoju
						InsideLocation* inside = (InsideLocation*)L.location;
						InsideLocationLevel& lvl = inside->GetLevelData();
						Room* room = lvl.GetNearestRoom(u.pos);
						int gdzie = room->connected[Rand() % room->connected.size()];
						ai.escape_room = &lvl.rooms[gdzie];
						ai.target_last_pos = ai.escape_room->GetRandomPos(u.GetUnitRadius());
					}
				}
				break;

			//===================================================================================================================
			// uciekaj
			case AIController::Escape:
				{
					// try to heal if you can and should
					ai.CheckPotion();

					// check if should finish escaping
					ai.timer -= dt;
					if(ai.timer <= 0.f)
					{
						if(ai.GetMorale() > 0.f)
						{
							ai.state = AIController::Fighting;
							ai.timer = 0.f;
							ai.escape_room = nullptr;
							repeat = true;
							break;
						}
						else
							ai.timer = Random(2.f, 4.f);
					}

					if(enemy)
					{
						ai.target_last_pos = enemy->pos;
						if(L.location->outside)
						{
							// zaawansowane uciekanie tylko w podziemiach, na zewn�trz uciekaj przed siebie
							if(Vec3::Distance2d(enemy->pos, u.pos) < 3.f)
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
									if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->invisible || !u.IsEnemy(**it2))
										continue;

									if(Vec3::Distance(u.pos, (*it2)->pos) < ALERT_RANGE.x + 0.1f)
										close_enemies.push_back(*it2);
								}

								// ustal w kt�rym pokoju s� wrogowie
								Vec3 mid = close_enemies.front()->pos;
								for(vector<Unit*>::iterator it2 = close_enemies.begin() + 1, end2 = close_enemies.end(); it2 != end2; ++it2)
									mid += (*it2)->pos;
								mid /= (float)close_enemies.size();

								// kt�re to pomieszczenie?
								Room* room = ((InsideLocation*)L.location)->GetLevelData().FindEscapeRoom(u.pos, mid);

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
					}
					else
					{
						// nie ma ju� wrog�w
						if(ai.escape_room)
						{
							// dobiegnij na miejsce
							if(!ai.escape_room->IsInside(u.pos))
							{
								move_type = MovePoint;
								look_at = LookAtWalk;
								target_pos = ai.escape_room->Center();
							}
						}
						else
						{
							move_type = MoveAway;
							look_at = LookAtWalk;
							target_pos = ai.target_last_pos;
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
						if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && u.IsEnemy(**it2) && (*it2)->action == A_ATTACK && !(*it2)->hitted
							&& (*it2)->animation_state < 2)
						{
							float dist = Vec3::Distance(u.pos, (*it2)->pos);
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
						if(best_dist <= u.GetAttackRange() && !u.mesh_inst->groups[1].IsBlending() && ai.ignore <= 0.f && u.stamina > 0)
						{
							if(Rand() % 2 == 0)
							{
								u.action = A_BASH;
								u.animation_state = 0;
								u.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1 | PLAY_RESTORE, 1);
								u.mesh_inst->groups[1].speed = 2.f;
								u.mesh_inst->frame_end_info2 = false;
								u.hitted = false;
								u.RemoveStamina(Unit::STAMINA_BASH_ATTACK);

								if(Net::IsOnline())
								{
									NetChange& c = Add1(Net::changes);
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
						u.mesh_inst->frame_end_info2 = false;
						u.mesh_inst->Deactivate(1);
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
						ai.ignore = BLOCK_AFTER_BLOCK_TIMER;
						repeat = true;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
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
						if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && u.IsEnemy(**it2))
						{
							float dist = Vec3::Distance(u.pos, (*it2)->pos);
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
						if(u.action == A_NONE && best_dist <= u.GetAttackRange() && ai.next_attack <= 0.f && u.frozen == FROZEN::NO)
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
						ai.timer = 0.f;
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

						ai.cooldown[u.attack_id] = s.cooldown.Random();
						u.action = A_CAST;
						u.animation_state = 0;
						u.target_pos = u.pos;

						if(u.mesh_inst->mesh->head.n_groups == 2)
						{
							u.mesh_inst->frame_end_info2 = false;
							u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
						}
						else
						{
							u.mesh_inst->frame_end_info = false;
							u.animation = ANI_PLAY;
							u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
							u.mesh_inst->groups[0].speed = 1.f;
						}
					}
				}
				else
				{
					if(!ValidateTarget(u, ai.cast_target))
					{
						ai.state = AIController::Idle;
						ai.idle_action = AIController::Idle_None;
						ai.timer = Random(1.f, 2.f);
						ai.loc_timer = Random(5.f, 10.f);
						ai.timer = Random(1.f, 2.f);
						break;
					}

					move_type = MovePoint;
					run_type = WalkIfNear;
					look_at = LookAtPoint;
					target_pos = look_pos = ai.cast_target->pos;

					if(Vec3::Distance(u.pos, target_pos) <= 2.5f)
					{
						move_type = DontMove;

						if(u.action == A_NONE)
						{
							Spell& s = *u.data->spells->spell[u.attack_id];

							ai.cooldown[u.attack_id] = s.cooldown.Random();
							u.action = A_CAST;
							u.animation_state = 0;
							u.target_pos = target_pos;

							if(u.mesh_inst->mesh->head.n_groups == 2)
							{
								u.mesh_inst->frame_end_info2 = false;
								u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
							}
							else
							{
								u.mesh_inst->frame_end_info = false;
								u.animation = ANI_PLAY;
								u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
								u.mesh_inst->groups[0].speed = 1.f;
							}
						}
					}

					if(u.action == A_CAST)
						u.target_pos = target_pos;
				}
				break;

			//===================================================================================================================
			// wait for stamina
			case AIController::Wait:
				{
					Unit* top = nullptr;
					float best_dist = 10.f;

					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->invisible && u.IsEnemy(**it2))
						{
							float dist = Vec3::Distance(u.pos, (*it2)->pos);
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
						if(u.IsHoldingBow() || IS_SET(u.data->flags, F_MAGE))
							move_type = KeepDistanceCheck;
						else
						{
							if(best_dist < 1.5f)
								move_type = MoveAway;
							else if(best_dist > 2.f)
							{
								move_type = MovePoint;
								run_type = Walk;
							}
						}
					}

					// end of waiting or no enemies
					if(ai.timer <= 0.f || !top)
					{
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
					}
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
		if(move_type != DontMove && u.frozen == FROZEN::NO)
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
				target_pos.Normalize();
				target_pos = u.pos + target_pos * 20;
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
					if(AngleDiff(u.rot, Vec3::LookAtAngle(u.pos, target_pos)) > PI / 4)
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
				u.prev_speed = Clamp((u.prev_speed + (u.speed - u.prev_speed)*dt * 3), 0.f, u.speed);
				float speed = u.prev_speed * dt;
				const float kat = Vec3::LookAtAngle(u.pos, target_pos);

				u.prev_pos = u.pos;

				const Vec3 dir(sin(kat)*speed, 0, cos(kat)*speed);
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
				const Int2 my_tile(u.pos / 2);
				const Int2 target_tile(target_pos / 2);
				const Int2 my_local_tile(u.pos.x * 4, u.pos.z * 4);

				if(my_tile != target_tile)
				{
					// check is it time to use pathfinding
					if(ai.pf_state == AIController::PFS_NOT_USING
						|| Int2::Distance(ai.pf_target_tile, target_tile) > 1
						|| ((ai.pf_state == AIController::PFS_WALKING || ai.pf_state == AIController::PFS_MANUAL_WALK)
							&& target_tile != ai.pf_target_tile && ai.pf_timer <= 0.f))
					{
						ai.pf_timer = Random(0.2f, 0.4f);
						if(pathfinding->FindPath(ctx, my_tile, target_tile, ai.pf_path, !IS_SET(u.data->flags, F_DONT_OPEN), ai.city_wander && L.city_ctx != nullptr))
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
						if(door.IsBlocking() && door.state == Door::Closed && door.locked == LOCK_NONE && Vec3::Distance(door.pos, u.pos) < 1.f)
						{
							// otw�rz magicznie drzwi :o
							if(!L.location->outside)
								L.minimap_opened_doors = true;
							door.state = Door::Opening;
							door.mesh_inst->Play(&door.mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
							door.mesh_inst->frame_end_info = false;

							// play sound
							if(Rand() % 2 == 0)
								sound_mgr->PlaySound3d(sDoor[Rand() % 3], door.GetCenter(), 2.f, 5.f);

							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::USE_DOOR;
								c.id = door.netid;
								c.count = 0;
							}
						}
					}
				}

				// local pathfinding
				Vec3 move_target;
				if(ai.pf_state == AIController::PFS_GLOBAL_DONE ||
					ai.pf_state == AIController::PFS_GLOBAL_NOT_USED ||
					ai.pf_state == AIController::PFS_LOCAL_TRY_WALK)
				{
					Int2 local_tile;
					bool is_end_point;

					if(ai.pf_state == AIController::PFS_GLOBAL_DONE)
					{
						const Int2& pt = ai.pf_path.back();
						local_tile.x = (2 * pt.x + 1) * 4;
						local_tile.y = (2 * pt.y + 1) * 4;
						is_end_point = (ai.pf_path.size() == 1);
					}
					else if(ai.pf_state == AIController::PFS_GLOBAL_NOT_USED)
					{
						local_tile.x = int(target_pos.x * 4);
						local_tile.y = int(target_pos.z * 4);
						if(Int2::Distance(my_local_tile, local_tile) > 32)
						{
							move_target = Vec3(target_pos.x, 0, target_pos.z);
							goto skip_localpf;
						}
						else
							is_end_point = true;
					}
					else
					{
						const Int2& pt = ai.pf_path[ai.pf_path.size() - ai.pf_local_try - 1];
						local_tile.x = (2 * pt.x + 1) * 4;
						local_tile.y = (2 * pt.y + 1) * 4;
						is_end_point = (ai.pf_local_try + 1 == ai.pf_path.size());
					}

					int ret = pathfinding->FindLocalPath(ctx, ai.pf_local_path, my_local_tile, local_tile, &u, path_unit_ignore, path_obj_ignore, is_end_point);
					ai.pf_local_target_tile = local_tile;
					if(ret == 0)
					{
						// local path found
						assert(!ai.pf_local_path.empty());
						if(ai.pf_state == AIController::PFS_LOCAL_TRY_WALK)
						{
							for(int i = 0; i < ai.pf_local_try; ++i)
								ai.pf_path.pop_back();
							ai.pf_state = AIController::PFS_WALKING;
						}
						else if(ai.pf_state == AIController::PFS_GLOBAL_DONE)
							ai.pf_state = AIController::PFS_WALKING;
						else
							ai.pf_state = AIController::PFS_WALKING_LOCAL;
						ai.pf_local_target_tile = local_tile;
						const Int2& pt = ai.pf_local_path.back();
						move_target = Vec3(0.25f*pt.x + 0.125f, 0, 0.25f*pt.y + 0.125f);
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
								move_target = Vec3(0.25f*local_tile.x + 0.125f, 0, 0.25f*local_tile.y + 0.125f);
							}
							else
								move = 0;
						}
						else if(ai.pf_path.size() > 1u)
						{
							ai.pf_state = AIController::PFS_LOCAL_TRY_WALK;
							ai.pf_local_try = 1;
							move = 0;
						}
						else
						{
							ai.pf_state = AIController::PFS_MANUAL_WALK;
							move_target = Vec3(0.25f*local_tile.x + 0.125f, 0, 0.25f*local_tile.y + 0.125f);
						}
					}
					else
					{
						ai.pf_state = AIController::PFS_MANUAL_WALK;
						move_target = Vec3(0.25f*local_tile.x + 0.125f, 0, 0.25f*local_tile.y + 0.125f);
					}
				}
				else if(ai.pf_state == AIController::PFS_MANUAL_WALK)
					move_target = target_pos;
				else if(ai.pf_state == AIController::PFS_WALKING ||
					ai.pf_state == AIController::PFS_WALKING_LOCAL)
				{
					const Int2& pt = ai.pf_local_path.back();
					move_target = Vec3(0.25f*pt.x + 0.125f, 0, 0.25f*pt.y + 0.125f);
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
							run = (Vec3::Distance(u.pos, target_pos) >= 1.5f);
							break;
						case WalkIfNear2:
							run = (Vec3::Distance(u.pos, target_pos) >= 3.f);
							break;
						default:
							assert(0);
							run = true;
							break;
						}
					}

					u.speed = run ? u.GetRunSpeed() : u.GetWalkSpeed();
					u.prev_speed = Clamp((u.prev_speed + (u.speed - u.prev_speed)*dt * 3), 0.f, u.speed);
					float speed = u.prev_speed * dt;
					const float kat = Vec3::LookAtAngle(u.pos, move_target) + PI;

					u.prev_pos = u.pos;

					const Vec3 dir(sin(kat)*speed, 0, cos(kat)*speed);
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
						//Vec3 prev = u.pos;
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
								const Int2 new_tile(u.pos / 2);
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
								const Int2 new_tile(u.pos.x * 4, u.pos.z * 4);
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
		Vec3 look_pos2;
		if(look_at == LookAtTarget || look_at == LookAtPoint || (look_at == LookAtWalk && look_pt_valid))
		{
			Vec3 look_pt;
			if(look_at == LookAtTarget)
				look_pt = target_pos;
			else
				look_pt = look_pos;

			// patrz na cel
			float dir = Vec3::LookAtAngle(u.pos, look_pt),
				dif = AngleDiff(u.rot, dir);

			if(NotZero(dif))
			{
				const float rot_speed = u.GetRotationSpeed() * dt;
				const float arc = ShortestArc(u.rot, dir);

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
					u.rot = Clip(u.rot + Sign(arc) * rot_speed);

				u.changed = true;
			}
		}
		else if(look_at == LookAtAngle)
		{
			float dif = AngleDiff(u.rot, look_pos.x);
			if(NotZero(dif))
			{
				const float rot_speed = u.GetRotationSpeed() * dt;
				const float arc = ShortestArc(u.rot, look_pos.x);

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
					u.rot = Clip(u.rot + Sign(arc) * rot_speed);

				u.changed = true;
			}
		}
	}
}

//=================================================================================================
void Game::AI_Shout(LevelContext& ctx, AIController& ai)
{
	Unit& unit = *ai.unit;

	if(!unit.data->sounds->Have(SOUND_SEE_ENEMY))
		return;

	PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_SEE_ENEMY)->sound, 3.f, 20.f);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SHOUT;
		c.unit = &unit;
	}

	// alarm near allies
	for(Unit* u : *ctx.units)
	{
		if(u->to_remove || &unit == u || !u->IsStanding() || u->IsPlayer() || !unit.IsFriend(*u) || u->ai->state == AIController::Fighting
			|| u->ai->alert_target || u->dont_attack)
			continue;

		if(Vec3::Distance(unit.pos, u->pos) <= 20.f && L.CanSee(unit, *u))
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

	if(u.action == A_NONE && (u.mesh_inst->mesh->head.n_groups == 1 || u.weapon_state == WS_TAKEN) && ai.next_attack <= 0.f && u.stamina > 0)
	{
		if(u.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
			PlayAttachedSound(u, u.data->sounds->Random(SOUND_ATTACK)->sound, 1.f, 10.f);
		u.action = A_ATTACK;
		u.attack_id = u.GetRandomAttack();

		bool do_power_attack;
		if(!IS_SET(u.data->flags, F_NO_POWER_ATTACK))
		{
			if(target && target->action == A_BLOCK)
				do_power_attack = (Rand() % 2 == 0);
			else
				do_power_attack = (Rand() % 5 == 0);
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

		float stamina = (w_biegu || do_power_attack) ? 1.5f : 1.f;
		if(u.HaveWeapon())
			stamina *= u.GetWeapon().GetInfo().stamina;
		else
			stamina *= Unit::STAMINA_UNARMED_ATTACK;
		u.RemoveStamina(stamina);

		float speed(do_power_attack ? ai.unit->GetPowerAttackSpeed() : ai.unit->GetAttackSpeed());

		if(u.mesh_inst->mesh->head.n_groups > 1)
		{
			u.mesh_inst->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
			u.mesh_inst->groups[1].speed = speed;
		}
		else
		{
			u.mesh_inst->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 0);
			u.mesh_inst->groups[0].speed = speed;
			u.animation = ANI_PLAY;
		}
		u.animation_state = (do_power_attack ? 0 : 1);
		u.hitted = !target;

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::ATTACK;
			c.unit = &u;
			c.id = (do_power_attack ? AID_PrepareAttack : (w_biegu ? AID_RunningAttack : AID_Attack));
			c.f[1] = speed;
		}
	}
}

//=================================================================================================
void Game::AI_HitReaction(Unit& unit, const Vec3& pos)
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
		ai.timer = Random(10.f, 15.f);
		ai.city_wander = false;
		if(!unit.data->sounds->Have(SOUND_SEE_ENEMY))
			return;

		PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_SEE_ENEMY)->sound, 3.f, 20.f);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SHOUT;
			c.unit = &unit;
		}

		// alarm near allies
		for(Unit* u : *L.local_ctx.units)
		{
			if(u->to_remove || &unit == u || !u->IsStanding() || u->IsPlayer() || !unit.IsFriend(*u) || u->dont_attack)
				continue;

			if((u->ai->state == AIController::Idle || u->ai->state == AIController::SearchEnemy)
				&& Vec3::Distance(unit.pos, u->pos) <= 20.f && L.CanSee(unit, *u))
			{
				AIController* ai2 = u->ai;
				ai2->target_last_pos = pos;
				ai2->state = AIController::SeenEnemy;
				ai2->timer = Random(5.f, 10.f);
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
			float dist = Vec3::Distance(unit.pos, u->pos);
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
		if(L.CanSee(unit, talk_target))
		{
			bool ok = true;
			for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& check_unit = **it2;
				if(&talk_target == &check_unit || &unit == &check_unit)
					continue;

				if(check_unit.IsAlive() && talk_target.IsEnemy(check_unit) && check_unit.IsAI() && !check_unit.dont_attack
					&& Vec3::Distance2d(talk_target.pos, check_unit.pos) < ALERT_RANGE.x && L.CanSee(check_unit, talk_target))
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
			talk_player->StartDialog(&unit, unit.auto_talk_dialog);
		}
	}
	else if(unit.auto_talk == AutoTalkMode::Wait)
	{
		unit.auto_talk = AutoTalkMode::Yes;
		unit.auto_talk_timer = Unit::AUTO_TALK_WAIT;
	}

	near_units.clear();
}
