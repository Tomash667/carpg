#include "Pch.h"
#include "Quest_StolenArtifact.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "World.h"
#include "LevelArea.h"
#include "Team.h"
#include "ItemHelper.h"
#include "SaveState.h"

//=================================================================================================
void Quest_StolenArtifact::Start()
{
	type = Q_STOLEN_ARTIFACT;
	category = QuestCategory::Random;
	start_loc = world->GetCurrentLocationIndex();
	item = OtherItem::artifacts[Rand() % OtherItem::artifacts.size()];
	switch(Rand() % 6)
	{
	case 0:
		group = UnitGroup::Get("bandits");
		break;
	case 1:
		group = UnitGroup::Get("orcs");
		break;
	case 2:
		group = UnitGroup::Get("goblins");
		break;
	case 3:
	case 4:
		group = UnitGroup::Get("mages");
		break;
	case 5:
		group = UnitGroup::Get("evil");
		break;
	}
}

//=================================================================================================
GameDialog* Quest_StolenArtifact::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_stolen_artifact_start");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_stolen_artifact_end");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_stolen_artifact_timeout");
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
			OnStart(game->txQuest[86]);
			quest_mgr->quests_timeout.push_back(this);

			Location& sl = GetStartLocation();
			target_loc = world->GetRandomSpawnLocation(sl.pos, group);
			Location& tl = GetTargetLocation();
			at_level = tl.GetRandomLevel();
			tl.active_quest = this;
			tl.SetKnown();
			st = tl.st;

			item->CreateCopy(quest_item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.quest_id = id;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &quest_item;
			unit_to_spawn = group->GetLeader(tl.st);
			unit_spawn_level = -3;

			DialogContext::current->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[93], item->name.c_str(), group->name.c_str(), game->txQuest[group->gender ? 88 : 87],
				tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
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
			OnUpdate(game->txQuest[94]);
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
			OnUpdate(game->txQuest[95]);
			DialogContext::current->talker->temporary = true;
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
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
	else if(str == "random_loc")
		return world->GetRandomSettlement(start_loc)->name.c_str();
	else if(str == "Bandyci_ukradli")
		return Format("%s %s", Upper(group->name.c_str()), game->txQuest[group->gender ? 97 : 96]);
	else if(str == "Ci_bandyci")
		return Format("%s %s", game->txQuest[group->gender ? 99 : 98], group->name.c_str());
	else if(str == "reward")
		return Format("%d", GetReward());
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_StolenArtifact::IsTimedout() const
{
	return world->GetWorldtime() - start_time > 60;
}

//=================================================================================================
bool Quest_StolenArtifact::OnTimeout(TimeoutType ttype)
{
	if(done)
		ForLocation(target_loc, at_level)->RemoveQuestItemFromUnit(id);

	OnUpdate(game->txQuest[277]);
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
void Quest_StolenArtifact::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << item;
	f << group;
	f << st;
}

//=================================================================================================
Quest::LoadResult Quest_StolenArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> item;
	f >> group;
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
		spawn_item = Quest_Dungeon::Item_GiveSpawned;
		item_to_give[0] = &quest_item;
		unit_to_spawn = group->GetLeader(st);
		unit_spawn_level = -3;
	}

	return LoadResult::Ok;
}

//=================================================================================================
int Quest_StolenArtifact::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2000, 6000));
}
