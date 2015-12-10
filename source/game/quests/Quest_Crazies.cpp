#include "Pch.h"
#include "Base.h"
#include "Quest_Crazies.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"

//-----------------------------------------------------------------------------
DialogEntry crazies_trainer[] = {
	TALK(684),
	TALK(685),
	TALK(686),
	SET_QUEST_PROGRESS(Quest_Crazies::Progress::KnowLocation),
	TALK2(687),
	TALK2(688),
	END,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_Crazies::Start()
{
	type = Type::Unique;
	quest_id = Q_CRAZIES;
	target_loc = -1;
	name = game->txQuest[253];
	crazies_state = State::None;
	days = 0;
	check_stone = false;
}

//=================================================================================================
DialogEntry* Quest_Crazies::GetDialog(int type2)
{
	return crazies_trainer;
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

			quest_index = game->quests.size();
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			msgs.push_back(game->txQuest[254]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::KnowLocation: // trener powiedzia³ o labiryncie
		{
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -128.f, LABIRYNTH, SG_UNK, false);
			start_loc = game->current_location;
			Location& loc = GetTargetLocation();
			loc.active_quest = this;
			loc.state = LS_KNOWN;
			loc.st = 13;

			crazies_state = State::TalkedTrainer;

			msgs.push_back(Format(game->txQuest[255], game->location->name.c_str(), loc.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::Finished: // schowano kamieñ do skrzyni
		{
			state = Quest::Completed;
			GetTargetLocation().active_quest = nullptr;

			crazies_state = State::End;

			msgs.push_back(game->txQuest[256]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();

			if(game->IsOnline())
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
void Quest_Crazies::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << crazies_state;
	f << days;
	f << check_stone;
}

//=================================================================================================
void Quest_Crazies::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(LOAD_VERSION >= V_0_4)
	{
		GameReader f(file);

		f >> crazies_state;
		f >> days;
		f >> check_stone;
	}
}

//=================================================================================================
void Quest_Crazies::LoadOld(HANDLE file)
{
	int refid;
	GameReader f(file);

	f >> crazies_state;
	f >> refid;
	f >> check_stone;

	// days was missing in save!
	days = 13;
}
