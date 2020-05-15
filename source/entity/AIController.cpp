#include "Pch.h"
#include "AIController.h"

#include "Ability.h"
#include "Game.h"
#include "GameGui.h"
#include "GameFile.h"
#include "GameMessages.h"
#include "InsideLocation.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Tournament.h"
#include "SaveState.h"
#include "Unit.h"

//=================================================================================================
void AIController::Init(Unit* unit)
{
	assert(unit);

	this->unit = unit;
	unit->ai = this;
	state = Idle;
	next_attack = 0.f;
	ignore = 0.f;
	morale = unit->GetMaxMorale();
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	have_potion = HavePotion::Check;
	have_mp_potion = HavePotion::Check;
	potion = -1;
	in_combat = false;
	st.idle.action = Idle_None;
	start_pos = unit->pos;
	start_rot = unit->rot;
	loc_timer = 0.f;
	timer = 0.f;
	change_ai_mode = false;
	pf_state = PFS_NOT_USING;
}

//=================================================================================================
void AIController::Save(GameWriter& f)
{
	f << unit->id;
	f << target;
	f << alert_target;
	f << state;
	f << target_last_pos;
	f << alert_target_pos;
	f << start_pos;
	f << in_combat;
	f << pf_state;
	if(pf_state != PFS_NOT_USING)
	{
		f << pf_timer;
		if(pf_state == PFS_WALKING || pf_state == PFS_LOCAL_TRY_WALK)
		{
			f.WriteVector2(pf_path);
			f << pf_target_tile;
			if(pf_state == PFS_LOCAL_TRY_WALK)
				f << pf_local_try;
		}
		if(pf_state == PFS_WALKING || pf_state == PFS_WALKING_LOCAL)
		{
			f.WriteVector1(pf_local_path);
			f << pf_local_target_tile;
		}
	}
	f << next_attack;
	f << timer;
	f << ignore;
	f << morale;
	f << start_rot;
	if(unit->data->abilities)
		f << cooldown;
	switch(state)
	{
	case Cast:
		f << st.cast.ability->hash;
		break;
	case Escape:
		f << (st.escape.room ? st.escape.room->index : -1);
		break;
	case Idle:
		f << st.idle.action;
		switch(st.idle.action)
		{
		case Idle_None:
		case Idle_Animation:
			break;
		case Idle_Rot:
			f << st.idle.rot;
			break;
		case Idle_Move:
		case Idle_TrainCombat:
		case Idle_Pray:
			f << st.idle.pos;
			break;
		case Idle_Look:
		case Idle_WalkTo:
		case Idle_Chat:
		case Idle_WalkNearUnit:
		case Idle_MoveAway:
			f << st.idle.unit;
			break;
		case Idle_Use:
		case Idle_WalkUse:
		case Idle_WalkUseEat:
			f << st.idle.usable->id;
			break;
		case Idle_TrainBow:
			f << st.idle.obj.pos;
			f << st.idle.obj.rot;
			break;
		case Idle_MoveRegion:
		case Idle_RunRegion:
			f << st.idle.region.area->area_id;
			f << st.idle.region.pos;
			f << st.idle.region.exit;
			break;
		default:
			assert(0);
			break;
		}
		break;
	case SearchEnemy:
		f << (st.search.room ? st.search.room->index : -1);
		break;
	}
	f << have_potion;
	f << have_mp_potion;
	f << potion;
	f << city_wander;
	f << loc_timer;
}

