#include "Pch.h"
#include "Game.h"

#include "Ability.h"
#include "AIController.h"
#include "City.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "InsideLocation.h"
#include "Level.h"
#include "LevelGui.h"
#include "Pathfinding.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Mages.h"
#include "Quest_Scripted.h"
#include "Quest_Tournament.h"
#include "Team.h"

#include <SoundManager.h>

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
	"Wait",
	"Aggro"
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
	DontMove,				// not moving
	MoveAway,				// moving from the point
	MovePoint,				// moving to the point
	KeepDistance,			// keep 8-10 meter distance
	KeepDistanceCheck,		// keep 8-10 distance AND check if enemy is possible to hit
	RunAway					// running away from the point
};

enum RUN_TYPE
{
	Walk,					// always walk
	WalkIfNear,				// walk if target is nearby (closer than 1.5 meter)
	Run,					// always run
	WalkIfNear2				// walk if target is nearby (closer than 3m meter)
};

enum LOOK_AT
{
	DontLook,				// do not look anywhere
	LookAtTarget,			// look at the target
	LookAtWalk,				// look at the walk direction
	LookAtPoint,			// look at the point
	LookAtAngle				// look at the angle (look_pos.x)
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

inline float RandomRot(float base_rot, float random_angle)
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
	static vector<Unit*> close_enemies;

	auto stool = BaseUsable::Get("stool"),
		chair = BaseUsable::Get("chair"),
		throne = BaseUsable::Get("throne"),
		iron_vein = BaseUsable::Get("iron_vein"),
		gold_vein = BaseUsable::Get("gold_vein");
	Quest_Tournament* tournament = quest_mgr->quest_tournament;

	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		AIController& ai = **it;
		Unit& u = *ai.unit;
		if(u.to_remove)
			continue;

		for(Event& event : u.events)
		{
			if(event.type == EVENT_UPDATE)
			{
				ScriptEvent e(EVENT_UPDATE);
				e.on_update.unit = &u;
				event.quest->FireEvent(e);
			}
		}

		if(!u.IsStanding())
		{
			u.TryStandup(dt);
			continue;
		}
		else if(u.HaveEffect(EffectId::Stun) || u.action == A_STAND_UP)
			continue;

		LevelArea& area = *u.area;

		// update time
		u.prev_pos = u.pos;
		ai.timer -= dt;
		ai.next_attack -= dt;
		ai.ignore -= dt;
		ai.pf_timer -= dt;
		ai.morale = Min(ai.morale + dt, u.GetMaxMorale());
		if(u.data->abilities)
		{
			for(int i = 0; i < MAX_ABILITIES; ++i)
				ai.cooldown[i] -= dt;
		}

		if(u.frozen >= FROZEN::YES)
		{
			if(u.frozen == FROZEN::YES)
				u.animation = ANI_STAND;
			continue;
		}

