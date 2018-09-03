#include "Pch.h"
#include "GameCore.h"
#include "Quest_FindArtifact.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "GameGui.h"
#include "World.h"

//=================================================================================================
void Quest_FindArtifact::Start()
{
	quest_id = Q_FIND_ARTIFACT;
	type = QuestType::Random;
	start_loc = W.GetCurrentLocationIndex();
	item = OtherItem::artifacts[Rand() % OtherItem::artifacts.size()];
}

//=================================================================================================
GameDialog* Quest_FindArtifact::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_find_artifact_start");
	case QUEST_DIALOG_NEXT:
		return FindDialog("q_find_artifact_end");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_find_artifact_timeout");
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
			start_time = W.GetWorldtime();
			state = Quest::Started;
			name = game->txQuest[81];

			CreateItemCopy(quest_item, item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.refid = refid;

			Location& sl = GetStartLocation();

			// event
			spawn_item = Quest_Dungeon::Item_InTreasure;
			item_to_give[0] = &quest_item;
			if(Rand() % 4 == 0)
			{
				target_loc = W.GetClosestLocation(L_DUNGEON, sl.pos, LABIRYNTH);
				at_level = 0;
			}
			else
			{
				target_loc = W.GetClosestLocation(L_CRYPT, sl.pos);
				InsideLocation& inside = (InsideLocation&)GetTargetLocation();
				if(inside.IsMultilevel())
				{
					MultiInsideLocation& multi = (MultiInsideLocation&)inside;
					at_level = multi.levels.size() - 1;
				}
				else
					at_level = 0;
			}

			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.SetKnown();

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			quest_manager.quests_timeout.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[83], item->name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&quest_item, item);
			}
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			msgs.push_back(game->txQuest[84]);
			game->AddReward(1000);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;
			game->current_dialog->talker->AddItem(&quest_item, 1, true);
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			if(Net::IsOnline())
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
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			msgs.push_back(game->txQuest[85]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;

			if(Net::IsOnline())
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
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
	else if(str == "random_loc")
		return W.GetRandomSettlement(start_loc)->name.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_FindArtifact::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 60;
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
void Quest_FindArtifact::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << item;
}

//=================================================================================================
bool Quest_FindArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f.LoadArtifact(item);

	CreateItemCopy(quest_item, item);
	quest_item.id = Format("$%s", item->id.c_str());
	quest_item.refid = refid;
	spawn_item = Quest_Dungeon::Item_InTreasure;
	item_to_give[0] = &quest_item;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item, item);

	return true;
}