//=================================================================================================
void AIController::Load(GameReader& f)
{
	f >> unit;
	unit->ai = this;
	f >> target;
	f >> alert_target;
	f >> state;
	f >> target_last_pos;
	f >> alert_target_pos;
	f >> start_pos;
	f >> in_combat;
	f >> pf_state;
	if(pf_state != PFS_NOT_USING)
	{
		f >> pf_timer;
		if(pf_state == PFS_WALKING || pf_state == PFS_LOCAL_TRY_WALK)
		{
			f.ReadVector2(pf_path);
			f >> pf_target_tile;
			if(pf_state == PFS_LOCAL_TRY_WALK)
				f >> pf_local_try;
		}
		if(pf_state == PFS_WALKING || pf_state == PFS_WALKING_LOCAL)
		{
			f.ReadVector1(pf_local_path);
			f >> pf_local_target_tile;
		}
	}
	f >> next_attack;
	f >> timer;
	f >> ignore;
	f >> morale;
	if(LOAD_VERSION < V_0_12)
		f.Skip<float>(); // old last_scan
	f >> start_rot;
	if(unit->data->abilities)
		f >> cooldown;
	switch(state)
	{
	case Cast:
		if(LOAD_VERSION >= V_0_13)
			st.cast.ability = Ability::Get(f.Read<int>());
		else
		{
			st.cast.ability = unit->data->abilities->ability[unit->ai_mode];
			if(LOAD_VERSION < V_0_12)
				f.Skip<int>(); // old cast_target
		}
		break;
	case Escape:
		{
			int room_id = f.Read<int>();
			if(room_id != -1)
				st.escape.room = reinterpret_cast<InsideLocation*>(game_level->location)->GetLevelData().rooms[room_id];
			else
				st.escape.room = nullptr;
		}
		break;
	case Idle:
		if(LOAD_VERSION >= V_0_13)
			LoadIdleAction(f, st.idle, true);
		break;
	case SearchEnemy:
		{
			int room_id = f.Read<int>();
			st.search.room = reinterpret_cast<InsideLocation*>(game_level->location)->GetLevelData().rooms[room_id];
		}
		break;
	}
	f >> have_potion;
	if(LOAD_VERSION >= V_0_12)
		f >> have_mp_potion;
	else
		have_mp_potion = HavePotion::Check;
	f >> potion;
	if(LOAD_VERSION < V_0_13)
	{
		if(state == Idle)
			LoadIdleAction(f, st.idle, true);
		else
		{
			StateData::IdleState idle;
			LoadIdleAction(f, idle, false);
		}
	}
	f >> city_wander;
	f >> loc_timer;
	if(LOAD_VERSION < V_0_13)
		f.Skip<float>(); // old shoot_yspeed
	if(LOAD_VERSION < V_0_12)
	{
		bool goto_inn;
		f >> goto_inn;
		if(goto_inn)
			unit->OrderGoToInn();
	}
	change_ai_mode = false;
}

