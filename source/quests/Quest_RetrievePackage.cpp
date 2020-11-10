#include "Pch.h"
#include "Quest_RetrievePackage.h"

#include "Game.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "LevelAreaContext.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_RetrievePackage::Start()
{
	type = Q_RETRIEVE_PACKAGE;
	category = QuestCategory::Mayor;
	startLoc = world->GetCurrentLocation();
	from_loc = world->GetRandomSettlement(startLoc)->index;
}

//=================================================================================================
GameDialog* Quest_RetrievePackage::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_retrieve_package_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_retrieve_package_timeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_retrieve_package_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_RetrievePackage::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// received quest from mayor
		{
			OnStart(game->txQuest[265]);
			quest_mgr->quests_timeout.push_back(this);

			targetLoc = world->GetRandomSpawnLocation((startLoc->pos + world->GetLocation(from_loc)->pos) / 2, UnitGroup::Get("bandits"));
			targetLoc->SetKnown();
			targetLoc->active_quest = this;

			cstring who = (LocationHelper::IsCity(startLoc) ? game->txForMayor : game->txForSoltys);

			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(game->txQuest[8], who, startLoc->name.c_str());
			parcel.quest_id = id;
			unit_to_spawn = UnitGroup::Get("bandits")->GetLeader(8);
			unit_spawn_level = -3;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &parcel;
			at_level = targetLoc->GetRandomLevel();

			msgs.push_back(Format(game->txQuest[3], who, startLoc->name.c_str(), world->GetDate()));
			if(targetLoc->type == L_CAMP)
				msgs.push_back(Format(game->txQuest[22], who, startLoc->name.c_str(), GetLocationDirName(startLoc->pos, targetLoc->pos)));
			else
				msgs.push_back(Format(game->txQuest[23], who, startLoc->name.c_str(), targetLoc->name.c_str(), GetLocationDirName(startLoc->pos, targetLoc->pos)));
			st = targetLoc->st;
		}
		break;
	case Progress::Timeout:
		// player failed to retrieve package in time
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->quest_mayor = CityQuestState::Failed;

			if(targetLoc && targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;

			OnUpdate(game->txQuest[24]);
			RemoveElementTry<Quest_Dungeon*>(quest_mgr->quests_timeout, this);
		}
		break;
	case Progress::Finished:
		// player returned package to mayor, end of quest
		{
			state = Quest::Completed;
			int reward = GetReward();
			team->AddReward(reward, reward * 3);

			static_cast<City*>(startLoc)->quest_mayor = CityQuestState::None;
			DialogContext::current->pc->unit->RemoveQuestItem(id);
			if(targetLoc && targetLoc->active_quest == this)
				targetLoc->active_quest = nullptr;

			OnUpdate(game->txQuest[25]);
			RemoveElementTry<Quest_Dungeon*>(quest_mgr->quests_timeout, this);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_RetrievePackage::FormatString(const string& str)
{
	if(str == "burmistrz_od")
		return LocationHelper::IsCity(world->GetLocation(from_loc)) ? game->txQuest[26] : game->txQuest[27];
	else if(str == "locname_od")
		return world->GetLocation(from_loc)->name.c_str();
	else if(str == "locname")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(startLoc->pos, targetLoc->pos);
	else if(str == "reward")
		return Format("%d", GetReward());
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_RetrievePackage::IsTimedout() const
{
	return world->GetWorldtime() - start_time > 30;
}

//=================================================================================================
bool Quest_RetrievePackage::OnTimeout(TimeoutType ttype)
{
	if(done)
	{
		Unit* u = ForLocation(targetLoc, at_level)->FindUnit([](Unit* u) {return u->mark; });
		if(u)
		{
			u->mark = false;
			if(u->IsAlive())
				u->RemoveQuestItem(id);
		}
	}

	OnUpdate(game->txQuest[267]);
	return true;
}

//=================================================================================================
bool Quest_RetrievePackage::IfHaveQuestItem() const
{
	return world->GetCurrentLocation() == startLoc && prog == Progress::Started;
}

//=================================================================================================
const Item* Quest_RetrievePackage::GetQuestItem()
{
	return &parcel;
}

//=================================================================================================
bool Quest_RetrievePackage::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "is_camp") == 0)
		return targetLoc->type == L_CAMP;
	assert(0);
	return false;
}

//=================================================================================================
void Quest_RetrievePackage::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	if(prog != Progress::Finished)
	{
		f << from_loc;
		f << st;
	}
}

//=================================================================================================
Quest::LoadResult Quest_RetrievePackage::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(prog != Progress::Finished)
	{
		f >> from_loc;
		f >> st;

		if(prog >= Progress::Started)
		{
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(startLoc) ? game->txForMayor : game->txForSoltys, startLoc->name.c_str());
			parcel.quest_id = id;

			item_to_give[0] = &parcel;
			unit_to_spawn = UnitGroup::Get("bandits")->GetLeader(8);
			unit_spawn_level = -3;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
int Quest_RetrievePackage::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2500, 5000));
}
