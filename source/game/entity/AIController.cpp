// kontroler ai
#include "Pch.h"
#include "Base.h"
#include "AIController.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"

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
	goto_inn = false;
	change_ai_mode = false;
	pf_state = PFS_NOT_USING;
}

//=================================================================================================
void AIController::Save(HANDLE file)
{
	if(state == Cast)
	{
		if(!Game::Get().ValidateTarget(*unit, cast_target))
		{
			state = Idle;
			idle_action = Idle_None;
			timer = random(1.f,2.f);
		}
	}
	WriteFile(file, &unit->refid, sizeof(unit->refid), &tmp, nullptr);
	int refid = (target ? target->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	refid = (alert_target ? alert_target->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteFile(file, &state, sizeof(state), &tmp, nullptr);
	WriteFile(file, &target_last_pos, sizeof(target_last_pos), &tmp, nullptr);
	WriteFile(file, &alert_target_pos, sizeof(alert_target_pos), &tmp, nullptr);
	WriteFile(file, &start_pos, sizeof(start_pos), &tmp, nullptr);
	WriteFile(file, &in_combat, sizeof(in_combat), &tmp, nullptr);
	FileWriter f(file);
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
	WriteFile(file, &next_attack, sizeof(next_attack), &tmp, nullptr);
	WriteFile(file, &timer, sizeof(timer), &tmp, nullptr);
	WriteFile(file, &ignore, sizeof(ignore), &tmp, nullptr);
	WriteFile(file, &morale, sizeof(morale), &tmp, nullptr);
	WriteFile(file, &last_scan, sizeof(last_scan), &tmp, nullptr);
	WriteFile(file, &start_rot, sizeof(start_rot), &tmp, nullptr);
	if(unit->data->spells)
		WriteFile(file, cooldown, sizeof(cooldown), &tmp, nullptr);
	if(state == AIController::Escape)
	{
		if(escape_room)
			refid = ((InsideLocation*)Game::Get().location)->GetLevelData().GetRoomId(escape_room);
		else
			refid = -1;
		WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	}
	else if(state == AIController::Cast)
	{
		refid = cast_target->refid;
		WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	}
	WriteFile(file, &have_potion, sizeof(have_potion), &tmp, nullptr);
	WriteFile(file, &potion, sizeof(potion), &tmp, nullptr);
	WriteFile(file, &idle_action, sizeof(idle_action), &tmp, nullptr);
	switch(idle_action)
	{
	case AIController::Idle_None:
	case AIController::Idle_Animation:
		break;
	case AIController::Idle_Rot:
		WriteFile(file, &idle_data.rot, sizeof(idle_data.rot), &tmp, nullptr);
		break;
	case AIController::Idle_Move:
	case AIController::Idle_TrainCombat:
	case AIController::Idle_Pray:
		WriteFile(file, &idle_data.pos, sizeof(idle_data.pos), &tmp, nullptr);
		break;
	case AIController::Idle_Look:
	case AIController::Idle_WalkTo:
	case AIController::Idle_Chat:
	case AIController::Idle_WalkNearUnit:
	case AIController::Idle_MoveAway:
		WriteFile(file, &idle_data.unit->refid, sizeof(idle_data.unit->refid), &tmp, nullptr);
		break;
	case AIController::Idle_Use:
	case AIController::Idle_WalkUse:
	case AIController::Idle_WalkUseEat:
		WriteFile(file, &idle_data.useable->refid, sizeof(idle_data.useable->refid), &tmp, nullptr);
		break;
	case AIController::Idle_TrainBow:
		WriteFile(file, &idle_data.obj.pos, sizeof(idle_data.obj.pos), &tmp, nullptr);
		WriteFile(file, &idle_data.obj.rot, sizeof(idle_data.obj.rot), &tmp, nullptr);
		break;
	case AIController::Idle_MoveRegion:
	case AIController::Idle_RunRegion:
		WriteFile(file, &idle_data.area.id, sizeof(idle_data.area.id), &tmp, nullptr);
		WriteFile(file, &idle_data.area.pos, sizeof(idle_data.area.pos), &tmp, nullptr);
		break;
	default:
		assert(0);
		break;
	}
	WriteFile(file, &city_wander, sizeof(city_wander), &tmp, nullptr);
	WriteFile(file, &loc_timer, sizeof(loc_timer), &tmp, nullptr);
	WriteFile(file, &shoot_yspeed, sizeof(shoot_yspeed), &tmp, nullptr);
	WriteFile(file, &goto_inn, sizeof(goto_inn), &tmp, nullptr);
}

//=================================================================================================
void AIController::Load(HANDLE file)
{
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	unit = Unit::GetByRefid(refid);
	unit->ai = this;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	target = Unit::GetByRefid(refid);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	alert_target = Unit::GetByRefid(refid);
	ReadFile(file, &state, sizeof(state), &tmp, nullptr);
	ReadFile(file, &target_last_pos, sizeof(target_last_pos), &tmp, nullptr);
	ReadFile(file, &alert_target_pos, sizeof(alert_target_pos), &tmp, nullptr);
	ReadFile(file, &start_pos, sizeof(start_pos), &tmp, nullptr);
	ReadFile(file, &in_combat, sizeof(in_combat), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_3)
	{
		FileReader f(file);
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
		bool use_path;
		ReadFile(file, &use_path, sizeof(use_path), &tmp, nullptr);
		if(use_path)
		{
			uint count;
			ReadFile(file, &count, sizeof(count), &tmp, nullptr);
			pf_path.resize(count);
			if(count)
				ReadFile(file, &pf_path[0], sizeof(pf_path[0])*count, &tmp, nullptr);
			ReadFile(file, &pf_target_tile, sizeof(pf_target_tile), &tmp, nullptr);
		}
		bool use_local_path;
		ReadFile(file, &use_local_path, sizeof(use_local_path), &tmp, nullptr);
		if(use_local_path)
		{
			uint count;
			ReadFile(file, &count, sizeof(count), &tmp, nullptr);
			pf_local_path.resize(count);
			if(count)
				ReadFile(file, &pf_local_path[0], sizeof(pf_local_path[0])*count, &tmp, nullptr);
			ReadFile(file, &pf_local_target_tile, sizeof(pf_local_target_tile), &tmp, nullptr);
		}
		if(use_path && use_local_path)
			pf_state = PFS_WALKING;
		else
			pf_state = PFS_NOT_USING;
	}
	ReadFile(file, &next_attack, sizeof(next_attack), &tmp, nullptr);
	ReadFile(file, &timer, sizeof(timer), &tmp, nullptr);
	ReadFile(file, &ignore, sizeof(ignore), &tmp, nullptr);
	ReadFile(file, &morale, sizeof(morale), &tmp, nullptr);
	if(LOAD_VERSION < V_0_3)
	{
		float last_pf_check, last_lpf_check;
		ReadFile(file, &last_pf_check, sizeof(last_pf_check), &tmp, nullptr);
		ReadFile(file, &last_lpf_check, sizeof(last_lpf_check), &tmp, nullptr);
		if(pf_state == PFS_WALKING)
			pf_timer = last_pf_check;
	}
	ReadFile(file, &last_scan, sizeof(last_scan), &tmp, nullptr);
	ReadFile(file, &start_rot, sizeof(start_rot), &tmp, nullptr);
	if(unit->data->spells)
		ReadFile(file, cooldown, sizeof(cooldown), &tmp, nullptr);
	if(state == AIController::Escape)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		if(refid != -1)
		{
			Game& game = Game::Get();
			if(!game.location->outside)
				escape_room = &((InsideLocation*)game.location)->GetLevelData().rooms[refid];
			else
			{
				escape_room = nullptr;
#ifdef IS_DEV
				WARN(Format("%s had escape_room %d.", unit->GetName(), escape_room));
				game.AddGameMsg("Unit had escape room!", 5.f);
#endif
			}
		}
		else
			escape_room = nullptr;
	}
	else if(state == AIController::Cast)
	{
		if(LOAD_VERSION >= V_0_2_10)
			ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		else
			refid = -1;
		cast_target = (Unit*)refid;
		Game::Get().ai_cast_targets.push_back(this);
	}
	ReadFile(file, &have_potion, sizeof(have_potion), &tmp, nullptr);
	ReadFile(file, &potion, sizeof(potion), &tmp, nullptr);
	ReadFile(file, &idle_action, sizeof(idle_action), &tmp, nullptr);
	switch(idle_action)
	{
	case AIController::Idle_None:
	case AIController::Idle_Animation:
		break;
	case AIController::Idle_Rot:
		ReadFile(file, &idle_data.rot, sizeof(idle_data.rot), &tmp, nullptr);
		break;
	case AIController::Idle_Move:
	case AIController::Idle_TrainCombat:
	case AIController::Idle_Pray:
		ReadFile(file, &idle_data.pos, sizeof(idle_data.pos), &tmp, nullptr);
		break;
	case AIController::Idle_Look:
	case AIController::Idle_WalkTo:
	case AIController::Idle_Chat:
	case AIController::Idle_WalkNearUnit:
	case AIController::Idle_MoveAway:
		{
			int refi;
			ReadFile(file, &refi, sizeof(refi), &tmp, nullptr);
			idle_data.unit = Unit::GetByRefid(refi);
		}
		break;
	case AIController::Idle_Use:
	case AIController::Idle_WalkUse:
	case AIController::Idle_WalkUseEat:
		{
			int refi;
			ReadFile(file, &refi, sizeof(refi), &tmp, nullptr);
			idle_data.useable = Useable::GetByRefid(refi);
		}
		break;
	case AIController::Idle_TrainBow:
		ReadFile(file, &idle_data.obj.pos, sizeof(idle_data.obj.pos), &tmp, nullptr);
		ReadFile(file, &idle_data.obj.rot, sizeof(idle_data.obj.rot), &tmp, nullptr);
		Game::Get().ai_bow_targets.push_back(this);
		break;
	case AIController::Idle_MoveRegion:
	case AIController::Idle_RunRegion:
		ReadFile(file, &idle_data.area.id, sizeof(idle_data.area.id), &tmp, nullptr);
		ReadFile(file, &idle_data.area.pos, sizeof(idle_data.area.pos), &tmp, nullptr);
		break;
	default:
		assert(0);
		break;
	}
	ReadFile(file, &city_wander, sizeof(city_wander), &tmp, nullptr);
	ReadFile(file, &loc_timer, sizeof(loc_timer), &tmp, nullptr);
	ReadFile(file, &shoot_yspeed, sizeof(shoot_yspeed), &tmp, nullptr);
	ReadFile(file, &goto_inn, sizeof(goto_inn), &tmp, nullptr);
	change_ai_mode = false;
}

//=================================================================================================
bool AIController::CheckPotion(bool in_combat)
{
	if(unit->action == A_NONE && have_potion > 0)
	{
		float hpp = unit->GetHpp();
		if(hpp < 0.5f || (hpp < 0.75f && !in_combat) || (!equal(hpp, 1.f) && unit->busy == Unit::Busy_Tournament))
		{
			int index = unit->FindHealingPotion();
			if(index == -1)
			{
				if(unit->busy == Unit::Busy_No && unit->IsFollower())
				{
					Game& game = Game::Get();
					game.UnitTalk(*unit, random_string(game.txAiNoHpPot));
				}
				have_potion = 0;
				return false;
			}

			if(unit->ConsumeItem(index) != 3 && this->in_combat)
				state = AIController::Dodge;
			timer = random(1.f, 1.5f);
			
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
