#include "Pch.h"
#include "Base.h"
#include "Quest_Sawmill.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"
#include "QuestManager.h"
#include "GameGui.h"

//=================================================================================================
void Quest_Sawmill::Start()
{
	quest_id = Q_SAWMILL;
	type = QuestType::Unique;
	sawmill_state = State::None;
	build_state = BuildState::None;
	days = 0;
}

//=================================================================================================
GameDialog* Quest_Sawmill::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(game->current_dialog->talker->data->id == "artur_drwal")
			return FindDialog("q_sawmill_talk");
		else
			return FindDialog("q_sawmill_messenger");
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Sawmill::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::NotAccepted:
		// nie zaakceptowano
		break;
	case Progress::Started:
		// zakceptowano
		{
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[124];

			location_event_handler = this;

			Location& sl = GetStartLocation();
			target_loc = game->GetClosestLocation(L_FOREST, sl.pos);
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			else if(tl.state >= LS_ENTERED)
				tl.reset = true;
			tl.st = 8;

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[125], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[126], tl.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::ClearedLocation:
		// oczyszczono
		{
			msgs.push_back(Format(game->txQuest[127], GetTargetLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Talked:
		// poinformowano
		{
			days = 0;
			sawmill_state = State::InBuild;

			quest_manager.RemoveQuestRumor(P_TARTAK);
			
			msgs.push_back(game->txQuest[128]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// pierwsza kasa
		{
			state = Quest::Completed;
			sawmill_state = State::Working;
			days = 0;

			msgs.push_back(game->txQuest[129]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(400);
			quest_manager.EndUniqueQuest();
			game->AddNews(Format(game->txQuest[130], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Sawmill::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Sawmill::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "tartak") == 0;
}

//=================================================================================================
bool Quest_Sawmill::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "czy_tartak") == 0)
		return game->current_location == target_loc;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_Sawmill::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::Started && event == LocationEventHandler::CLEARED)
		SetProgress(Progress::ClearedLocation);
}

//=================================================================================================
void Quest_Sawmill::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << sawmill_state;
	f << build_state;
	f << days;
	f << messenger;
	if(sawmill_state != State::None && build_state != BuildState::Finished)
		f << hd_lumberjack;
}

//=================================================================================================
void Quest_Sawmill::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	location_event_handler = this;

	if(LOAD_VERSION >= V_0_4)
	{
		GameReader f(file);

		f >> sawmill_state;
		f >> build_state;
		f >> days;
		f >> messenger;
		if(sawmill_state != State::None && build_state != BuildState::Finished)
			f >> hd_lumberjack;
	}
}

//=================================================================================================
void Quest_Sawmill::LoadOld(HANDLE file)
{
	GameReader f(file);
	int city, forest;

	f >> city;
	f >> sawmill_state;
	f >> build_state;
	f >> days;
	f >> refid;
	f >> forest;
	f >> messenger; 
	if(sawmill_state != State::None && build_state != BuildState::InProgress)
		f >> hd_lumberjack;
	else if(sawmill_state != State::None && build_state == BuildState::InProgress)
	{
		// fix for missing human data
		Unit* u = game->FindUnit(game->ForLevel(target_loc), FindUnitData("artur_drwal"));
		if(u)
			hd_lumberjack.Get(*u->human_data);
		else
			hd_lumberjack.Random();
	}
}