		if(u.action == A_ANIMATION)
		{
			if(Unit* look_target = u.look_target; look_target)
			{
				float dir = Vec3::LookAtAngle(u.pos, look_target->pos);
				if(!Equal(u.rot, dir))
				{
					const float rot_speed = 3.f * dt;
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

		if(u.action == A_DASH)
			continue;

		// standing animation
		if(u.animation != ANI_PLAY && u.animation != ANI_IDLE)
			u.animation = ANI_STAND;

		// search for enemies
		Unit* enemy = nullptr;
		float best_dist = ALERT_RANGE + 0.1f, dist;
		for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
		{
			if((*it2)->to_remove || !(*it2)->IsAlive() || (*it2)->IsInvisible() || !u.IsEnemy(**it2))
				continue;

			dist = Vec3::Distance(u.pos, (*it2)->pos);

			if(dist < best_dist && game_level->CanSee(u, **it2))
			{
				best_dist = dist;
				enemy = *it2;
			}
		}

		// AI escape
		if(enemy && ai.state != AIController::Escape && !IsSet(u.data->flags, F_DONT_ESCAPE) && ai.GetMorale() < 0.f)
		{
			if(u.action == A_BLOCK)
			{
				u.action = A_NONE;
				u.mesh_inst->Deactivate(1);
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::ATTACK;
					c.unit = &u;
					c.id = AID_Cancel;
					c.f[1] = 1.f;
				}
			}
			ai.state = AIController::Escape;
			ai.timer = Random(2.5f, 5.f);
			ai.target = nullptr;
			ai.st.escape.room = nullptr;
			ai.ignore = 0.f;
			ai.target_last_pos = enemy->pos;
			ai.in_combat = true;
		}

		MOVE_TYPE move_type = DontMove;
		RUN_TYPE run_type = Run;
		LOOK_AT look_at = DontLook;
		Vec3 target_pos, look_pos;
		bool look_pt_valid = false;
		const void* path_obj_ignore = nullptr;
		const Unit* path_unit_ignore = nullptr;
		bool try_phase = false;

		// casting of non combat spells
		if(u.data->abilities && u.data->abilities->have_non_combat && u.action == A_NONE && ai.state != AIController::Escape && ai.state != AIController::Cast
			&& u.busy == Unit::Busy_No)
		{
			for(int i = 0; i < MAX_ABILITIES; ++i)
			{
				Ability* ability = u.data->abilities->ability[i];
				if(ability && IsSet(ability->flags, Ability::NonCombat)
					&& u.level >= u.data->abilities->level[i] && ai.cooldown[i] <= 0.f
					&& u.mp >= ability->mana
					&& u.stamina >= ability->stamina)
				{
					float spell_range = ability->move_range,
						best_prio = -999.f, dist;
					Unit* spell_target = nullptr;

					// if near enemies, cast only on near targets
					if(best_dist < 3.f)
						spell_range = ability->range;

					if(ability->effect == Ability::Raise)
					{
						// raise undead spell
						for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
						{
							Unit& target = **it2;
							if(!target.to_remove && target.live_state == Unit::DEAD && !u.IsEnemy(target) && IsSet(target.data->flags, F_UNDEAD)
								&& (dist = Vec3::Distance(u.pos, target.pos)) < spell_range && target.in_arena == u.in_arena && game_level->CanSee(u, target))
							{
								float prio = target.hpmax - dist * 10;
								if(prio > best_prio)
								{
									spell_target = &target;
									best_prio = prio;
								}
							}
						}
					}
					else if(ability->effect == Ability::Heal)
					{
						// healing spell
						for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
						{
							Unit& target = **it2;
							if(!target.to_remove && !u.IsEnemy(target) && !IsSet(target.data->flags, F_UNDEAD)
								&& !IsSet(target.data->flags, F2_CONSTRUCT) && target.GetHpp() <= 0.8f
								&& (dist = Vec3::Distance(u.pos, target.pos)) < spell_range && target.in_arena == u.in_arena
								&& (target.IsAlive() || target.IsTeamMember()) && game_level->CanSee(u, target))
							{
								float prio = target.hpmax - target.hp;
								if(&target == &u)
									prio *= 1.5f;
								if(!target.IsAlive())
									prio /= 10;
								prio -= dist * 10;
								if(prio > best_prio)
								{
									spell_target = &target;
									best_prio = prio;
								}
							}
						}
					}

					if(spell_target)
					{
						ai.state = AIController::Cast;
						ai.target = spell_target;
						ai.st.cast.ability = ability;
						break;
					}
				}
			}
		}

		// AI states
		bool repeat = true;
		while(repeat)
		{
			repeat = false;

			switch(ai.state)
			{
			//===================================================================================================================
			// no enemies nearby
			case AIController::Idle:
				{
					if(u.usable == nullptr)
					{
						if(Unit* alert_target = ai.alert_target)
						{
							// someone else alert him with enemy alert shout
							u.talking = false;
							u.mesh_inst->need_update = true;
							ai.in_combat = true;
							ai.target = alert_target;
							ai.target_last_pos = ai.alert_target_pos;
							ai.alert_target = nullptr;
							ai.state = AIController::Fighting;
							ai.timer = 0.f;
							ai.city_wander = false;
							ai.change_ai_mode = true;
							repeat = true;
							if(IsSet(u.data->flags2, F2_YELL))
								ai.Shout();
							if(IsSet(u.data->flags2, F2_BOSS))
								game_level->StartBossFight(u);
							break;
						}

						if(enemy)
						{
							// enemy noticed - start the fight
							u.talking = false;
							u.mesh_inst->need_update = true;
							ai.in_combat = true;
							ai.target = enemy;
							ai.target_last_pos = enemy->pos;
							ai.city_wander = false;
							ai.change_ai_mode = true;
							if(IsSet(u.data->flags, F_AGGRO))
							{
								ai.state = AIController::Aggro;
								ai.timer = AGGRO_TIMER;
								ai.next_attack = -1.f;
							}
							else
							{
								ai.state = AIController::Fighting;
								ai.timer = 0.f;
								ai.Shout();
								if(IsSet(u.data->flags2, F2_BOSS))
									game_level->StartBossFight(u);
							}
							repeat = true;
							break;
						}
					}
					else if(enemy || ai.alert_target)
					{
						if(u.action != A_USE_USABLE || u.animation_state == AS_USE_USABLE_MOVE_TO_ENDPOINT)
							break;
						// interrupt object usage
						ai.st.idle.action = AIController::Idle_None;
						ai.timer = Random(2.f, 5.f);
						ai.city_wander = false;
						u.StopUsingUsable();
					}

					bool hide_weapon = true;
					if(u.action != A_NONE)
						hide_weapon = false;
					else if(ai.st.idle.action == AIController::Idle_TrainCombat)
					{
						if(u.weapon_taken == W_ONE_HANDED)
							hide_weapon = false;
					}
					else if(ai.st.idle.action == AIController::Idle_TrainBow)
					{
						if(u.weapon_taken == W_BOW)
							hide_weapon = false;
					}
					if(hide_weapon)
					{
						u.HideWeapon();
						if(!u.look_target)
							ai.CheckPotion(true);
					}

					// fix bug of unkown origin. occurs after arena fight, affecting loosers
					if(u.weapon_state == WeaponState::Hiding && u.action == A_NONE)
					{
						u.SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
						ai.potion = -1;
						ReportError(8, Format("Unit %s was hiding weapon without action.", u.GetRealName()));
					}

					ai.loc_timer -= dt;
					if(ai.in_combat)
						ai.in_combat = false;

					// temporary fix - invalid usable user
					if(u.usable && u.usable->user != u)
					{
						Unit* user = u.usable->user;
						ReportError(2, Format("Invalid usable user: %s is using %s but the user is %s.", u.GetRealName(), u.usable->base->id.c_str(),
							user ? user->GetRealName() : "nullptr"));
						u.usable = nullptr;
					}

					// temporary fix - unit blocks in idle
					if(u.action == A_BLOCK)
					{
						u.action = A_NONE;
						u.mesh_inst->Deactivate(1);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = &u;
							c.id = AID_Cancel;
							c.f[1] = 1.f;
						}
						ReportError(3, Format("Unit %s blocks in idle.", u.GetRealName()));
					}

					if(Unit* look_target = u.look_target; look_target && !u.usable)
					{
						// unit looking at talker in dialog
						look_at = LookAtPoint;
						look_pos = look_target->pos;
						u.timer = Random(1.f, 2.f);
						continue;
					}

					if(UnitOrder order = u.GetOrder(); order != ORDER_NONE && order != ORDER_AUTO_TALK && u.order->timer > 0.f)
					{
						u.order->timer -= dt;
						if(u.order->timer <= 0.f)
							u.OrderNext();
					}

					UnitOrder order = u.GetOrder();
					Unit* order_unit = u.order ? u.order->unit : nullptr;
					if(u.assist)
					{
						order = ORDER_FOLLOW;
						order_unit = team->GetLeader();
					}

					bool use_idle = true;
					switch(order)
					{
					case ORDER_FOLLOW:
						if(!order_unit)
						{
							u.OrderNext();
							break;
						}
						if(order_unit->in_arena == -1 && u.busy != Unit::Busy_Tournament)
						{
							dist = Vec3::Distance(u.pos, order_unit->pos);
							if(dist >= (u.assist ? 4.f : 2.f))
							{
								// follow the target
								if(u.usable)
								{
									if(u.busy != Unit::Busy_Talking && (u.action != A_USE_USABLE || u.animation_state != AS_USE_USABLE_MOVE_TO_ENDPOINT))
									{
										u.StopUsingUsable();
										ai.st.idle.action = AIController::Idle_None;
										ai.timer = Random(1.f, 2.f);
										ai.city_wander = false;
									}
								}
								else if(order_unit->area != u.area)
								{
									// target is in another area
									ai.st.idle.action = AIController::Idle_RunRegion;
									ai.st.idle.region.exit = false;
									ai.timer = Random(15.f, 30.f);

									if(u.area->area_type == LevelArea::Type::Outside)
									{
										// target is in the building but hero is not - go to the entrance
										ai.st.idle.region.area = order_unit->area;
										ai.st.idle.region.pos = static_cast<InsideBuilding*>(order_unit->area)->enter_region.Midpoint().XZ();
									}
									else
									{
										// target is not in the building but the hero is - leave the building
										ai.st.idle.region.area = game_level->local_area;
										ai.st.idle.region.pos = static_cast<InsideBuilding*>(u.area)->exit_region.Midpoint().XZ();
									}

									if(u.IsHero())
										try_phase = true;
								}
								else
								{
									// move to target
									if(dist > 8.f)
										look_at = LookAtWalk;
									else
									{
										look_at = LookAtPoint;
										look_pos = order_unit->pos;
									}
									move_type = MovePoint;
									target_pos = order_unit->pos;
									run_type = WalkIfNear2;
									ai.st.idle.action = AIController::Idle_None;
									ai.city_wander = false;
									ai.timer = Random(2.f, 5.f);
									path_unit_ignore = order_unit;
									if(u.IsHero())
										try_phase = true;
									use_idle = false;
								}
							}
							else
							{
								// move away to not block
								if(u.usable)
								{
									if(u.busy != Unit::Busy_Talking && (u.action != A_USE_USABLE || u.animation_state != AS_USE_USABLE_MOVE_TO_ENDPOINT))
									{
										u.StopUsingUsable();
										ai.st.idle.action = AIController::Idle_None;
										ai.timer = Random(1.f, 2.f);
										ai.city_wander = false;
										use_idle = false;
									}
								}
								else
								{
									for(Unit& unit : team->members)
									{
										if(&unit != &u && Vec3::Distance(unit.pos, u.pos) < 1.f)
										{
											look_at = LookAtPoint;
											look_pos = unit.pos;
											move_type = MoveAway;
											target_pos = unit.pos;
											run_type = Walk;
											ai.st.idle.action = AIController::Idle_None;
											ai.timer = Random(2.f, 5.f);
											ai.city_wander = false;
											use_idle = false;
											break;
										}
									}
								}
							}
						}
						break;
					case ORDER_LEAVE:
						if(u.usable)
						{
							if(u.busy != Unit::Busy_Talking && (u.action != A_USE_USABLE || u.animation_state != AS_USE_USABLE_MOVE_TO_ENDPOINT))
							{
								u.StopUsingUsable();
								ai.st.idle.action = AIController::Idle_None;
								ai.timer = Random(1.f, 2.f);
								ai.city_wander = true;
							}
						}
						else if(ai.timer <= 0.f)
						{
							ai.st.idle.action = AIController::Idle_MoveRegion;
							ai.timer = Random(30.f, 40.f);
							ai.st.idle.region.area = game_level->local_area;
							ai.st.idle.region.exit = false;
							ai.st.idle.region.pos = u.area->area_type == LevelArea::Type::Building ?
								static_cast<InsideBuilding*>(u.area)->exit_region.Midpoint().XZ() :
								game_level->GetExitPos(u);
						}
						break;
					case ORDER_LOOK_AT:
						use_idle = false;
						look_at = LookAtPoint;
						look_pos = u.order->pos;
						break;
					case ORDER_MOVE:
						use_idle = false;
						if(Vec3::Distance2d(u.pos, u.order->pos) < u.order->range)
							u.OrderNext();
						move_type = MovePoint;
						target_pos = u.order->pos;
						look_at = LookAtWalk;
						switch(u.order->move_type)
						{
						default:
						case MOVE_WALK:
							run_type = Walk;
							break;
						case MOVE_RUN:
							run_type = Run;
							break;
						case MOVE_RUN_WHEN_NEAR_TEAM:
							{
								float dist;
								Unit* near_team = team->GetNearestTeamMember(u.pos, &dist);
								if(dist < 4.f)
									run_type = Run;
								else if(dist < 5.f)
									run_type = Walk;
								else
								{
									move_type = DontMove;
									look_at = LookAtPoint;
									look_pos = near_team->pos;
								}
							}
							break;
						}
						break;
					case ORDER_GOTO_INN:
						if(!(u.IsHero() && tournament->IsGenerated()))
						{
							if(u.usable)
							{
								if(u.busy != Unit::Busy_Talking && (u.action != A_USE_USABLE || u.animation_state != AS_USE_USABLE_MOVE_TO_ENDPOINT))
								{
									u.StopUsingUsable();
									ai.st.idle.action = AIController::Idle_None;
									ai.timer = Random(1.f, 2.f);
									use_idle = false;
								}
							}
							else
							{
								InsideBuilding* inn = game_level->city_ctx->FindInn();
								if(u.area->area_type == LevelArea::Type::Outside)
								{
									// go to inn
									if(ai.timer <= 0.f)
									{
										ai.st.idle.action = AIController::Idle_MoveRegion;
										ai.st.idle.region.area = inn;
										ai.st.idle.region.exit = false;
										ai.st.idle.region.pos = inn->enter_region.Midpoint().XZ();
										ai.timer = Random(30.f, 40.f);
										ai.city_wander = true;
									}
								}
								else
								{
									if(u.area == inn)
									{
										// is inside inn - move to random point
										u.OrderNext();
										ai.timer = Random(5.f, 7.5f);
										ai.st.idle.action = AIController::Idle_Move;
										ai.st.idle.pos = (Rand() % 5 == 0 ? inn->region2.Midpoint() : inn->region1.Midpoint()).XZ();
									}
									else
									{
										// is not inside inn - go outside
										ai.timer = Random(15.f, 30.f);
										ai.st.idle.action = AIController::Idle_MoveRegion;
										ai.st.idle.region.area = game_level->local_area;
										ai.st.idle.region.exit = false;
										ai.st.idle.region.pos = static_cast<InsideBuilding*>(u.area)->exit_region.Midpoint().XZ();
									}
								}
							}
						}
						break;
					case ORDER_GUARD:
						// remove dont_attack bit if leader removed it
						if(order_unit)
						{
							if(u.dont_attack && !order_unit->dont_attack)
								u.SetDontAttack(false);
						}
						else
							u.OrderNext();
						break;
					case ORDER_AUTO_TALK:
						u.CheckAutoTalk(dt);
						break;
					}

					if(!use_idle || (u.action == A_USE_USABLE && !Any(u.animation_state, AS_USE_USABLE_USING, AS_USE_USABLE_USING_SOUND)))
						break;

					if(ai.timer <= 0.f && Any(u.action, A_NONE, A_USE_USABLE))
					{
						if(Any(ai.st.idle.action, AIController::Idle_TrainCombat, AIController::Idle_TrainBow) && u.weapon_state != WeaponState::Hidden)
						{
							if(u.action == A_NONE)
							{
								u.HideWeapon();
								ai.in_combat = false;
							}
						}
						else if(IsSet(u.data->flags2, F2_XAR))
						{
							// search for altar
							BaseObject* base_obj = BaseObject::Get("bloody_altar");
							Object* obj = area.FindObject(base_obj);

							if(obj)
							{
								// pray at the altar
								ai.st.idle.action = AIController::Idle_Pray;
								ai.st.idle.pos = obj->pos;
								ai.timer = 120.f;
								u.animation = ANI_KNEELS;
							}
							else
							{
								// no altar - stay in place
								ai.st.idle.action = AIController::Idle_Animation;
								ai.st.idle.rot = u.rot;
								ai.timer = 120.f;
							}
						}
						else if(ai.st.idle.action == AIController::Idle_Look)
						{
							// stop looking at the person, go look somewhere else
							ai.st.idle.action = AIController::Idle_Rot;
							if(IsSet(u.data->flags, F_AI_GUARD) && AngleDiff(u.rot, ai.start_rot) > PI / 4)
								ai.st.idle.rot = ai.start_rot;
							else if(IsSet(u.data->flags2, F2_LIMITED_ROT))
								ai.st.idle.rot = RandomRot(ai.start_rot, PI / 4);
							else
								ai.st.idle.rot = Clip(Vec3::LookAtAngle(u.pos, ai.st.idle.pos) + Random(-PI / 2, PI / 2));
							ai.timer = Random(2.f, 5.f);
						}
						else if(ai.st.idle.action == AIController::Idle_Chat)
						{
							u.talking = false;
							u.mesh_inst->need_update = true;
							ai.st.idle.action = AIController::Idle_None;
						}
						else if(ai.st.idle.action == AIController::Idle_Use)
						{
							if(u.usable->base == stool && u.area->area_type == LevelArea::Type::Building)
							{
								int what;
								if(u.IsDrunkman())
									what = Rand() % 3;
								else
									what = Rand() % 2 + 1;
								switch(what)
								{
								case 0: // drink
									u.ConsumeItem(ItemList::GetItem("drink")->ToConsumable());
									ai.timer = Random(10.f, 15.f);
									break;
								case 1: // eat
									u.ConsumeItem(ItemList::GetItem("normal_food")->ToConsumable());
									ai.timer = Random(10.f, 15.f);
									break;
								case 2: // stop sitting
									u.StopUsingUsable();
									ai.st.idle.action = AIController::Idle_None;
									ai.timer = Random(2.5f, 5.f);
									break;
								}
							}
							else
							{
								u.StopUsingUsable();
								ai.st.idle.action = AIController::Idle_None;
								ai.timer = Random(2.5f, 5.f);
							}
						}
						else if(ai.st.idle.action == AIController::Idle_WalkUseEat)
						{
							if(u.usable)
							{
								int what;
								if(u.IsDrunkman())
									what = Rand() % 2;
								else
									what = 1;
								switch(what)
								{
								case 0: // drink
									u.ConsumeItem(ItemList::GetItem("drink")->ToConsumable());
									ai.timer = Random(10.f, 15.f);
									break;
								case 1: // eat
									u.ConsumeItem(ItemList::GetItem("normal_food")->ToConsumable());
									ai.timer = Random(10.f, 15.f);
									break;
								}
								ai.st.idle.action = AIController::Idle_Use;
							}
							else
							{
								ai.st.idle.action = AIController::Idle_None;
								ai.timer = Random(2.f, 3.f);
							}
						}
						else if(ai.CanWander())
						{
							if(u.IsHero())
							{
								if(u.area->area_type == LevelArea::Type::Outside)
								{
									// unit is outside
									int where = Rand() % (IsSet(game_level->city_ctx->flags, City::HaveTrainingGrounds) ? 3 : 2);
									if(where == 0)
									{
										// go to random position
										ai.loc_timer = ai.timer = Random(30.f, 120.f);
										ai.st.idle.action = AIController::Idle_Move;
										ai.st.idle.pos = game_level->city_ctx->buildings[Rand() % game_level->city_ctx->buildings.size()].walk_pt
											+ Vec3::Random(Vec3(-1.f, 0, -1), Vec3(1, 0, 1));
									}
									else if(where == 1)
									{
										// go to inn
										InsideBuilding* inn = game_level->city_ctx->FindInn();
										ai.loc_timer = ai.timer = Random(75.f, 150.f);
										ai.st.idle.action = AIController::Idle_MoveRegion;
										ai.st.idle.region.area = inn;
										ai.st.idle.region.exit = false;
										ai.st.idle.region.pos = inn->enter_region.Midpoint().XZ();
									}
									else if(where == 2)
									{
										// go to training grounds
										ai.loc_timer = ai.timer = Random(75.f, 150.f);
										ai.st.idle.action = AIController::Idle_Move;
										ai.st.idle.pos = game_level->city_ctx->FindBuilding(BuildingGroup::BG_TRAINING_GROUNDS)->walk_pt
											+ Vec3::Random(Vec3(-1.f, 0, -1), Vec3(1, 0, 1));
									}
								}
								else
								{
									// leave building
									ai.loc_timer = ai.timer = Random(15.f, 30.f);
									ai.st.idle.action = AIController::Idle_MoveRegion;
									ai.st.idle.region.area = game_level->local_area;
									ai.st.idle.region.exit = false;
									ai.st.idle.region.pos = static_cast<InsideBuilding*>(u.area)->exit_region.Midpoint().XZ();
								}
								ai.city_wander = true;
							}
							else
							{
								// go near random building
								ai.loc_timer = ai.timer = Random(30.f, 120.f);
								ai.st.idle.action = AIController::Idle_Move;
								ai.st.idle.pos = game_level->city_ctx->buildings[Rand() % game_level->city_ctx->buildings.size()].walk_pt
									+ Vec3::Random(Vec3(-1.f, 0, -1), Vec3(1, 0, 1));
								ai.city_wander = true;
							}
						}
						else if(order == ORDER_GUARD && Vec3::Distance(u.pos, order_unit->pos) > 5.f)
						{
							// walk to guard target when too far
							ai.timer = Random(2.f, 4.f);
							ai.st.idle.action = AIController::Idle_WalkNearUnit;
							ai.st.idle.unit = order_unit;
							ai.city_wander = false;
						}
						else if(IsSet(u.data->flags3, F3_MINER) && Rand() % 2 == 0)
						{
							// check if unit have required item
							const Item* req_item = iron_vein->item;
							if(req_item && !u.HaveItem(req_item) && u.GetEquippedItem(SLOT_WEAPON) != req_item)
								goto normal_idle_action;
							// find closest ore vein
							Usable* usable = nullptr;
							float range = 20.1f;
							for(vector<Usable*>::iterator it2 = area.usables.begin(), end2 = area.usables.end(); it2 != end2; ++it2)
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
								ai.st.idle.action = AIController::Idle_WalkUse;
								ai.st.idle.usable = usable;
								ai.timer = Random(5.f, 10.f);
							}
							else
								goto normal_idle_action;
						}
						else if(IsSet(u.data->flags3, F3_DRUNK_MAGE)
							&& quest_mgr->quest_mages2->mages_state >= Quest_Mages2::State::OldMageJoined
							&& quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::MageCured
							&& Rand() % 3 == 0)
						{
							// drink something
							u.ConsumeItem(ItemList::GetItem("drink")->ToConsumable(), true);
							ai.st.idle.action = AIController::Idle_None;
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
									ai.st.idle.action = AIController::Idle_WalkNearUnit;
									ai.st.idle.unit = tournament->GetMaster();
								}
								else
								{
									ai.timer = Random(4.f, 8.f);
									ai.st.idle.action = AIController::Idle_Move;
									ai.st.idle.pos = tournament->GetMaster()->pos + Vec3::Random(Vec3(-10, 0, -10), Vec3(10, 0, 10));
								}
							}
							else if(IsSet(u.data->flags2, F2_SIT_ON_THRONE) && !u.IsFollower())
								what = AI_USE;
							else if(u.data->type == UNIT_TYPE::HUMAN)
							{
								if(!IsSet(u.data->flags, F_AI_GUARD))
								{
									if(IsSet(u.data->flags2, F2_AI_TRAIN) && Rand() % 5 == 0)
									{
										static vector<Object*> do_cw;
										BaseObject* melee_target = BaseObject::Get("melee_target"),
											*bow_target = BaseObject::Get("bow_target");
										for(vector<Object*>::iterator it2 = area.objects.begin(), end2 = area.objects.end(); it2 != end2; ++it2)
										{
											Object& obj = **it2;
											if((obj.base == melee_target || (obj.base == bow_target && u.HaveBow())) && Vec3::Distance(obj.pos, u.pos) < 10.f)
												do_cw.push_back(&obj);
										}
										if(!do_cw.empty())
										{
											Object& o = *do_cw[Rand() % do_cw.size()];
											if(o.base == melee_target)
											{
												ai.timer = Random(10.f, 30.f);
												ai.st.idle.action = AIController::Idle_TrainCombat;
												ai.st.idle.pos = o.pos;
											}
											else
											{
												ai.timer = Random(15.f, 35.f);
												ai.st.idle.action = AIController::Idle_TrainBow;
												ai.st.idle.obj.pos = o.pos;
												ai.st.idle.obj.rot = o.rot.y;
												ai.st.idle.obj.ptr = &o;
											}
											do_cw.clear();
											what = AI_NONE;
										}
									}
									if(what != AI_NONE)
									{
										if(IsSet(u.data->flags3, F3_DONT_EAT))
											what = Rand() % 6;
										else
											what = Rand() % 7;
									}
								}
								else // guard (don't move/use objects)
								{
									if(IsSet(u.data->flags3, F3_DONT_EAT))
										what = Rand() % 5;
									else
										what = Rand() % 7;
									if(what == AI_MOVE)
										what = Rand() % 3;
								}
							}
							else // not human
							{
								if(IsSet(u.data->flags3, F3_DONT_EAT))
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
								if(IsSet(u.data->flags2, F2_DONT_TALK))
									what = Rand() % 3;
								else if(IsSet(u.data->flags3, F3_TALK_AT_COMPETITION)
									&& (quest_mgr->quest_contest->state >= Quest_Contest::CONTEST_STARTING
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
								{
									LocalVector<Usable*> uses;
									for(vector<Usable*>::iterator it2 = area.usables.begin(), end2 = area.usables.end(); it2 != end2; ++it2)
									{
										Usable& use = **it2;
										if(!use.user && (use.base != throne || IsSet(u.data->flags2, F2_SIT_ON_THRONE))
											&& Vec3::Distance(use.pos, u.pos) < 10.f && !use.base->IsContainer()
											&& game_level->CanSee(area, use.pos, u.pos))
										{
											const Item* needed_item = use.base->item;
											if(!needed_item || u.HaveItem(needed_item) || u.GetEquippedItem(SLOT_WEAPON) == needed_item)
												uses.push_back(*it2);
										}
									}
									if(!uses.empty())
									{
										ai.st.idle.action = AIController::Idle_WalkUse;
										ai.st.idle.usable = uses[Rand() % uses.size()];
										ai.timer = Random(3.f, 6.f);
										if(ai.st.idle.usable->base == stool && Rand() % 3 == 0)
											ai.st.idle.action = AIController::Idle_WalkUseEat;
										uses.clear();
										break;
									}
								}
							// nothing to use, play animation
							case AI_ANIMATION:
								{
									int id = Rand() % u.data->idles->anims.size();
									ai.timer = Random(2.f, 5.f);
									ai.st.idle.action = AIController::Idle_Animation;
									u.mesh_inst->Play(u.data->idles->anims[id].c_str(), PLAY_ONCE, 0);
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
									ai.st.idle.action = AIController::Idle_Look;
									ai.st.idle.unit = tournament->GetMaster();
									break;
								}
								else
								{
									close_enemies.clear();
									for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
									{
										if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->IsInvisible() || *it2 == &u)
											continue;

										if(Vec3::Distance(u.pos, (*it2)->pos) < 10.f && game_level->CanSee(u, **it2))
											close_enemies.push_back(*it2);
									}
									if(!close_enemies.empty())
									{
										ai.timer = Random(1.5f, 2.5f);
										ai.st.idle.action = AIController::Idle_Look;
										ai.st.idle.unit = close_enemies[Rand() % close_enemies.size()];
										break;
									}
								}
								// no close units, rotate
							case AI_ROTATE:
								ai.timer = Random(2.f, 5.f);
								ai.st.idle.action = AIController::Idle_Rot;
								if(IsSet(u.data->flags, F_AI_GUARD) && AngleDiff(u.rot, ai.start_rot) > PI / 4)
									ai.st.idle.rot = ai.start_rot;
								else if(IsSet(u.data->flags2, F2_LIMITED_ROT))
									ai.st.idle.rot = RandomRot(ai.start_rot, PI / 4);
								else
									ai.st.idle.rot = Random(MAX_ANGLE);
								break;
							case AI_TALK:
								{
									const float d = ((IsSet(u.data->flags, F_AI_GUARD) || IsSet(u.data->flags, F_AI_STAY)) ? 1.5f : 10.f);
									close_enemies.clear();
									for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
									{
										if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->IsInvisible() || *it2 == &u)
											continue;

										if(Vec3::Distance(u.pos, (*it2)->pos) < d && game_level->CanSee(u, **it2))
											close_enemies.push_back(*it2);
									}
									if(!close_enemies.empty())
									{
										Unit* target = close_enemies[Rand() % close_enemies.size()];
										ai.timer = Random(3.f, 6.f);
										ai.st.idle.action = AIController::Idle_WalkTo;
										ai.st.idle.unit = target;
										ai.target_last_pos = target->pos;
										break;
									}
								}
								// no units nearby - move random
								if(IsSet(u.data->flags, F_AI_GUARD))
									break;
							case AI_MOVE:
								if(IsSet(u.data->flags, F_AI_GUARD))
									break;
								ai.timer = Random(3.f, 6.f);
								ai.st.idle.action = AIController::Idle_Move;
								ai.city_wander = false;
								if(IsSet(u.data->flags, F_AI_STAY))
								{
									if(Vec3::Distance(u.pos, ai.start_pos) > 2.f)
										ai.st.idle.pos = ai.start_pos;
									else
										ai.st.idle.pos = u.pos + Vec3::Random(Vec3(-2.f, 0, -2.f), Vec3(2.f, 0, 2.f));
								}
								else
									ai.st.idle.pos = u.pos + Vec3::Random(Vec3(-5.f, 0, -5.f), Vec3(5.f, 0, 5.f));
								if(game_level->city_ctx && !game_level->city_ctx->IsInsideCity(ai.st.idle.pos))
								{
									ai.timer = Random(2.f, 4.f);
									ai.st.idle.action = AIController::Idle_None;
								}
								else if(!game_level->location->outside)
								{
									InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
									if(!inside->GetLevelData().IsValidWalkPos(ai.st.idle.pos, u.GetUnitRadius()))
									{
										ai.timer = Random(2.f, 4.f);
										ai.st.idle.action = AIController::Idle_None;
									}
								}
								break;
							case AI_EAT:
								ai.timer = Random(3.f, 5.f);
								ai.st.idle.action = AIController::Idle_None;
								u.ConsumeItem(ItemList::GetItem(IsSet(u.data->flags3, F3_ORC_FOOD) ? "orc_food" : "normal_food")->ToConsumable());
								break;
							default:
								assert(0);
								break;
							}
						}
					}
					else
					{
						switch(ai.st.idle.action)
						{
						case AIController::Idle_None:
						case AIController::Idle_Animation:
							break;
						case AIController::Idle_Rot:
							if(Equal(u.rot, ai.st.idle.rot))
								ai.st.idle.action = AIController::Idle_None;
							else
							{
								look_at = LookAtAngle;
								look_pos.x = ai.st.idle.rot;
							}
							break;
						case AIController::Idle_Move:
							if(Vec3::Distance2d(u.pos, ai.st.idle.pos) < u.GetUnitRadius() * 2)
							{
								if(ai.city_wander)
								{
									ai.city_wander = false;
									ai.timer = 0.f;
								}
								ai.st.idle.action = AIController::Idle_None;
							}
							else
							{
								move_type = MovePoint;
								target_pos = ai.st.idle.pos;
								look_at = LookAtWalk;
								run_type = Walk;
							}
							break;
						case AIController::Idle_Look:
							if(Unit* target = ai.st.idle.unit; target && Vec3::Distance2d(u.pos, target->pos) <= 10.f)
							{
								look_at = LookAtPoint;
								look_pos = target->pos;
							}
							else
							{
								// stop looking
								ai.st.idle.action = AIController::Idle_Rot;
								if(IsSet(u.data->flags, F_AI_GUARD) && AngleDiff(u.rot, ai.start_rot) > PI / 4)
									ai.st.idle.rot = ai.start_rot;
								else if(IsSet(u.data->flags2, F2_LIMITED_ROT))
									ai.st.idle.rot = RandomRot(ai.start_rot, PI / 4);
								else
									ai.st.idle.rot = Random(MAX_ANGLE);
								ai.timer = Random(2.f, 5.f);
							}
							break;
						case AIController::Idle_WalkTo:
							if(Unit* target = ai.st.idle.unit; target && target->IsStanding())
							{
								if(game_level->CanSee(u, *target))
								{
									if(Vec3::Distance2d(u.pos, target->pos) < 1.5f)
									{
										// stop approaching, start talking
										ai.st.idle.action = AIController::Idle_Chat;
										ai.timer = Random(2.f, 3.f);

										cstring msg = game->idle_context.GetIdleText(u);

										int ani = 0;
										game_gui->level_gui->AddSpeechBubble(&u, msg);
										if(u.data->type == UNIT_TYPE::HUMAN && Rand() % 3 != 0)
										{
											ani = Rand() % 2 + 1;
											u.mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
											u.animation = ANI_PLAY;
											u.action = A_ANIMATION;
										}

										if(target->IsAI())
										{
											AIController& ai2 = *target->ai;
											if(ai2.state == AIController::Idle
												&& OR3_EQ(ai2.st.idle.action, AIController::Idle_None, AIController::Idle_Rot, AIController::Idle_Look))
											{
												// look at the person in response to his looking
												ai2.st.idle.action = AIController::Idle_Chat;
												ai2.timer = ai.timer + Random(1.f);
												ai2.st.idle.unit = u;
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
											net->net_strs.push_back(c.str);
										}
									}
									else
									{
										if(IsSet(u.data->flags, F_AI_GUARD))
											ai.st.idle.action = AIController::Idle_None;
										else
										{
											ai.target_last_pos = target->pos;
											move_type = MovePoint;
											run_type = Walk;
											look_at = LookAtWalk;
											target_pos = ai.target_last_pos;
											path_unit_ignore = target;
										}
									}
								}
								else
								{
									if(Vec3::Distance2d(u.pos, ai.target_last_pos) < 1.5f)
										ai.st.idle.action = AIController::Idle_None;
									else
									{
										if(IsSet(u.data->flags, F_AI_GUARD))
											ai.st.idle.action = AIController::Idle_None;
										else
										{
											move_type = MovePoint;
											run_type = Walk;
											look_at = LookAtWalk;
											target_pos = ai.target_last_pos;
											path_unit_ignore = target;
										}
									}
								}
							}
							else
								ai.st.idle.action = AIController::Idle_None;
							break;
						case AIController::Idle_WalkNearUnit:
							if(Unit* target = ai.st.idle.unit; target && target->IsStanding() && Vec3::Distance2d(u.pos, target->pos) >= 4.f)
							{
								ai.target_last_pos = target->pos;
								move_type = MovePoint;
								run_type = Walk;
								look_at = LookAtWalk;
								target_pos = ai.target_last_pos;
								path_unit_ignore = target;
							}
							else
								ai.st.idle.action = AIController::Idle_None;
							break;
						case AIController::Idle_Chat:
							if(Unit* target = ai.st.idle.unit)
							{
								look_at = LookAtPoint;
								look_pos = target->pos;
							}
							else
								ai.st.idle.action = AIController::Idle_None;
							break;
						case AIController::Idle_WalkUse:
						case AIController::Idle_WalkUseEat:
							{
								Usable& use = *ai.st.idle.usable;
								if(use.user || u.frozen != FROZEN::NO)
									ai.st.idle.action = AIController::Idle_None;
								else if(Vec3::Distance2d(u.pos, use.pos) < PICKUP_RANGE)
								{
									if(AngleDiff(Clip(u.rot + PI / 2), Clip(-Vec3::Angle2d(u.pos, ai.st.idle.usable->pos))) < PI / 4)
									{
										BaseUsable& base = *use.base;
										const Item* needed_item = base.item;
										if(!needed_item || u.HaveItem(needed_item) || u.GetEquippedItem(SLOT_WEAPON) == needed_item)
										{
											u.action = A_USE_USABLE;
											u.animation = ANI_PLAY;
											bool read_papers = false;
											if(use.base == chair && IsSet(u.data->flags, F_AI_CLERK))
											{
												read_papers = true;
												u.mesh_inst->Play("czyta_papiery", PLAY_PRIO3, 0);
											}
											else
												u.mesh_inst->Play(base.anim.c_str(), PLAY_PRIO1, 0);
											u.UseUsable(&use);
											u.target_pos = u.pos;
											u.target_pos2 = use.pos;
											if(use.base->limit_rot == 4)
												u.target_pos2 -= Vec3(sin(use.rot) * 1.5f, 0, cos(use.rot) * 1.5f);
											u.timer = 0.f;
											u.animation_state = AS_USE_USABLE_MOVE_TO_OBJECT;
											u.act.use_usable.rot = Vec3::LookAtAngle(u.pos, u.usable->pos);
											u.used_item = needed_item;
											if(ai.st.idle.action == AIController::Idle_WalkUseEat)
												ai.timer = -1.f;
											else
											{
												ai.st.idle.action = AIController::Idle_Use;
												if(u.usable->base == stool && u.area->area_type == LevelArea::Type::Building && u.IsDrunkman())
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
												c.id = use.id;
												c.count = (read_papers ? USE_USABLE_START_SPECIAL : USE_USABLE_START);
											}
										}
										else
											ai.st.idle.action = AIController::Idle_None;
									}
									else
									{
										// rotate towards usable object
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
							if(Vec3::Distance2d(u.pos, ai.st.idle.pos) < u.GetUnitRadius() * 2 + 0.25f)
							{
								u.TakeWeapon(W_ONE_HANDED);
								if(u.GetStaminap() >= 0.25f)
									ai.DoAttack(nullptr, false);
								ai.in_combat = true;
								look_at = LookAtPoint;
								look_pos = ai.st.idle.pos;
							}
							else
							{
								move_type = MovePoint;
								target_pos = ai.st.idle.pos;
								look_at = LookAtWalk;
								run_type = Walk;
							}
							break;
						case AIController::Idle_TrainBow:
							{
								Vec3 pt = ai.st.idle.obj.pos;
								pt -= Vec3(sin(ai.st.idle.obj.rot) * 5, 0, cos(ai.st.idle.obj.rot) * 5);
								if(Vec3::Distance2d(u.pos, pt) < u.GetUnitRadius() * 2)
								{
									u.TakeWeapon(W_BOW);
									float dir = Vec3::LookAtAngle(u.pos, ai.st.idle.obj.pos);
									if(AngleDiff(u.rot, dir) < PI / 4 && u.action == A_NONE && u.weapon_taken == W_BOW && ai.next_attack <= 0.f
										&& u.GetStaminap() >= 0.25f && u.frozen == FROZEN::NO
										&& game_level->CanShootAtLocation2(u, ai.st.idle.obj.ptr, ai.st.idle.obj.pos))
									{
										// bow shooting
										u.target_pos = ai.st.idle.obj.pos;
										u.target_pos.y += 1.27f;
										u.DoRangedAttack(false);
									}
									look_at = LookAtPoint;
									look_pos = ai.st.idle.obj.pos;
									ai.in_combat = true;
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
							if(Vec3::Distance2d(u.pos, ai.st.idle.region.pos) < u.GetUnitRadius() * 2)
							{
								if(game_level->city_ctx && !IsSet(game_level->city_ctx->flags, City::HaveExit) && u.area == ai.st.idle.region.area
									&& ai.st.idle.region.area->area_type == LevelArea::Type::Outside && !ai.st.idle.region.exit)
								{
									// in exit area, go to border
									ai.st.idle.region.exit = true;
									ai.st.idle.region.pos = game_level->GetExitPos(u, true);
								}
								else
								{
									ai.st.idle.action = AIController::Idle_None;
									game_level->WarpUnit(&u, ai.st.idle.region.area->area_id);
									if(ai.st.idle.region.area->area_type != LevelArea::Type::Building || (u.IsFollower() && order == ORDER_FOLLOW))
									{
										ai.loc_timer = -1.f;
										ai.timer = -1.f;
									}
									else
									{
										// stay somewhere in the inn
										ai.st.idle.action = AIController::Idle_Move;
										ai.timer = Random(5.f, 15.f);
										ai.loc_timer = Random(60.f, 120.f);
										InsideBuilding* inside = static_cast<InsideBuilding*>(ai.st.idle.region.area);
										ai.st.idle.pos = (Rand() % 5 == 0 ? inside->region2 : inside->region1).GetRandomPos3();
									}
								}
							}
							else
							{
								move_type = MovePoint;
								target_pos = ai.st.idle.region.pos;
								look_at = LookAtWalk;
								run_type = (ai.st.idle.action == AIController::Idle_MoveRegion ? Walk : WalkIfNear);
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
								look_pos = ai.st.idle.pos;
								u.animation = ANI_KNEELS;
							}
							break;
						case AIController::Idle_MoveAway:
							if(Unit* target = ai.st.idle.unit)
							{
								look_at = LookAtPoint;
								look_pos = target->pos;
								if(Vec3::Distance2d(u.pos, target->pos) < 2.f)
								{
									move_type = MoveAway;
									target_pos = target->pos;
									run_type = Walk;
								}
							}
							else
								ai.st.idle.action = AIController::Idle_None;
							break;
						default:
							assert(0);
							break;
						}
					}
				}
				break;

			//===================================================================================================================
			// fight
			case AIController::Fighting:
				{
					ai.in_combat = true;

					if(!enemy)
					{
						// enemy lost from the eyesight
						if(Unit* target = ai.target; !target || !target->IsAlive())
						{
							// target died, end of the fight
							ai.state = AIController::Idle;
							ai.st.idle.action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
						}
						else
						{
							// go to last seen place
							ai.state = AIController::SeenEnemy;
							ai.timer = Random(10.f, 15.f);
						}
						repeat = true;
						break;
					}

					// ignore alert target
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
					float staminap = u.GetStaminap();
					if(tick && u.action == A_NONE && (staminap < 0.25f || Rand() % 6 == 0))
					{
						if(Rand() % 4 == 0 || staminap <= 0.f)
						{
							ai.timer = Random(0.5f, 1.f);
							if(staminap <= 0.f)
								ai.timer += 1.f;
							ai.state = AIController::Wait;
							break;
						}
						else
							ai.timer = 0.25f;
					}

					// change current weapon
					if(u.action == A_NONE)
					{
						WeaponType weapon = W_NONE;
						if(u.PreferMelee() || IsSet(u.data->flags, F_MAGE) || !u.HaveBow())
						{
							if(u.HaveWeapon())
								weapon = W_ONE_HANDED;
							else if(u.HaveBow())
								weapon = W_BOW;
						}
						else
						{
							float safe_dist = (u.IsHoldingMeeleWeapon() ? 5.f : 2.5f);
							if(IsSet(u.data->flags, F_ARCHER))
								safe_dist /= 2.f;
							if(best_dist > safe_dist)
								weapon = W_BOW;
							else if(u.HaveWeapon())
								weapon = W_ONE_HANDED;
						}

						if(weapon != W_NONE && u.weapon_taken != weapon)
							u.TakeWeapon(weapon);
					}

					if(u.data->abilities && u.action == A_NONE && u.frozen == FROZEN::NO && ai.next_attack <= 0.f)
					{
						// spellcasting
						for(int i = MAX_ABILITIES - 1; i >= 0; --i)
						{
							if(u.data->abilities->ability[i] && u.level >= u.data->abilities->level[i] && ai.cooldown[i] <= 0.f)
							{
								Ability& ability = *u.data->abilities->ability[i];

								if(IsSet(ability.flags, Ability::NonCombat)
									|| ability.mana > u.mp
									|| ability.stamina > u.stamina)
									continue;

								if(IsSet(ability.flags, Ability::Boss50Hp) && u.GetHpp() > 0.5f)
									continue;

								if(best_dist < ability.range
									// can't cast drain blood on bloodless units
									&& !(ability.effect == Ability::Drain && IsSet(enemy->data->flags2, F2_BLOODLESS))
									&& game_level->CanShootAtLocation(u, *enemy, enemy->pos)
									&& (ability.type != Ability::Charge || AngleDiff(u.rot, Vec3::LookAtAngle(u.pos, enemy->pos)) < 0.1f))
								{
									ai.cooldown[i] = ability.cooldown.Random();
									u.action = A_CAST;
									u.animation_state = AS_CAST_ANIMATION;
									u.act.cast.ability = &ability;
									u.act.cast.target = nullptr;

									if(ability.mana > 0.f)
										u.RemoveMana(ability.mana);
									if(ability.stamina > 0.f)
										u.RemoveStamina(ability.stamina);

									if(ability.animation.empty())
									{
										if(u.mesh_inst->mesh->head.n_groups == 2)
											u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
										else
										{
											u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
											u.animation = ANI_PLAY;
										}
									}
									else
									{
										u.mesh_inst->Play(ability.animation.c_str(), PLAY_ONCE | PLAY_PRIO1, 0);
										u.animation = ANI_PLAY;
									}

									if(Net::IsOnline())
									{
										NetChange& c = Add1(Net::changes);
										c.type = NetChange::CAST_SPELL;
										c.unit = &u;
										c.ability = &ability;
									}

									break;
								}
							}
						}
					}

					if(u.IsHoldingBow() || IsSet(u.data->flags, F_MAGE))
					{
						// mage and bowmen distace keeping
						move_type = KeepDistanceCheck;
						look_at = LookAtTarget;
						target_pos = enemy->pos;
					}
					if(u.action == A_CAST)
					{
						// spellshot
						if(u.act.cast.ability->IsTargeted())
						{
							look_pos = ai.PredictTargetPos(*enemy, u.act.cast.ability->speed);
							look_at = LookAtPoint;
							u.target_pos = look_pos;
						}
						else
							break;
					}
					else if(u.action == A_SHOOT)
					{
						// bowshot
						float arrow_speed = u.GetArrowSpeed();
						look_pos = ai.PredictTargetPos(*enemy, arrow_speed);
						look_at = LookAtPoint;
						u.target_pos = look_pos;
					}

					if(u.IsHoldingBow())
					{
						if(u.action == A_NONE && ai.next_attack <= 0.f && u.frozen == FROZEN::NO)
						{
							// shooting possibility check
							look_pos = ai.PredictTargetPos(*enemy, u.GetArrowSpeed());

							if(game_level->CanShootAtLocation(u, *enemy, enemy->pos))
							{
								// bowshot
								u.DoRangedAttack(false);
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
								ai.DoAttack(enemy);
							else if(u.running && best_dist <= u.GetAttackRange() * 3 + 1.f)
								ai.DoAttack(enemy, true);
						}

						// stay close but not too close
						if(ai.state == AIController::Fighting && !(u.IsHoldingBow() || IsSet(u.data->flags, F_MAGE)))
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
							else if(Any(u.action, A_EAT, A_DRINK))
								move_type = DontMove;
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
							// mark the closest attacker
							Unit* top = nullptr;
							float best_dist = JUMP_BACK_MIN_RANGE;

							for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
							{
								if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->IsInvisible() && u.IsEnemy(**it2) && (*it2)->action == A_ATTACK
									&& !(*it2)->act.attack.hitted && (*it2)->animation_state < AS_ATTACK_FINISHED)
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
									// blocking
									float speed = u.GetBlockSpeed();
									ai.state = AIController::Block;
									ai.timer = BLOCK_TIMER;
									ai.ignore = 0.f;
									u.action = A_BLOCK;
									u.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END, 1);
									u.mesh_inst->groups[1].blend_max = speed;

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
									// dodge
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
			// go to the last known enemy position
			case AIController::SeenEnemy:
				{
					if(enemy)
					{
						// enemy spotted
						ai.target = enemy;
						ai.target_last_pos = enemy->pos;
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
						repeat = true;
						break;
					}

					if(Unit* alert_target = ai.alert_target)
					{
						// someone else spotted enemy
						ai.target = alert_target;
						ai.target_last_pos = ai.alert_target_pos;
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
						ai.alert_target = nullptr;
						repeat = true;
						break;
					}

					move_type = MovePoint;
					target_pos = ai.target_last_pos;
					look_at = LookAtWalk;
					run_type = Run;

					if(ai.target)
					{
						if(Unit* target = ai.target; !target || !target->IsAlive())
						{
							// target is dead
							ai.state = AIController::Idle;
							ai.st.idle.action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
							ai.target = nullptr;
							break;
						}
					}

					if(Vec3::Distance(u.pos, ai.target_last_pos) < 1.f || ai.timer <= 0.f)
					{
						// last seen point reached
						if(game_level->location->outside)
						{
							// outside, therefore idle
							ai.state = AIController::Idle;
							ai.st.idle.action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
						}
						else
						{
							InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
							Room* room = inside->FindChaseRoom(u.pos);
							if(room)
							{
								// underground, go to random nearby room
								ai.timer = u.IsFollower() ? Random(1.f, 2.f) : Random(15.f, 30.f);
								ai.state = AIController::SearchEnemy;
								ai.st.search.room = room->connected[Rand() % room->connected.size()];
								ai.target_last_pos = ai.st.search.room->GetRandomPos(u.GetUnitRadius());
							}
							else
							{
								// does not work because of no rooms in the labirynth/cave
								ai.state = AIController::Idle;
								ai.st.idle.action = AIController::Idle_None;
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
			// search enemies in the nearby rooms
			case AIController::SearchEnemy:
				{
					if(enemy)
					{
						// enemy noticed
						ai.target = enemy;
						ai.target_last_pos = enemy->pos;
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
						ai.Shout();
						repeat = true;
						break;
					}

					if(Unit* alert_target = ai.alert_target)
					{
						// enemy noticed by someone else
						ai.target = alert_target;
						ai.target_last_pos = ai.alert_target_pos;
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
						ai.alert_target = nullptr;
						repeat = true;
						break;
					}

					move_type = MovePoint;
					target_pos = ai.target_last_pos;
					look_at = LookAtWalk;
					run_type = Run;

					if(ai.timer <= 0.f || u.IsFollower()) // heroes in team, dont search enemies on your own
					{
						// pursuit finished, no enemy found
						ai.state = AIController::Idle;
						ai.st.idle.action = AIController::Idle_None;
						ai.in_combat = false;
						ai.change_ai_mode = true;
						ai.loc_timer = Random(5.f, 10.f);
						ai.timer = Random(1.f, 2.f);
					}
					else if(Vec3::Distance(u.pos, ai.target_last_pos) < 1.f)
					{
						// search for another room
						InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
						InsideLocationLevel& lvl = inside->GetLevelData();
						Room* room = ai.st.search.room;
						if(!room) // pre V_0_13
							room = lvl.GetNearestRoom(u.pos);
						ai.st.search.room = room->connected[Rand() % room->connected.size()];
						ai.target_last_pos = ai.st.search.room->GetRandomPos(u.GetUnitRadius());
					}
				}
				break;

			//===================================================================================================================
			// escape
			case AIController::Escape:
				{
					// try to heal if you can and should
					ai.CheckPotion();

					// ignore alert target
					ai.alert_target = nullptr;

					if(u.GetOrder() == ORDER_ESCAPE_TO_UNIT)
					{
						Unit* order_unit = u.order->unit;
						if(!order_unit)
							u.order->order = ORDER_ESCAPE_TO;
						else
						{
							if(Vec3::Distance(order_unit->pos, u.pos) < 3.f)
							{
								u.OrderNext();
								break;
							}
							move_type = MovePoint;
							run_type = Run;
							look_at = LookAtWalk;
							u.order->pos = order_unit->pos;
							target_pos = order_unit->pos;
							break;
						}
					}
					if(u.GetOrder() == ORDER_ESCAPE_TO)
					{
						if(Vec3::Distance(u.order->pos, u.pos) < 0.1f)
						{
							u.OrderNext();
							break;
						}
						move_type = MovePoint;
						run_type = Run;
						look_at = LookAtWalk;
						target_pos = u.order->pos;
						break;
					}

					// check if should finish escaping
					ai.timer -= dt;
					if(ai.timer <= 0.f)
					{
						if(!ai.in_combat)
						{
							ai.state = AIController::Idle;
							ai.st.idle.action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
							break;
						}
						else if(ai.GetMorale() > 0.f)
						{
							ai.state = AIController::Fighting;
							ai.timer = 0.f;
							repeat = true;
							break;
						}
						else
							ai.timer = Random(2.f, 4.f);
					}

					if(enemy)
					{
						ai.target_last_pos = enemy->pos;
						if(game_level->location->outside)
						{
							// underground advanced escape, run ahead when outside
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
						else if(!ai.st.escape.room)
						{
							// escape location not set yet
							if(best_dist < 1.5f)
							{
								// better not to stay back to the enemy at this moment
								move_type = MoveAway;
								target_pos = enemy->pos;
							}
							else if(ai.ignore <= 0.f)
							{
								// search for the room

								// gather nearby enemies
								close_enemies.clear();
								for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
								{
									if((*it2)->to_remove || !(*it2)->IsStanding() || (*it2)->IsInvisible() || !u.IsEnemy(**it2))
										continue;

									if(Vec3::Distance(u.pos, (*it2)->pos) < ALERT_RANGE + 0.1f)
										close_enemies.push_back(*it2);
								}

								// determine enemy occupied room
								Vec3 mid = close_enemies.front()->pos;
								for(vector<Unit*>::iterator it2 = close_enemies.begin() + 1, end2 = close_enemies.end(); it2 != end2; ++it2)
									mid += (*it2)->pos;
								mid /= (float)close_enemies.size();

								// find room to escape to
								Room* room = static_cast<InsideLocation*>(game_level->location)->GetLevelData().FindEscapeRoom(u.pos, mid);
								if(room)
								{
									// escape to the room
									ai.st.escape.room = room;
									move_type = MovePoint;
									look_at = LookAtWalk;
									target_pos = ai.st.escape.room->Center();
								}
								else
								{
									// no way, just escape back
									move_type = MoveAway;
									target_pos = enemy->pos;
									ai.ignore = 0.5f;
								}
							}
							else
							{
								// wait before next search
								ai.ignore -= dt;
								move_type = MoveAway;
								target_pos = enemy->pos;
							}
						}
						else
						{
							if(best_dist < 1.f)
							{
								// someone put him in the corner or catched him up
								ai.st.escape.room = nullptr;
								move_type = MoveAway;
								target_pos = enemy->pos;
							}
							else if(ai.st.escape.room->IsInside(u.pos))
							{
								// reached escape room, find next room
								ai.st.escape.room = nullptr;
							}
							else
							{
								// escape to room
								target_pos = ai.st.escape.room->Center();
								move_type = MovePoint;
								look_at = LookAtWalk;
							}
						}
					}
					else
					{
						// no enemies
						if(ai.st.escape.room)
						{
							// run to the place
							target_pos = ai.st.escape.room->Center();
							if(Vec3::Distance(target_pos, u.pos) > 0.5f)
							{
								move_type = MovePoint;
								look_at = LookAtWalk;
							}
						}
						else if(Unit* target = ai.target)
						{
							ai.target_last_pos = target->pos;
							move_type = MoveAway;
							look_at = LookAtTarget;
							target_pos = ai.target_last_pos;
						}
						else
						{
							ai.target = nullptr;
							move_type = MoveAway;
							look_at = LookAtWalk;
							target_pos = ai.target_last_pos;
						}
					}
				}
				break;

			//===================================================================================================================
			// block hits
			case AIController::Block:
				{
					// choose the closest attacker
					Unit* top = nullptr;
					float best_dist = 5.f;

					for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
					{
						if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->IsInvisible() && u.IsEnemy(**it2) && (*it2)->action == A_ATTACK
							&& !(*it2)->act.attack.hitted && (*it2)->animation_state < AS_ATTACK_FINISHED)
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

						// hit with shield
						if(best_dist <= u.GetAttackRange() && !u.mesh_inst->groups[1].IsBlending() && ai.ignore <= 0.f)
						{
							if(Rand() % 2 == 0)
							{
								const float speed = u.GetBashSpeed();
								u.action = A_BASH;
								u.animation_state = AS_BASH_ANIMATION;
								u.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1, 1);
								u.mesh_inst->groups[1].speed = speed;
								u.RemoveStamina(u.GetStaminaMod(u.GetShield()) * Unit::STAMINA_BASH_ATTACK);

								if(Net::IsOnline())
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::ATTACK;
									c.unit = &u;
									c.id = AID_Bash;
									c.f[1] = speed;
								}
							}
							else
								ai.ignore = 1.f;
						}
					}

					if(!top || ai.timer <= 0.f)
					{
						// no hits to block or time expiration
						u.action = A_NONE;
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
							c.id = AID_Cancel;
							c.f[1] = 1.f;
						}
						break;
					}
				}
				break;

			//===================================================================================================================
			// dodge attacks
			case AIController::Dodge:
				{
					// choose the closest attacker
					Unit* top = nullptr;
					float best_dist = JUMP_BACK_MIN_RANGE;

					for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
					{
						if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->IsInvisible() && u.IsEnemy(**it2))
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

						// attack
						if(u.action == A_NONE && best_dist <= u.GetAttackRange() && ai.next_attack <= 0.f && u.frozen == FROZEN::NO)
							ai.DoAttack(enemy);
						else if(u.action == A_CAST)
						{
							// cast a spell
							look_pos = ai.PredictTargetPos(*top, u.act.cast.ability->speed);
							look_at = LookAtPoint;
							u.target_pos = look_pos;
						}
						else if(u.action == A_SHOOT)
						{
							// shoot with bow
							look_pos = ai.PredictTargetPos(*top, u.GetArrowSpeed());
							look_at = LookAtPoint;
							u.target_pos = look_pos;
						}
					}

					// dodge finished, time expiration or no enemies
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
			// spellcasting upon target
			case AIController::Cast:
				if(Unit* target = ai.target)
				{
					Ability& ability = *ai.st.cast.ability;
					bool ok = true;
					if(target == &u)
					{
						move_type = DontMove;
						look_at = DontLook;
					}
					else
					{
						move_type = MovePoint;
						run_type = WalkIfNear;
						look_at = LookAtPoint;
						target_pos = look_pos = target->pos;

						if(Vec3::Distance(u.pos, target_pos) <= ability.range)
							move_type = DontMove;
						else
						{
							ok = false;

							// fix for Jozan getting stuck when trying to heal someone
							if(u.IsHero())
								try_phase = true;
						}
					}

					if(ok && u.action == A_NONE)
					{
						for(int i = 0; i < MAX_ABILITIES; ++i)
						{
							if(u.data->abilities->ability[i] == &ability)
							{
								ai.cooldown[i] = ability.cooldown.Random();
								break;
							}
						}
						u.action = A_CAST;
						u.animation_state = AS_CAST_ANIMATION;
						u.act.cast.ability = &ability;
						u.act.cast.target = target;
						u.target_pos = target->pos;

						if(ability.mana > 0.f)
							u.RemoveMana(ability.mana);
						if(ability.stamina > 0.f)
							u.RemoveStamina(ability.stamina);

						if(ability.animation.empty())
						{
							if(u.mesh_inst->mesh->head.n_groups == 2)
								u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
							else
							{
								u.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
								u.animation = ANI_PLAY;
							}
						}
						else
						{
							u.mesh_inst->Play(ability.animation.c_str(), PLAY_ONCE | PLAY_PRIO1, 0);
							u.animation = ANI_PLAY;
						}

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CAST_SPELL;
							c.unit = &u;
							c.ability = &ability;
						}
					}

					if(u.action == A_CAST)
						u.target_pos = target_pos;
				}
				else
				{
					ai.state = AIController::Idle;
					ai.st.idle.action = AIController::Idle_None;
					ai.timer = Random(1.f, 2.f);
					ai.loc_timer = Random(5.f, 10.f);
					ai.timer = Random(1.f, 2.f);
				}
				break;

			//===================================================================================================================
			// wait for stamina
			case AIController::Wait:
				{
					Unit* top = nullptr;
					float best_dist = 10.f;

					for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
					{
						if(!(*it2)->to_remove && (*it2)->IsStanding() && !(*it2)->IsInvisible() && u.IsEnemy(**it2))
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
						if(u.IsHoldingBow() || IsSet(u.data->flags, F_MAGE))
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

			//===================================================================================================================
			case AIController::Aggro:
				{
					if(Unit* alert_target = ai.alert_target)
					{
						// someone else alert him with enemy alert shout
						ai.target = alert_target;
						ai.target_last_pos = ai.alert_target_pos;
						ai.alert_target = nullptr;
						ai.state = AIController::Fighting;
						ai.timer = 0.f;
						repeat = true;
						if(IsSet(u.data->flags2, F2_YELL))
							ai.Shout();
						break;
					}

					if(enemy)
					{
						if(best_dist < ALERT_RANGE_ATTACK || ai.timer <= 0)
						{
							// attack
							ai.target = enemy;
							ai.target_last_pos = enemy->pos;
							ai.timer = 0.f;
							ai.state = AIController::Fighting;
							ai.Shout();
							repeat = true;
							break;
						}

						ai.target_last_pos = enemy->pos;
						if(AngleDiff(u.rot, Vec3::LookAtAngle(u.pos, ai.target_last_pos)) < 0.1f && u.action == A_NONE && ai.next_attack < 0)
						{
							// aggro
							u.mesh_inst->Play("aggro", PLAY_ONCE | PLAY_PRIO1, 0);
							u.action = A_ANIMATION;
							u.animation = ANI_PLAY;
							ai.next_attack = u.mesh_inst->GetGroup(0).anim->length + Random(0.2f, 0.5f);

							u.PlaySound(u.GetSound(SOUND_AGGRO), AGGRO_SOUND_DIST);
						}

						// look at
						look_at = LookAtTarget;
						target_pos = ai.target_last_pos;
					}
					else
					{
						if(AngleDiff(u.rot, Vec3::LookAtAngle(u.pos, ai.target_last_pos)) > 0.1f)
						{
							// look at
							look_at = LookAtTarget;
							target_pos = ai.target_last_pos;
						}
						else
						{
							// end aggro
							ai.state = AIController::Idle;
							ai.st.idle.action = AIController::Idle_None;
							ai.in_combat = false;
							ai.change_ai_mode = true;
							ai.loc_timer = Random(5.f, 10.f);
							ai.timer = Random(1.f, 2.f);
						}
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

		// animation
		if(u.animation != ANI_PLAY && u.animation != ANI_KNEELS)
		{
			if(ai.in_combat || ((ai.st.idle.action == AIController::Idle_TrainBow || ai.st.idle.action == AIController::Idle_TrainCombat) && u.weapon_state == WeaponState::Taken))
			{
				if(u.IsHoldingBow())
					u.animation = ANI_BATTLE_BOW;
				else
					u.animation = ANI_BATTLE;
			}
			else if(u.animation != ANI_IDLE)
				u.animation = ANI_STAND;
		}

		// character movement
		u.running = false;
		if(move_type != DontMove && u.frozen == FROZEN::NO)
		{
			int move;

			if(move_type == KeepDistanceCheck)
			{
				if(u.action == A_TAKE_WEAPON || game_level->CanShootAtLocation(u, *enemy, target_pos))
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
					if(best_dist < u.GetAttackRange() * 0.9f)
						move = -1;
					else if(best_dist > u.GetAttackRange())
						move = 1;
					else
						move = 0;
				}
				else if(run_type == WalkIfNear2 && look_at != LookAtWalk)
				{
					if(AngleDiff(u.rot, Vec3::LookAtAngle(u.pos, target_pos)) > PI / 4)
						move = 0;
					else
						move = 1;
				}
				else
					move = 1;
			}

			if(move != 0 && !u.CanMove())
				move = 0;

			if(move == -1)
			{
				// back movement
				u.speed = u.GetWalkSpeed();
				u.prev_speed = Clamp((u.prev_speed + (u.speed - u.prev_speed) * dt * 3), 0.f, u.speed);
				float speed = u.prev_speed * dt;
				const float angle = Vec3::LookAtAngle(u.pos, target_pos);

				u.prev_pos = u.pos;

				const Vec3 dir(sin(angle) * speed, 0, cos(angle) * speed);
				bool small;

				if(move_type == KeepDistanceCheck)
				{
					u.pos += dir;
					if(u.action != A_TAKE_WEAPON && !game_level->CanShootAtLocation(u, *enemy, target_pos))
						move = 0;
					u.pos = u.prev_pos;
				}

				if(move != 0 && game_level->CheckMove(u.pos, dir, u.GetUnitRadius(), &u, &small))
				{
					u.Moved();
					if(!small && u.animation != ANI_PLAY)
						u.animation = ANI_WALK_BACK;
				}
			}
			else if(move == 1)
			{
				// movement towards the point (pathfinding)
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
						if(pathfinding->FindPath(area, my_tile, target_tile, ai.pf_path, !IsSet(u.data->flags, F_DONT_OPEN), ai.city_wander && game_level->city_ctx != nullptr))
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

				// open doors when ai moves next to them
				if(!IsSet(u.data->flags, F_DONT_OPEN))
				{
					for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
					{
						Door& door = **it;
						if(door.IsBlocking() && door.state == Door::Closed && door.locked == LOCK_NONE && Vec3::Distance(door.pos, u.pos) < 1.f)
							door.Open();
					}
				}

				// local pathfinding
				Vec3 move_target;
				if(ai.pf_state == AIController::PFS_GLOBAL_DONE
					|| ai.pf_state == AIController::PFS_GLOBAL_NOT_USED
					|| ai.pf_state == AIController::PFS_LOCAL_TRY_WALK)
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

					int ret = pathfinding->FindLocalPath(area, ai.pf_local_path, my_local_tile, local_tile, &u, path_unit_ignore, path_obj_ignore, is_end_point);
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
						move_target = Vec3(0.25f * pt.x + 0.125f, 0, 0.25f * pt.y + 0.125f);
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
								move_target = Vec3(0.25f * local_tile.x + 0.125f, 0, 0.25f * local_tile.y + 0.125f);
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
							move_target = Vec3(0.25f * local_tile.x + 0.125f, 0, 0.25f * local_tile.y + 0.125f);
						}
					}
					else
					{
						ai.pf_state = AIController::PFS_MANUAL_WALK;
						move_target = target_pos;
					}
				}
				else if(ai.pf_state == AIController::PFS_MANUAL_WALK)
					move_target = target_pos;
				else if(ai.pf_state == AIController::PFS_WALKING
					|| ai.pf_state == AIController::PFS_WALKING_LOCAL)
				{
					const Int2& pt = ai.pf_local_path.back();
					move_target = Vec3(0.25f * pt.x + 0.125f, 0, 0.25f * pt.y + 0.125f);
				}

			skip_localpf:

				if(move != 0)
				{
					// character movement
					bool run;
					if(!u.CanRun())
						run = false;
					else if(u.action == A_ATTACK && u.act.attack.run)
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
					u.prev_speed = Clamp((u.prev_speed + (u.speed - u.prev_speed) * dt * 3), 0.f, u.speed);
					float speed = u.prev_speed * dt;
					const float angle = Vec3::LookAtAngle(u.pos, move_target) + PI;

					u.prev_pos = u.pos;

					if(move_target == target_pos)
					{
						float dist = Vec3::Distance2d(u.pos, target_pos);
						if(dist < speed)
							speed = dist;
					}
					const Vec3 dir(sin(angle) * speed, 0, cos(angle) * speed);
					bool small;

					if(move_type == KeepDistanceCheck)
					{
						u.pos += dir;
						if(!game_level->CanShootAtLocation(u, *enemy, target_pos))
							move = 0;
						u.pos = u.prev_pos;
					}

					if(move != 0)
					{
						int move_state = 0;
						if(game_level->CheckMove(u.pos, dir, u.GetUnitRadius(), &u, &small))
							move_state = 1;
						else if(try_phase && u.hero->phase)
						{
							move_state = 2;
							u.pos += dir;
							small = false;
						}

						if(move_state)
						{
							u.Moved();

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
								{
									u.animation = ANI_RUN;
									u.running = abs(u.speed - u.prev_speed) < 0.25;
								}
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

		// rotation
		Vec3 look_pos2;
		if(look_at == LookAtTarget || look_at == LookAtPoint || (look_at == LookAtWalk && look_pt_valid))
		{
			Vec3 look_pt;
			if(look_at == LookAtTarget)
				look_pt = target_pos;
			else
				look_pt = look_pos;

			// look at the target
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
				{
					u.rot = dir;
					if(u.GetOrder() == ORDER_LOOK_AT && u.order->timer < 0.f)
						u.OrderNext();
				}
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
