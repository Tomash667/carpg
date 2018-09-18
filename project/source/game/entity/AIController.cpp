#include "Pch.h"
#include "GameCore.h"
#include "AIController.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "InsideLocation.h"
#include "GameFile.h"
#include "Level.h"
#include "GlobalGui.h"
#include "GameMessages.h"

//=================================================================================================
void AIController::Init(Unit* _unit)
{
	assert(_unit);

	unit = _unit;
	unit->ai = this;
	target = nullptr;
	state = Idle;
	next_attack = 0.f;
	ignore = 0.f;
	morale = (IS_SET(_unit->data->flags, F_COWARD) ? 5.f : 10.f);
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	have_potion = 1;
	potion = -1;
	last_scan = 0.f;
	in_combat = false;
	alert_target = nullptr;
	escape_room = nullptr;
	idle_action = Idle_None;
	start_pos = unit->pos;
	start_rot = unit->rot;
	loc_timer = 0.f;
	timer = 0.f;
	goto_inn = false;
	change_ai_mode = false;
	pf_state = PFS_NOT_USING;
}

//=================================================================================================
void AIController::Save(GameWriter& f)
{
	if(state == Cast)
	{
		if(!Game::Get().ValidateTarget(*unit, cast_target))
		{
			state = Idle;
			idle_action = Idle_None;
			timer = Random(1.f, 2.f);
		}
	}
	f << unit->refid;
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
	f << last_scan;
	f << start_rot;
	if(unit->data->spells)
		f << cooldown;
	if(state == AIController::Escape)
		f << GetEscapeRoomId();
	else if(state == AIController::Cast)
		f << cast_target->refid;
	f << have_potion;
	f << potion;
	f << idle_action;
	switch(idle_action)
	{
	case AIController::Idle_None:
	case AIController::Idle_Animation:
		break;
	case AIController::Idle_Rot:
		f << idle_data.rot;
		break;
	case AIController::Idle_Move:
	case AIController::Idle_TrainCombat:
	case AIController::Idle_Pray:
		f << idle_data.pos;
		break;
	case AIController::Idle_Look:
	case AIController::Idle_WalkTo:
	case AIController::Idle_Chat:
	case AIController::Idle_WalkNearUnit:
	case AIController::Idle_MoveAway:
		f << idle_data.unit->refid;
		break;
	case AIController::Idle_Use:
	case AIController::Idle_WalkUse:
	case AIController::Idle_WalkUseEat:
		f << idle_data.usable->refid;
		break;
	case AIController::Idle_TrainBow:
		f << idle_data.obj.pos;
		f << idle_data.obj.rot;
		break;
	case AIController::Idle_MoveRegion:
	case AIController::Idle_RunRegion:
		f << idle_data.area.id;
		f << idle_data.area.pos;
		break;
	default:
		assert(0);
		break;
	}
	f << city_wander;
	f << loc_timer;
	f << shoot_yspeed;
	f << goto_inn;
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
	if(LOAD_VERSION >= V_0_3)
	{
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
	}
	else
	{
		bool use_path = f.Read<bool>();
		if(use_path)
		{
			f.ReadVector4(pf_path);
			f >> pf_target_tile;
		}
		bool use_local_path = f.Read<bool>();
		if(use_local_path)
		{
			f.ReadVector4(pf_local_path);
			f >> pf_local_target_tile;
		}
		if(use_path && use_local_path)
			pf_state = PFS_WALKING;
		else
			pf_state = PFS_NOT_USING;
	}
	f >> next_attack;
	f >> timer;
	f >> ignore;
	f >> morale;
	if(LOAD_VERSION < V_0_3)
	{
		float last_pf_check, last_lpf_check;
		f >> last_pf_check;
		f >> last_lpf_check;
		if(pf_state == PFS_WALKING)
			pf_timer = last_pf_check;
	}
	f >> last_scan;
	f >> start_rot;
	if(unit->data->spells)
		f >> cooldown;
	if(state == AIController::Escape)
	{
		int room_id = f.Read<int>();
		if(room_id != -1)
		{
			if(!L.location->outside)
				escape_room = &((InsideLocation*)L.location)->GetLevelData().rooms[room_id];
			else
			{
				escape_room = nullptr;
#ifdef _DEBUG
				Warn("%s had escape_room %d.", unit->GetName(), room_id);
				Game::Get().gui->messages->AddGameMsg("Unit had escape room!", 5.f);
#endif
			}
		}
		else
			escape_room = nullptr;
	}
	else if(state == AIController::Cast)
	{
		cast_target = (Unit*)f.Read<int>();
		Game::Get().ai_cast_targets.push_back(this);
	}
	f >> have_potion;
	f >> potion;
	f >> idle_action;
	switch(idle_action)
	{
	case AIController::Idle_None:
	case AIController::Idle_Animation:
		break;
	case AIController::Idle_Rot:
		f >> idle_data.rot;
		break;
	case AIController::Idle_Move:
	case AIController::Idle_TrainCombat:
	case AIController::Idle_Pray:
		f >> idle_data.pos;
		break;
	case AIController::Idle_Look:
	case AIController::Idle_WalkTo:
	case AIController::Idle_Chat:
	case AIController::Idle_WalkNearUnit:
	case AIController::Idle_MoveAway:
		f >> idle_data.unit;
		break;
	case AIController::Idle_Use:
	case AIController::Idle_WalkUse:
	case AIController::Idle_WalkUseEat:
		f >> idle_data.usable;
		break;
	case AIController::Idle_TrainBow:
		f >> idle_data.obj.pos;
		f >> idle_data.obj.rot;
		Game::Get().ai_bow_targets.push_back(this);
		break;
	case AIController::Idle_MoveRegion:
	case AIController::Idle_RunRegion:
		f >> idle_data.area.id;
		f >> idle_data.area.pos;
		break;
	default:
		assert(0);
		break;
	}
	f >> city_wander;
	f >> loc_timer;
	f >> shoot_yspeed;
	f >> goto_inn;
	change_ai_mode = false;
}

