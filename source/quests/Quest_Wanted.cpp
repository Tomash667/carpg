#include "Pch.h"
#include "Quest_Wanted.h"

#include "City.h"
#include "Game.h"
#include "GameFile.h"
#include "Journal.h"
#include "NameHelper.h"
#include "QuestManager.h"
#include "SaveState.h"
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
GameDialog* Quest_Wanted::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_wanted_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_wanted_timeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_wanted_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Wanted::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(game->txQuest[257]);
			quest_mgr->quests_timeout.push_back(this);

			NameHelper::GenerateHeroName(clas, crazy, unit_name);
			targetLoc = world->GetRandomFreeSettlement(startLoc);
			// if there is no free city he will talk about some random city but there won't be there...
			if(!targetLoc)
				targetLoc = world->GetRandomSettlement(startLoc);
			if(!targetLoc->active_quest)
			{
				targetLoc->active_quest = this;
				unit_to_spawn = crazy ? clas->crazy : clas->hero;
				unit_dont_attack = true;
				unit_event_handler = this;
				send_spawn_event = true;
				unit_spawn_level = level;
			}

			// add letter
			Item::Get("wanted_letter")->CreateCopy(letter);
			letter.id = "$wanted_letter";
			letter.name = game->txQuest[258];
			letter.quest_id = id;
			letter.desc = Format(game->txQuest[259], level * 100, unit_name.c_str());
			DialogContext::current->pc->unit->AddItem2(&letter, 1u, 1u);

			// add journal entry
			msgs.push_back(Format(game->txQuest[29], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[260], level * 100, unit_name.c_str(), GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::Timeout:
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->quest_captain = CityQuestState::Failed;

			if(targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;

			OnUpdate(Format(game->txQuest[261], unit_name.c_str()));

			done = false;
		}
		break;
	case Progress::Killed:
		{
			state = Quest::Started; // if recruited that will change it to in progress
			OnUpdate(Format(game->txQuest[262], unit_name.c_str()));
			RemoveElementTry<Quest_Dungeon*>(quest_mgr->quests_timeout, this);
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			static_cast<City*>(startLoc)->quest_captain = CityQuestState::None;

			team->AddReward(level * 100, level * 250);

			OnUpdate(Format(game->txQuest[263], unit_name.c_str()));
		}
		break;
	case Progress::Recruited:
		{
			state = Quest::Failed;
			OnUpdate(Format(game->txQuest[266], target_unit->GetName()));
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Wanted::FormatString(const string& str)
{
	if(str == "reward")
		return Format("%d", level * 100);
	else if(str == "name")
		return unit_name.c_str();
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "player")
		return DialogContext::current->pc->name.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Wanted::IsTimedout() const
{
	return world->GetWorldtime() - start_time > 30;
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

	OnUpdate(game->txQuest[267]);

	return true;
}

//=================================================================================================
bool Quest_Wanted::IfHaveQuestItem() const
{
	return DialogContext::current->talker == target_unit;
}

//=================================================================================================
bool Quest_Wanted::IfNeedTalk(cstring topic) const
{
	return prog == Progress::Killed && strcmp(topic, "wanted") == 0 && world->GetCurrentLocation() == startLoc;
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
		letter.name = game->txQuest[258];
		letter.quest_id = id;
		letter.desc = Format(game->txQuest[259], level * 100, unit_name.c_str());
	}

	return LoadResult::Ok;
}
