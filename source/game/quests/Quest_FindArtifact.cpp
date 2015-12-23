#include "Pch.h"
#include "Base.h"
#include "Quest_FindArtifact.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"

//-----------------------------------------------------------------------------
DialogEntry find_artifact_start[] = {
	TALK2(117),
	TALK(118),
	CHOICE(119),
		SET_QUEST_PROGRESS(Quest_FindArtifact::Progress::Started),
		TALK2(120),
		TALK(121),
		END,
	END_CHOICE,
	CHOICE(122),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry find_artifact_end[] = {
	SET_QUEST_PROGRESS(Quest_FindArtifact::Progress::Finished),
	TALK(123),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry find_artifact_timeout[] = {
	SET_QUEST_PROGRESS(Quest_FindArtifact::Progress::Timeout),
	TALK(124),
	TALK2(125),
	TALK(126),
	END2,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_FindArtifact::Start()
{
	quest_id = Q_FIND_ARTIFACT;
	type = Type::Random;
	start_loc = game->current_location;
	item = g_artifacts[rand2() % g_artifacts.size()];
}

//=================================================================================================
DialogEntry* Quest_FindArtifact::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return find_artifact_start;
	case QUEST_DIALOG_NEXT:
		return find_artifact_end;
	case QUEST_DIALOG_FAIL:
		return find_artifact_timeout;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_FindArtifact::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[81];

			CreateItemCopy(quest_item, item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.refid = refid;

			Location& sl = *game->locations[start_loc];

			// event
			spawn_item = Quest_Dungeon::Item_InTreasure;
			item_to_give[0] = &quest_item;
			if(rand2()%4 == 0)
			{
				target_loc = game->GetClosestLocation(L_DUNGEON, sl.pos, LABIRYNTH);
				at_level = 0;
			}
			else
			{
				target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
				InsideLocation* inside = (InsideLocation*)(game->locations[target_loc]);
				if(inside->IsMultilevel())
				{
					MultiInsideLocation* multi = (MultiInsideLocation*)inside;
					at_level = multi->levels.size()-1;
				}
				else
					at_level = 0;
			}

			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			quest_index = game->quests.size();
			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[83], item->name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&quest_item, item);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[84]);
			game->AddReward(1000);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;
			game->current_dialog->talker->AddItem(&quest_item, 1, true);
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case Progress::Timeout:
		{
			state = Quest::Failed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[85]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_FindArtifact::FormatString(const string& str)
{
	if(str == "przedmiot")
		return item->name.c_str();
	else if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "random_loc")
		return game->locations[game->GetRandomCityLocation(start_loc)]->name.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_FindArtifact::IsTimedout() const
{
	return game->worldtime - start_time > 60;
}

//=================================================================================================
bool Quest_FindArtifact::OnTimeout(TimeoutType ttype)
{
	if(done)
	{
		InsideLocation& inside = (InsideLocation&)GetTargetLocation();
		inside.RemoveQuestItemFromChest(refid, at_level);
	}

	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_FindArtifact::IfHaveQuestItem2(cstring id) const
{
	return prog == Progress::Started && strcmp(id, "$$artifact") == 0;
}

//=================================================================================================
const Item* Quest_FindArtifact::GetQuestItem()
{
	return &quest_item;
}

//=================================================================================================
void Quest_FindArtifact::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);
	f << item;
}

//=================================================================================================
void Quest_FindArtifact::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);
	f.LoadArtifact(item);

	CreateItemCopy(quest_item, item);
	quest_item.id = Format("$%s", item->id.c_str());
	quest_item.refid = refid;
	spawn_item = Quest_Dungeon::Item_InTreasure;
	item_to_give[0] = &quest_item;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item, item);
}
