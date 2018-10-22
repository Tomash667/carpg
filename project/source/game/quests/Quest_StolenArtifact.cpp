#include "Pch.h"
#include "GameCore.h"
#include "Quest_StolenArtifact.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "World.h"
#include "LevelArea.h"

//=================================================================================================
void Quest_StolenArtifact::Start()
{
	quest_id = Q_STOLEN_ARTIFACT;
	type = QuestType::Random;
	start_loc = W.GetCurrentLocationIndex();
	item = OtherItem::artifacts[Rand() % OtherItem::artifacts.size()];
	switch(Rand() % 6)
	{
	case 0:
		group = SG_BANDITS;
		break;
	case 1:
		group = SG_ORCS;
		break;
	case 2:
		group = SG_GOBLINS;
		break;
	case 3:
	case 4:
		group = SG_MAGES;
		break;
	case 5:
		group = SG_EVIL;
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
			quest_manager.quests_timeout.push_back(this);

			item->CreateCopy(quest_item);
			quest_item.id = Format("$%s", item->id.c_str());
			quest_item.refid = refid;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &quest_item;
			unit_to_spawn = g_spawn_groups[group].GetSpawnLeader();
			unit_spawn_level = -3;

			Location& sl = GetStartLocation();
			target_loc = W.GetRandomSpawnLocation(sl.pos, group);
			Location& tl = GetTargetLocation();
			at_level = tl.GetRandomLevel();
			tl.active_quest = this;
			tl.SetKnown();

			cstring kto;
			switch(group)
			{
			case SG_BANDITS:
				kto = game->txQuest[87];
				break;
			case SG_GOBLINS:
				kto = game->txQuest[88];
				break;
			case SG_ORCS:
				kto = game->txQuest[89];
				break;
			case SG_MAGES:
				kto = game->txQuest[90];
				break;
			case SG_EVIL:
				kto = game->txQuest[91];
				break;
			default:
				kto = game->txQuest[92];
				break;
			}

			DialogContext::current->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[93], item->name.c_str(), kto, tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
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
			OnUpdate(game->txQuest[94]);
			game->AddReward(1200);
			DialogContext::current->talker->temporary = true;
			DialogContext::current->talker->AddItem(&quest_item, 1, true);
			DialogContext::current->pc->unit->RemoveQuestItem(refid);
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
		return W.GetRandomSettlement(start_loc)->name.c_str();
	else if(str == "Bandyci_ukradli")
	{
		switch(group)
		{
		case SG_BANDITS:
			return game->txQuest[96];
		case SG_ORCS:
			return game->txQuest[97];
		case SG_GOBLINS:
			return game->txQuest[98];
		case SG_MAGES:
			return game->txQuest[99];
		case SG_EVIL:
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
		case SG_BANDITS:
			return game->txQuest[101];
		case SG_ORCS:
			return game->txQuest[102];
		case SG_GOBLINS:
			return game->txQuest[103];
		case SG_MAGES:
			return game->txQuest[104];
		case SG_EVIL:
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
	return W.GetWorldtime() - start_time > 60;
}

//=================================================================================================
bool Quest_StolenArtifact::OnTimeout(TimeoutType ttype)
{
	if(done)
		ForLocation(target_loc, at_level)->RemoveQuestItemFromUnit(refid);

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
}

//=================================================================================================
bool Quest_StolenArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f.LoadArtifact(item);
	f >> group;

	item->CreateCopy(quest_item);
	quest_item.id = Format("$%s", item->id.c_str());
	quest_item.refid = refid;
	spawn_item = Quest_Dungeon::Item_GiveSpawned;
	item_to_give[0] = &quest_item;
	unit_to_spawn = g_spawn_groups[group].GetSpawnLeader();
	unit_spawn_level = -3;

	return true;
}
