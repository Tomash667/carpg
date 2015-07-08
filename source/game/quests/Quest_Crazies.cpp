#include "Pch.h"
#include "Base.h"
#include "Quest_Crazies.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"

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

			game->szaleni_stan = Game::SS_POGADANO_Z_TRENEREM;

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
			GetTargetLocation().active_quest = NULL;

			game->szaleni_stan = Game::SS_KONIEC;

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
		return NULL;
	}
}

//=================================================================================================
bool Quest_Crazies::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "szaleni") == 0;
}
