#include "Pch.h"
#include "Base.h"
#include "MainQuest.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"

//-----------------------------------------------------------------------------
DialogEntry dialog_main[] = {
	TALK2(1307),
	TALK(1308),
	TALK2(1309),
	TALK(1310),
	SET_QUEST_PROGRESS(1),
	TALK(1311),
	END
};

//=================================================================================================
void MainQuest::Start()
{
	start_loc = game->current_location;
	quest_id = Q_MAIN;
	type = Type::Unique;
	name = game->txQuest[9];
}

//=================================================================================================
DialogEntry* MainQuest::GetDialog(int type2)
{
	return dialog_main;
}

//=================================================================================================
void MainQuest::SetProgress(int prog2)
{
	prog = prog2;

	switch(prog)
	{
	case Progress::Started:
		{
			state = Quest::Started;

			msgs.push_back(Format(game->txQuest[170], game->day + 1, game->month + 1, game->year));
			msgs.push_back(Format(game->txQuest[267], GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);		

			if(game->IsOnline())
			{
				NetChange& c = Add1(game->net_changes);
				c.type = NetChange::ADD_QUEST_MAIN;
				c.id = refid;
			}
		}
		break;
	case Progress::TalkedWithMayor:
		{
			game->AddReward(100 * game->active_team.size());
			const Item* letter = FindItem("q_main_letter");
			game->current_dialog->pc->unit->AddItem(letter, 1, true);

			close_loc = game->GetRandomCity(start_loc);
			Location& close = *game->locations[close_loc];
			target_loc = game->CreateLocation(L_ACADEMY, close.pos, 64.f, -1, SG_LOSOWO, false);
			Location& target = *game->locations[target_loc];
			target.state = LS_KNOWN;
			msgs.push_back(Format(game->txQuest[268], GetLocationDirName(close.pos, target.pos), close.name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, letter, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	}
}

//=================================================================================================
cstring MainQuest::FormatString(const string& str)
{
	if(str == "player_name")
		return game->current_dialog->pc->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[close_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "close_name")
		return game->locations[close_loc]->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

//=================================================================================================
bool MainQuest::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "main") == 0;
}

//=================================================================================================
void MainQuest::Save(HANDLE file)
{
	Quest::Save(file);

	if(prog == Progress::TalkedWithMayor)
	{
		File f(file);
		f << close_loc;
		f << target_loc;
	}
}

//=================================================================================================
void MainQuest::Load(HANDLE file)
{
	Quest::Load(file);

	if(prog == Progress::TalkedWithMayor)
	{
		File f(file);
		f >> close_loc;
		f >> target_loc;
	}
}
