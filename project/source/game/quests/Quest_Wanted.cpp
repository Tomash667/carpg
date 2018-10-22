#include "Pch.h"
#include "GameCore.h"
#include "Quest_Wanted.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "City.h"
#include "World.h"
#include "NameHelper.h"

//=================================================================================================
void Quest_Wanted::Start()
{
	start_loc = W.GetCurrentLocationIndex();
	quest_id = Q_WANTED;
	type = QuestType::Captain;
	level = Random(5, 15);
	crazy = (Rand() % 5 == 0);
	clas = ClassInfo::GetRandomEvil();
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
	case Progress::Started: // zaakceptowano
		{
			OnStart(game->txQuest[257]);
			quest_manager.quests_timeout.push_back(this);

			NameHelper::GenerateHeroName(clas, crazy, unit_name);
			target_loc = W.GetRandomFreeSettlementIndex(start_loc);
			// je�li nie ma wolnego miasta to powie jakie� ale go tam nie b�dzie...
			if(target_loc == -1)
				target_loc = W.GetRandomSettlementIndex(start_loc);
			Location& target = GetTargetLocation();
			if(!target.active_quest)
			{
				target.active_quest = this;
				unit_to_spawn = &ClassInfo::GetUnitData(clas, crazy);
				unit_dont_attack = true;
				unit_event_handler = this;
				send_spawn_event = true;
				unit_spawn_level = level;
			}

			// dodaj list
			Item::Get("wanted_letter")->CreateCopy(letter);
			letter.id = "$wanted_letter";
			letter.name = game->txQuest[258];
			letter.refid = refid;
			letter.desc = Format(game->txQuest[259], level * 100, unit_name.c_str());
			DialogContext::current->pc->unit->AddItem2(&letter, 1u, 1u);

			// wpis do dziennika
			msgs.push_back(Format(game->txQuest[29], GetStartLocationName(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[260], level * 100, unit_name.c_str(), GetTargetLocationName(), GetTargetLocationDir()));
		}
		break;
	case Progress::Timeout: // czas min��
		{
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;

			Location& target = GetTargetLocation();
			if(target.active_quest == this)
				target.active_quest = nullptr;

			OnUpdate(Format(game->txQuest[261], unit_name.c_str()));

			done = false;
		}
		break;
	case Progress::Killed: // zabito
		{
			state = Quest::Started; // if recruited that will change it to in progress
			OnUpdate(Format(game->txQuest[262], unit_name.c_str()));
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
		}
		break;
	case Progress::Finished: // wykonano
		{
			state = Quest::Completed;
			((City&)GetStartLocation()).quest_captain = CityQuestState::None;

			game->AddReward(level * 100);

			OnUpdate(Format(game->txQuest[263], unit_name.c_str()));
		}
		break;
	case Progress::Recruited:
		{
			state = Quest::Failed;
			OnUpdate(Format(game->txQuest[276], target_unit->GetName()));
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
	return W.GetWorldtime() - start_time > 30;
}

//=================================================================================================
bool Quest_Wanted::OnTimeout(TimeoutType ttype)
{
	if(target_unit)
	{
		if(state == Quest::Failed)
			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
		if(!target_unit->hero->team_member)
		{
			// not a team member, remove
			ForLocation(in_location)->RemoveUnit(target_unit);
		}
		else
			target_unit->event_handler = nullptr;
		target_unit = nullptr;
	}

	OnUpdate(game->txQuest[277]);

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
	return prog == Progress::Killed && strcmp(topic, "wanted") == 0;
}

//=================================================================================================
void Quest_Wanted::HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit)
{
	switch(event_type)
	{
	case UnitEventHandler::SPAWN:
		unit->hero->name = unit_name;
		GetTargetLocation().active_quest = nullptr;
		target_unit = unit;
		in_location = W.GetCurrentLocationIndex();
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
		in_location = W.GetCurrentLocationIndex();
		break;
	case UnitEventHandler::LEAVE:
		if(state == Quest::Failed)
			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
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
	f << clas;
	f << unit_name;
	f << target_unit;
	f << in_location;
}

//=================================================================================================
bool Quest_Wanted::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> level;
	f >> crazy;
	f >> clas;
	if(LOAD_VERSION < V_0_4)
		clas = ClassInfo::OldToNew(clas);
	f >> unit_name;
	f >> target_unit;
	if(LOAD_VERSION >= V_0_4)
		f >> in_location;
	else if(!target_unit || target_unit->hero->team_member)
		in_location = -1;
	else
		in_location = W.FindWorldUnit(target_unit, target_loc, W.GetCurrentLocationIndex());

	if(!done)
	{
		unit_to_spawn = &ClassInfo::GetUnitData(clas, crazy);
		unit_dont_attack = true;
		unit_event_handler = this;
		send_spawn_event = true;
		unit_spawn_level = level;
	}

	// list
	Item::Get("wanted_letter")->CreateCopy(letter);
	letter.id = "$wanted_letter";
	letter.name = game->txQuest[258];
	letter.refid = refid;
	letter.desc = Format(game->txQuest[259], level * 100, unit_name.c_str());

	return true;
}