//=================================================================================================
void AIController::LoadIdleAction(GameReader& f, StateData::IdleState& idle, bool apply)
{
	f >> idle.action;
	switch(idle.action)
	{
	case Idle_None:
	case Idle_Animation:
		break;
	case Idle_Rot:
		f >> idle.rot;
		break;
	case Idle_Move:
	case Idle_TrainCombat:
	case Idle_Pray:
		f >> idle.pos;
		break;
	case Idle_Look:
	case Idle_WalkTo:
	case Idle_Chat:
	case Idle_WalkNearUnit:
	case Idle_MoveAway:
		f >> idle.unit;
		break;
	case Idle_Use:
	case Idle_WalkUse:
	case Idle_WalkUseEat:
		f >> idle.usable;
		break;
	case Idle_TrainBow:
		f >> idle.obj.pos;
		f >> idle.obj.rot;
		if(apply)
			game->ai_bow_targets.push_back(this);
		break;
	case Idle_MoveRegion:
	case Idle_RunRegion:
		{
			int area_id;
			f >> area_id;
			f >> idle.region.pos;
			if(LOAD_VERSION >= V_0_11)
			{
				f >> idle.region.exit;
				idle.region.area = game_level->GetAreaById(area_id);
			}
			else
			{
				if(area_id == LevelArea::OLD_EXIT_ID)
				{
					idle.region.exit = true;
					idle.region.area = game_level->GetAreaById(LevelArea::OUTSIDE_ID);
				}
				else
				{
					idle.region.exit = false;
					idle.region.area = game_level->GetAreaById(area_id);
					if(!idle.region.area)
						idle.region.area = game_level->local_area;
				}
			}
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
bool AIController::CheckPotion(bool in_combat)
{
	if(unit->action != A_NONE)
		return false;

	if(have_potion != HavePotion::No && !IsSet(unit->data->flags, F_UNDEAD))
	{
		float hpp = unit->GetHpp();
		if(hpp < 0.5f || (hpp < 0.75f && !in_combat) || (!Equal(hpp, 1.f) && unit->busy == Unit::Busy_Tournament))
		{
			int index = unit->FindHealingPotion();
			if(index == -1)
			{
				if(unit->busy == Unit::Busy_No && unit->IsFollower() && !unit->summoner)
					unit->Talk(RandomString(game->txAiNoHpPot));
				have_potion = HavePotion::No;
				return false;
			}

			if(unit->ConsumeItem(index) != 3 && this->in_combat)
				state = Dodge;
			timer = Random(1.f, 1.5f);

			return true;
		}
	}

	if(have_mp_potion != HavePotion::No)
	{
		Class* clas = unit->GetClass();
		if(!clas || !IsSet(clas->flags, Class::F_MP_BAR))
			have_mp_potion = HavePotion::No;
		else
		{
			float mpp = unit->GetMpp();
			if(mpp < 0.33f || (mpp < 0.66f && !in_combat) || (mpp < 0.80f && unit->busy == Unit::Busy_Tournament))
			{
				int index = unit->FindManaPotion();
				if(index == -1)
				{
					if(unit->busy == Unit::Busy_No && unit->IsFollower() && !unit->summoner)
						unit->Talk(RandomString(game->txAiNoMpPot));
					have_mp_potion = HavePotion::No;
					return false;
				}

				if(unit->ConsumeItem(index) != 3 && this->in_combat)
					state = Dodge;
				timer = Random(1.f, 1.5f);

				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
void AIController::Reset()
{
	if(state != AIController::Idle)
	{
		state = Idle;
		change_ai_mode = true;
	}
	target = nullptr;
	alert_target = nullptr;
	pf_path.clear();
	pf_local_path.clear();
	in_combat = false;
	next_attack = 0.f;
	timer = Random(1.f, 3.f);;
	ignore = 0.f;
	morale = unit->GetMaxMorale();
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	have_potion = HavePotion::Check;
	have_mp_potion = HavePotion::Check;
	st.idle.action = Idle_None;
	pf_state = PFS_NOT_USING;
}

//=================================================================================================
float AIController::GetMorale() const
{
	float m = morale;
	float hpp = unit->GetHpp();

	if(hpp < 0.1f)
		m -= 3.f;
	else if(hpp < 0.25f)
		m -= 2.f;
	else if(hpp < 0.5f)
		m -= 1.f;

	return m;
}

//=================================================================================================
bool AIController::CanWander() const
{
	if(game_level->city_ctx && loc_timer <= 0.f && !game->dont_wander && IsSet(unit->data->flags, F_AI_WANDERS))
	{
		if(unit->busy != Unit::Busy_No)
			return false;
		if(unit->IsHero())
		{
			if(unit->hero->team_member && unit->GetOrder() != ORDER_WANDER)
				return false;
			else if(quest_mgr->quest_tournament->IsGenerated())
				return false;
			else
				return true;
		}
		else if(unit->area->area_type == LevelArea::Type::Outside)
			return true;
		else
			return false;
	}
	else
		return false;
}

//=================================================================================================
Vec3 AIController::PredictTargetPos(const Unit& target, float bullet_speed) const
{
	if(bullet_speed == 0.f)
		return target.GetCenter();

	Vec3 vel = target.pos - target.prev_pos;
	vel *= 60;

	float a = bullet_speed * bullet_speed - vel.Dot2d();
	float b = -2 * vel.Dot2d(target.pos - unit->pos);
	float c = -(target.pos - unit->pos).Dot2d();

	float delta = b * b - 4 * a * c;
	// no solution, can't hit target, just aim at center
	if(delta < 0)
		return target.GetCenter();

	Vec3 pos = target.pos + ((b + std::sqrt(delta)) / (2 * a)) * Vec3(vel.x, 0, vel.z);
	pos.y += target.GetUnitHeight() / 2;
	return pos;
}

//=================================================================================================
void AIController::Shout()
{
	if(!unit->data->sounds->Have(SOUND_SEE_ENEMY))
		return;

	game->PlayAttachedSound(*unit, unit->data->sounds->Random(SOUND_SEE_ENEMY), Unit::ALERT_SOUND_DIST);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UNIT_SOUND;
		c.unit = unit;
		c.id = SOUND_SEE_ENEMY;
	}

	// alarm near allies
	Unit* target_unit = target;
	for(Unit* u : unit->area->units)
	{
		if(u->to_remove || unit == u || !u->IsStanding() || u->IsPlayer() || !unit->IsFriend(*u) || u->ai->state == Fighting
			|| u->ai->alert_target || u->dont_attack)
			continue;

		if(Vec3::Distance(unit->pos, u->pos) <= 20.f && game_level->CanSee(*unit, *u))
		{
			u->ai->alert_target = target_unit;
			u->ai->alert_target_pos = target_last_pos;
		}
	}
}

//=================================================================================================
void AIController::HitReaction(const Vec3& pos)
{
	if(unit->dont_attack || !Any(state, Idle, SearchEnemy))
		return;

	target = nullptr;
	target_last_pos = pos;
	if(state == Idle)
		change_ai_mode = true;
	state = SeenEnemy;
	timer = Random(10.f, 15.f);
	city_wander = false;
	if(!unit->data->sounds->Have(SOUND_SEE_ENEMY))
		return;

	game->PlayAttachedSound(*unit, unit->data->sounds->Random(SOUND_SEE_ENEMY), Unit::ALERT_SOUND_DIST);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UNIT_SOUND;
		c.unit = unit;
		c.id = SOUND_SEE_ENEMY;
	}

	// alarm near allies
	for(Unit* u : unit->area->units)
	{
		if(u->to_remove || unit == u || !u->IsStanding() || u->IsPlayer() || !unit->IsFriend(*u) || u->dont_attack)
			continue;

		if((u->ai->state == Idle || u->ai->state == SearchEnemy)
			&& Vec3::Distance(unit->pos, u->pos) <= 20.f && game_level->CanSee(*unit, *u))
		{
			AIController* ai2 = u->ai;
			ai2->target_last_pos = pos;
			ai2->state = SeenEnemy;
			ai2->timer = Random(5.f, 10.f);
		}
	}
}

//=================================================================================================
// when target is nullptr, it deals no damage (dummy training)
void AIController::DoAttack(Unit* target, bool running)
{
	if(!(unit->action == A_NONE && (unit->mesh_inst->mesh->head.n_groups == 1 || unit->weapon_state == WeaponState::Taken) && next_attack <= 0.f))
		return;

	if(unit->data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
		game->PlayAttachedSound(*unit, unit->data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
	unit->action = A_ATTACK;
	unit->act.attack.index = unit->GetRandomAttack();

	bool do_power_attack;
	if(!IsSet(unit->data->flags, F_NO_POWER_ATTACK))
	{
		if(target && target->action == A_BLOCK)
			do_power_attack = (Rand() % 2 == 0);
		else
			do_power_attack = (Rand() % 5 == 0);
	}
	else
		do_power_attack = false;

	if(running)
	{
		unit->act.attack.power = 1.5f;
		unit->act.attack.run = true;
		do_power_attack = false;
	}
	else
	{
		unit->act.attack.power = 1.f;
		unit->act.attack.run = false;
	}

	float stamina_cost = (running || do_power_attack) ? 1.5f : 1.f;
	if(unit->HaveWeapon())
		stamina_cost *= unit->GetWeapon().GetInfo().stamina;
	else
		stamina_cost *= Unit::STAMINA_UNARMED_ATTACK;
	unit->RemoveStamina(stamina_cost);

	float speed = (do_power_attack ? unit->GetPowerAttackSpeed() : unit->GetAttackSpeed()) * unit->GetStaminaAttackSpeedMod();

	if(unit->mesh_inst->mesh->head.n_groups > 1)
	{
		unit->mesh_inst->Play(NAMES::ani_attacks[unit->act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
		unit->mesh_inst->groups[1].speed = speed;
	}
	else
	{
		unit->mesh_inst->Play(NAMES::ani_attacks[unit->act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 0);
		unit->mesh_inst->groups[0].speed = speed;
		unit->animation = ANI_PLAY;
	}
	unit->animation_state = (do_power_attack ? AS_ATTACK_PREPARE : AS_ATTACK_CAN_HIT);
	unit->act.attack.hitted = !target;

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ATTACK;
		c.unit = unit;
		c.id = (do_power_attack ? AID_PrepareAttack : (running ? AID_RunningAttack : AID_Attack));
		c.f[1] = speed;
	}
}
