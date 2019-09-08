#include "Pch.h"
#include "GameCore.h"
#include "AIController.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"
#include "InsideLocation.h"
#include "GameFile.h"
#include "Level.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "QuestManager.h"
#include "Quest_Tournament.h"

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
	last_scan = 0.f;
	in_combat = false;
	escape_room = nullptr;
	idle_action = Idle_None;
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
	f << last_scan;
	f << start_rot;
	if(unit->data->spells)
		f << cooldown;
	if(state == AIController::Escape)
		f << (escape_room ? escape_room->index : -1);
	f << have_potion;
	f << have_mp_potion;
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
		f << idle_data.unit;
		break;
	case AIController::Idle_Use:
	case AIController::Idle_WalkUse:
	case AIController::Idle_WalkUseEat:
		f << idle_data.usable->id;
		break;
	case AIController::Idle_TrainBow:
		f << idle_data.obj.pos;
		f << idle_data.obj.rot;
		break;
	case AIController::Idle_MoveRegion:
	case AIController::Idle_RunRegion:
		f << idle_data.region.area->area_id;
		f << idle_data.region.pos;
		f << idle_data.region.exit;
		break;
	default:
		assert(0);
		break;
	}
	f << city_wander;
	f << loc_timer;
	f << shoot_yspeed;
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
	f >> last_scan;
	f >> start_rot;
	if(unit->data->spells)
		f >> cooldown;
	if(state == AIController::Escape)
	{
		int room_id = f.Read<int>();
		if(room_id != -1)
		{
			if(!game_level->location->outside)
				escape_room = ((InsideLocation*)game_level->location)->GetLevelData().rooms[room_id];
			else
			{
				game->ReportError(1, Format("Unit %s has escape_room %d.", unit->GetName(), room_id));
				escape_room = nullptr;
			}
		}
		else
			escape_room = nullptr;
	}
	else if(state == AIController::Cast && LOAD_VERSION < V_DEV)
		f.Skip<int>(); // old cast_target
	f >> have_potion;
	if(LOAD_VERSION >= V_DEV)
		f >> have_mp_potion;
	else
		have_mp_potion = HavePotion::Check;
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
		game->ai_bow_targets.push_back(this);
		break;
	case AIController::Idle_MoveRegion:
	case AIController::Idle_RunRegion:
		{
			int area_id;
			f >> area_id;
			f >> idle_data.region.pos;
			if(LOAD_VERSION >= V_0_11)
			{
				f >> idle_data.region.exit;
				idle_data.region.area = game_level->GetAreaById(area_id);
			}
			else
			{
				if(area_id == LevelArea::OLD_EXIT_ID)
				{
					idle_data.region.exit = true;
					idle_data.region.area = game_level->GetAreaById(LevelArea::OUTSIDE_ID);
				}
				else
				{
					idle_data.region.exit = false;
					idle_data.region.area = game_level->GetAreaById(area_id);
					if(!idle_data.region.area)
						idle_data.region.area = game_level->local_area;
				}
			}
		}
		break;
	default:
		assert(0);
		break;
	}
	f >> city_wander;
	f >> loc_timer;
	f >> shoot_yspeed;
	if(LOAD_VERSION < V_DEV)
	{
		bool goto_inn;
		f >> goto_inn;
		if(goto_inn)
			unit->OrderGoToInn();
	}
	change_ai_mode = false;
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
				state = AIController::Dodge;
			timer = Random(1.f, 1.5f);

			return true;
		}
	}

	if(have_mp_potion != HavePotion::No)
	{
		Class* clas = unit->GetClass();
		if(!clas || !clas->mp_bar)
			have_mp_potion = HavePotion::No;
		else
		{
			float mpp = unit->GetMpp();
			if(mpp < 0.33f || (mpp < 0.66f && !in_combat) || (mpp < 0.80f && unit->busy == Unit::Busy_Tournament))
			{
				int index = unit->FindManaPotion();
				if(index == -1)
				{
					have_mp_potion = HavePotion::No;
					return false;
				}

				if(unit->ConsumeItem(index) != 3 && this->in_combat)
					state = AIController::Dodge;
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
	target = nullptr;
	alert_target = nullptr;
	state = Idle;
	pf_path.clear();
	pf_local_path.clear();
	in_combat = false;
	next_attack = 0.f;
	timer = 0.f;
	ignore = 0.f;
	morale = unit->GetMaxMorale();
	cooldown[0] = 0.f;
	cooldown[1] = 0.f;
	cooldown[2] = 0.f;
	last_scan = 0.f;
	escape_room = nullptr;
	have_potion = HavePotion::Check;
	have_mp_potion = HavePotion::Check;
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