//=================================================================================================
bool AIController::CheckPotion(bool in_combat)
{
	if(unit->action == A_NONE && have_potion > 0)
	{
		float hpp = unit->GetHpp();
		if(hpp < 0.5f || (hpp < 0.75f && !in_combat) || (!Equal(hpp, 1.f) && unit->busy == Unit::Busy_Tournament))
		{
			int index = unit->FindHealingPotion();
			if(index == -1)
			{
				if(unit->busy == Unit::Busy_No && unit->IsFollower() && !unit->summoner)
				{
					Game& game = Game::Get();
					game.UnitTalk(*unit, RandomString(game.txAiNoHpPot));
				}
				have_potion = 0;
				return false;
			}

			if(unit->ConsumeItem(index) != 3 && this->in_combat)
				state = AIController::Dodge;
			timer = Random(1.f, 1.5f);

			return true;
		}
	}

	return false;
}

//=================================================================================================
void AIController::Reset()
{
	target = nullptr;
	alert_target = nullptr;
	state = Idle;
	pf_path.clear();
	pf_local_path.clear();
	in_combat = false;
	next_attack = 0.f;
	timer = 0.f;
	ignore = 0.f;
	morale = (IS_SET(unit->data->flags, F_COWARD) ? 5.f : 10.f);
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	last_scan = 0.f;
	escape_room = nullptr;
	have_potion = 1;
	idle_action = Idle_None;
	change_ai_mode = true;
	unit->run_attack = false;
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

	return morale;
}

//=================================================================================================
int AIController::GetEscapeRoomId() const
{
	if(escape_room)
		return ((InsideLocation*)L.location)->GetLevelData().GetRoomId(escape_room);
	else
		return -1;
}
