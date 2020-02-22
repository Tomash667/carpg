#include "Pch.h"
#include "Quest_FindArtifact.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "World.h"
#include "Team.h"
#include "ItemHelper.h"
#include "SaveState.h"

//=================================================================================================
void Quest_FindArtifact::Start()
{
	type = Q_FIND_ARTIFACT;
	category = QuestCategory::Random;
	start_loc = world->GetCurrentLocationIndex();
	item = OtherItem::artifacts[Rand() % OtherItem::artifacts.size()];
}

//=================================================================================================
GameDialog* Quest_FindArtifact::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_find_artifact_start");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_find_artifact_end");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_find_artifact_timeout");
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
			OnStart(game->txQuest[81]);
			quest_mgr->quests_timeout.push_back(this);

			item->CreateCopy(quest_item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.quest_id = id;

			Location& sl = GetStartLocation();

			// event
			spawn_item = Quest_Dungeon::Item_InTreasure;
			item_to_give[0] = &quest_item;
			if(Rand() % 4 == 0)
			{
				target_loc = world->GetClosestLocation(L_DUNGEON, sl.pos, LABYRINTH);
				at_level = 0;
			}
			else
			{
				target_loc = world->GetClosestLocation(L_DUNGEON, sl.pos, { HERO_CRYPT, MONSTER_CRYPT });
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
			st = tl.st;

			DialogContext::current->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[83], item->name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
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
			RemoveElementTry<Quest_Dungeon*>(quest_mgr->quests_timeout, this);
			OnUpdate(game->txQuest[84]);
			int reward = GetReward();
			team->AddReward(reward, reward * 3);
			DialogContext::current->talker->temporary = true;
			DialogContext::current->talker->AddItem(&quest_item, 1, true);
			DialogContext::current->pc->unit->RemoveQuestItem(id);
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
			RemoveElementTry<Quest_Dungeon*>(quest_mgr->quests_timeout, this);
			OnUpdate(game->txQuest[85]);
			DialogContext::current->talker->temporary = true;
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
		return world->GetRandomSettlement(start_loc)->name.c_str();
	else if(str == "reward")
		return Format("%d", GetReward());
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_FindArtifact::IsTimedout() const
{
	return world->GetWorldtime() - start_time > 60;
}

//=================================================================================================
bool Quest_FindArtifact::OnTimeout(TimeoutType ttype)
{
	if(done)
	{
		InsideLocation& inside = (InsideLocation&)GetTargetLocation();
		inside.RemoveQuestItemFromChest(id, at_level);
	}

	OnUpdate(game->txQuest[277]);
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
	f << st;
}

//=================================================================================================
Quest::LoadResult Quest_FindArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> item;
	if(LOAD_VERSION >= V_0_8)
		f >> st;
	else if(target_loc != -1)
		st = GetTargetLocation().st;
	else
		st = 10;

	if(prog >= Progress::Started)
	{
		item->CreateCopy(quest_item);
		quest_item.id = Format("$%s", item->id.c_str());
		quest_item.quest_id = id;
		spawn_item = Quest_Dungeon::Item_InTreasure;
		item_to_give[0] = &quest_item;
	}

	return LoadResult::Ok;
}

//=================================================================================================
int Quest_FindArtifact::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2000, 6000));
}
