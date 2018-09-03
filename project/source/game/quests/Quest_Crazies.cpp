#include "Pch.h"
#include "GameCore.h"
#include "Quest_Crazies.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "GameGui.h"
#include "World.h"

//=================================================================================================
void Quest_Crazies::Start()
{
	type = QuestType::Unique;
	quest_id = Q_CRAZIES;
	target_loc = -1;
	name = game->txQuest[253];
	crazies_state = State::None;
	days = 0;
	check_stone = false;
}

//=================================================================================================
GameDialog* Quest_Crazies::GetDialog(int type2)
{
	return FindDialog("q_crazies_trainer");
}

//=================================================================================================
void Quest_Crazies::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started: // zaatakowano przez unk
		{
			state = Quest::Started;

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[170], W.GetDate()));
			msgs.push_back(game->txQuest[254]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::KnowLocation: // trener powiedzia� o labiryncie
		{
			start_loc = W.current_location_index;
			Location& loc = *W.CreateLocation(L_DUNGEON, Vec2(0, 0), -128.f, LABIRYNTH, SG_UNKNOWN, false);
			loc.active_quest = this;
			loc.SetKnown();
			loc.st = 13;
			target_loc = loc.index;

			crazies_state = State::TalkedTrainer;

			msgs.push_back(Format(game->txQuest[255], W.current_location->name.c_str(), loc.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished: // schowano kamie� do skrzyni
		{
			state = Quest::Completed;
			GetTargetLocation().active_quest = nullptr;

			crazies_state = State::End;

			msgs.push_back(game->txQuest[256]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			quest_manager.EndUniqueQuest();

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
	}
}

//=================================================================================================
cstring Quest_Crazies::FormatString(const string& str)
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
bool Quest_Crazies::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "szaleni") == 0;
}

//=================================================================================================
void Quest_Crazies::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << crazies_state;
	f << days;
	f << check_stone;
}

//=================================================================================================
bool Quest_Crazies::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(LOAD_VERSION >= V_0_4)
	{
		f >> crazies_state;
		f >> days;
		f >> check_stone;
	}

	return true;
}

//=================================================================================================
void Quest_Crazies::LoadOld(GameReader& f)
{
	int old_refid;
	f >> crazies_state;
	f >> old_refid;
	f >> check_stone;

	// days was missing in save!
	days = 13;
}
