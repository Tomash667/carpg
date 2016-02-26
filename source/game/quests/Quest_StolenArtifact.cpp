#include "Pch.h"
#include "Base.h"
#include "Quest_StolenArtifact.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"

//-----------------------------------------------------------------------------
DialogEntry stolen_artifact_start[] = {
	TALK2(127),
	TALK(128),
	CHOICE(129),
		SET_QUEST_PROGRESS(Quest_StolenArtifact::Progress::Started),
		TALK2(130),
		TALK(131),
		END,
	END_CHOICE,
	CHOICE(132),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry stolen_artifact_end[] = {
	SET_QUEST_PROGRESS(Quest_StolenArtifact::Progress::Finished),
	TALK(133),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry stolen_artifact_timeout[] = {
	SET_QUEST_PROGRESS(Quest_StolenArtifact::Progress::Timeout),
	TALK(134),
	TALK2(135),
	TALK(136),
	END2,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_StolenArtifact::Start()
{
	quest_id = Q_STOLEN_ARTIFACT;
	type = Type::Random;
	start_loc = game->current_location;
	item = g_artifacts[rand2() % g_artifacts.size()];
	switch(rand2()%6)
	{
	case 0:
		group = SG_BANDYCI;
		break;
	case 1:
		group = SG_ORKOWIE;
		break;
	case 2:
		group = SG_GOBLINY;
		break;
	case 3:
	case 4:
		group = SG_MAGOWIE;
		break;
	case 5:
		group = SG_ZLO;
		break;
	}
}

//=================================================================================================
DialogEntry* Quest_StolenArtifact::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return stolen_artifact_start;
	case QUEST_DIALOG_NEXT:
		return stolen_artifact_end;
	case QUEST_DIALOG_FAIL:
		return stolen_artifact_timeout;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_StolenArtifact::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[86];

			CreateItemCopy(quest_item, item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.refid = refid;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &quest_item;
			unit_to_spawn = g_spawn_groups[group].GetSpawnLeader();
			unit_spawn_level = -3;

			Location& sl = *game->locations[start_loc];
			target_loc = game->GetRandomSpawnLocation(sl.pos, group);
			Location& tl = *game->locations[target_loc];
			at_level = tl.GetRandomLevel();
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			cstring kto;
			switch(group)
			{
			case SG_BANDYCI:
				kto = game->txQuest[87];
				break;
			case SG_GOBLINY:
				kto = game->txQuest[88];
				break;
			case SG_ORKOWIE:
				kto = game->txQuest[89];
				break;
			case SG_MAGOWIE:
				kto = game->txQuest[90];
				break;
			case SG_ZLO:
				kto = game->txQuest[91];
				break;
			default:
				kto = game->txQuest[92];
				break;
			}

			quest_index = game->quests.size();
			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[93], item->name.c_str(), kto, tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
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
			msgs.push_back(game->txQuest[94]);
			game->AddReward(1200);
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
			msgs.push_back(game->txQuest[95]);
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
cstring Quest_StolenArtifact::FormatString(const string& str)
{
	if(str == "przedmiot")
		return item->name.c_str();
	else if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "random_loc")
		return game->locations[game->GetRandomCityLocation(start_loc)]->name.c_str();
	else if(str == "Bandyci_ukradli")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[96];
		case SG_ORKOWIE:
			return game->txQuest[97];
		case SG_GOBLINY:
			return game->txQuest[98];
		case SG_MAGOWIE:
			return game->txQuest[99];
		case SG_ZLO:
			return game->txQuest[100];
		default:
			assert(0);
			return nullptr;
		}
	}
	else if(str == "Ci_bandyci")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[101];
		case SG_ORKOWIE:
			return game->txQuest[102];
		case SG_GOBLINY:
			return game->txQuest[103];
		case SG_MAGOWIE:
			return game->txQuest[104];
		case SG_ZLO:
			return game->txQuest[105];
		default:
			assert(0);
			return nullptr;
		}
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_StolenArtifact::IsTimedout() const
{
	return game->worldtime - start_time > 60;
}

//=================================================================================================
bool Quest_StolenArtifact::OnTimeout(TimeoutType ttype)
{
	if(done)
		game->RemoveQuestItemFromUnit(game->ForLevel(target_loc, at_level), refid);

	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_StolenArtifact::IfHaveQuestItem2(cstring id) const
{
	return prog == Progress::Started && strcmp(id, "$$stolen_artifact") == 0;
}

//=================================================================================================
const Item* Quest_StolenArtifact::GetQuestItem()
{
	return &quest_item;
}

//=================================================================================================
void Quest_StolenArtifact::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);
	f << item;
	f << group;
}

//=================================================================================================
void Quest_StolenArtifact::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);
	f.LoadArtifact(item);
	f >> group;

	CreateItemCopy(quest_item, item);
	quest_item.id = Format("$%s", item->id.c_str());
	quest_item.refid = refid;
	spawn_item = Quest_Dungeon::Item_GiveSpawned;
	item_to_give[0] = &quest_item;
	unit_to_spawn = g_spawn_groups[group].GetSpawnLeader();
	unit_spawn_level = -3;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item, item);
}
