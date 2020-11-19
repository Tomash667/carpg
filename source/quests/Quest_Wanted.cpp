#include "Pch.h"
#include "Quest_Wanted.h"

#include "City.h"
#include "DialogContext.h"
#include "Journal.h"
#include "LevelAreaContext.h"
#include "NameHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Wanted::Start()
{
	startLoc = world->GetCurrentLocation();
	type = Q_WANTED;
	category = QuestCategory::Captain;
	level = Random(5, 15);
	crazy = (Rand() % 5 == 0);
	clas = crazy ? Class::GetRandomCrazy() : Class::GetRandomHero(true);
	target_unit = nullptr;
	in_location = -1;
}

//=================================================================================================
bool Quest_Wanted::OnTimeout(TimeoutType ttype)
{
	if(target_unit)
	{
		if(state == Quest::Failed)
			static_cast<City*>(startLoc)->quest_captain = CityQuestState::Failed;
		if(!target_unit->hero->team_member)
		{
			// not a team member, remove
			ForLocation(in_location)->RemoveUnit(target_unit);
		}
		else
			target_unit->event_handler = nullptr;
		target_unit = nullptr;
	}

	OnUpdate(quest_mgr->txQuest[267]);

	return true;
}

//=================================================================================================
void Quest_Wanted::HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit)
{
	switch(event_type)
	{
	case UnitEventHandler::SPAWN:
		unit->hero->name = unit_name;
		targetLoc->active_quest = nullptr;
		target_unit = unit;
		in_location = world->GetCurrentLocationIndex();
		break;
	case UnitEventHandler::DIE:
		if(!unit->hero->team_member)
		{
			SetProgress(Progress::Killed);
			target_unit = nullptr;
		}
		break;
	case UnitEventHandler::RECRUIT:
		// target recruited, add note to journal
		if(prog != Progress::Recruited)
			SetProgress(Progress::Recruited);
		break;
	case UnitEventHandler::KICK:
		// kicked from team, can be killed now, don't dissapear
		unit->temporary = false;
		in_location = world->GetCurrentLocationIndex();
		break;
	case UnitEventHandler::LEAVE:
		if(state == Quest::Failed)
			static_cast<City*>(startLoc)->quest_captain = CityQuestState::Failed;
		target_unit = nullptr;
		break;
	}
}

//=================================================================================================
void Quest_Wanted::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << level;
	f << crazy;
	f << clas->id;
	f << unit_name;
	f << target_unit;
	f << in_location;
}

//=================================================================================================
Quest::LoadResult Quest_Wanted::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> level;
	f >> crazy;
	if(LOAD_VERSION >= V_0_12)
		clas = Class::TryGet(f.ReadString1());
	else
	{
		int old_clas;
		f >> old_clas;
		clas = old::ConvertOldClass(old_clas);
	}
	f >> unit_name;
	f >> target_unit;
	f >> in_location;

	if(!done)
	{
		unit_to_spawn = crazy ? clas->crazy : clas->hero;
		unit_dont_attack = true;
		unit_event_handler = this;
		send_spawn_event = true;
		unit_spawn_level = level;
	}

	if(prog >= Progress::Started)
	{
		Item::Get("wanted_letter")->CreateCopy(letter);
		letter.id = "$wanted_letter";
		letter.name = quest_mgr->txQuest[258];
		letter.quest_id = id;
		letter.desc = Format(quest_mgr->txQuest[259], level * 100, unit_name.c_str());
	}

	return LoadResult::Ok;
}
