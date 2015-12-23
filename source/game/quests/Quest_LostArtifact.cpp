#include "Pch.h"
#include "Base.h"
#include "Quest_LostArtifact.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"

//-----------------------------------------------------------------------------
DialogEntry lost_artifact_start[] = {
	TALK2(137),
	TALK(138),
	CHOICE(139),
		SET_QUEST_PROGRESS(Quest_LostArtifact::Progress::Started),
		TALK2(140),
		TALK(141),
		END,
	END_CHOICE,
	CHOICE(142),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry lost_artifact_end[] = {
	SET_QUEST_PROGRESS(Quest_LostArtifact::Progress::Finished),
	TALK(143),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry lost_artifact_timeout[] = {
	SET_QUEST_PROGRESS(Quest_LostArtifact::Progress::Timeout),
	TALK(144),
	TALK2(145),
	TALK(146),
	END2,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_LostArtifact::Start()
{
	quest_id = Q_LOST_ARTIFACT;
	type = Type::Random;
	start_loc = game->current_location;
	item = g_artifacts[rand2() % g_artifacts.size()];
}

//=================================================================================================
DialogEntry* Quest_LostArtifact::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return lost_artifact_start;
	case QUEST_DIALOG_NEXT:
		return lost_artifact_end;
	case QUEST_DIALOG_FAIL:
		return lost_artifact_timeout;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_LostArtifact::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[106];

			CreateItemCopy(quest_item, item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.refid = refid;

			Location& sl = *game->locations[start_loc];

			// event
			spawn_item = Quest_Dungeon::Item_OnGround;
			item_to_give[0] = &quest_item;
			if(rand2()%2 == 0)
				target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
			else
				target_loc = game->GetClosestLocationNotTarget(L_DUNGEON, sl.pos, LABIRYNTH);
			Location& tl = *game->locations[target_loc];
			at_level = tl.GetRandomLevel();

			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			cstring poziom;
			switch(at_level)
			{
			case 0:
				poziom = game->txQuest[107];
				break;
			case 1:
				poziom = game->txQuest[108];
				break;
			case 2:
				poziom = game->txQuest[109];
				break;
			case 3:
				poziom = game->txQuest[110];
				break;
			case 4:
				poziom = game->txQuest[111];
				break;
			case 5:
				poziom = game->txQuest[112];
				break;
			default:
				poziom = game->txQuest[113];
				break;
			}

			quest_index = game->quests.size();
			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[114], item->name.c_str(), poziom, tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
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
			msgs.push_back(game->txQuest[115]);
			game->AddReward(800);
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
			msgs.push_back(game->txQuest[116]);
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
cstring Quest_LostArtifact::FormatString(const string& str)
{
	if(str == "przedmiot")
		return item->name.c_str();
	else if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "random_loc")
		return game->locations[game->GetRandomCityLocation(start_loc)]->name.c_str();
	else if(str == "poziomie")
	{
		switch(at_level)
		{
		case 0:
			return game->txQuest[117];
		case 1:
			return game->txQuest[118];
		case 2:
			return game->txQuest[119];
		case 3:
			return game->txQuest[120];
		case 4:
			return game->txQuest[121];
		case 5:
			return game->txQuest[122];
		default:
			return game->txQuest[123];
		}
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_LostArtifact::IsTimedout() const
{
	return game->worldtime - start_time > 60;
}

//=================================================================================================
bool Quest_LostArtifact::OnTimeout(TimeoutType ttype)
{
	if(done)
		game->RemoveQuestGroundItem(game->ForLevel(target_loc, at_level), refid);

	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_LostArtifact::IfHaveQuestItem2(cstring id) const
{
	return prog == Progress::Started && strcmp(id, "$$lost_item") == 0;
}

//=================================================================================================
const Item* Quest_LostArtifact::GetQuestItem()
{
	return &quest_item;
}

//=================================================================================================
void Quest_LostArtifact::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);
	f << item;
}

//=================================================================================================
void Quest_LostArtifact::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);
	f.LoadArtifact(item);

	CreateItemCopy(quest_item, item);
	quest_item.id = Format("$%s", item->id.c_str());
	quest_item.refid = refid;
	spawn_item = Quest_Dungeon::Item_OnGround;
	item_to_give[0] = &quest_item;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item, item);
}
