#include "Pch.h"
#include "Quest_RetrievePackage.h"

#include "DialogContext.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "LocationContext.h"
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
	fromLoc = world->GetRandomSettlement(startLoc)->index;
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
			OnStart(questMgr->txQuest[265]);
			questMgr->questTimeouts.push_back(this);

			targetLoc = world->GetRandomSpawnLocation((startLoc->pos + world->GetLocation(fromLoc)->pos) / 2, UnitGroup::Get("bandits"));
			targetLoc->SetKnown();
			targetLoc->activeQuest = this;

			cstring who = (LocationHelper::IsCity(startLoc) ? questMgr->txForMayor : questMgr->txForSoltys);

			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(questMgr->txQuest[8], who, startLoc->name.c_str());
			parcel.questId = id;
			unitToSpawn = UnitGroup::Get("bandits")->GetLeader(8);
			unitSpawnLevel = -3;
			spawnItem = Quest_Dungeon::Item_GiveSpawned;
			itemToGive[0] = &parcel;
			atLevel = targetLoc->GetRandomLevel();

			msgs.push_back(Format(questMgr->txQuest[3], who, startLoc->name.c_str(), world->GetDate()));
			if(targetLoc->type == L_CAMP)
				msgs.push_back(Format(questMgr->txQuest[22], who, startLoc->name.c_str(), GetLocationDirName(startLoc->pos, targetLoc->pos)));
			else
				msgs.push_back(Format(questMgr->txQuest[23], who, startLoc->name.c_str(), targetLoc->name.c_str(), GetLocationDirName(startLoc->pos, targetLoc->pos)));
			st = targetLoc->st;
		}
		break;
	case Progress::Timeout:
		// player failed to retrieve package in time
		{
			state = Quest::Failed;
			static_cast<City*>(startLoc)->questMayor = CityQuestState::Failed;

			if(targetLoc && targetLoc->activeQuest == this)
				targetLoc->activeQuest = nullptr;

			OnUpdate(questMgr->txQuest[24]);
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);
		}
		break;
	case Progress::Finished:
		// player returned package to mayor, end of quest
		{
			state = Quest::Completed;
			int reward = GetReward();
			team->AddReward(reward, reward * 3);

			static_cast<City*>(startLoc)->questMayor = CityQuestState::None;
			DialogContext::current->pc->unit->RemoveQuestItem(id);
			if(targetLoc && targetLoc->activeQuest == this)
				targetLoc->activeQuest = nullptr;

			OnUpdate(questMgr->txQuest[25]);
			RemoveElementTry<Quest_Dungeon*>(questMgr->questTimeouts, this);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_RetrievePackage::FormatString(const string& str)
{
	if(str == "burmistrz_od")
		return LocationHelper::IsCity(world->GetLocation(fromLoc)) ? questMgr->txQuest[26] : questMgr->txQuest[27];
	else if(str == "locname_od")
		return world->GetLocation(fromLoc)->name.c_str();
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
	return world->GetWorldtime() - startTime >= 30;
}

//=================================================================================================
bool Quest_RetrievePackage::OnTimeout(TimeoutType ttype)
{
	if(done)
	{
		Unit* u = ForLocation(targetLoc, atLevel)->FindUnit([](Unit* u) {return u->mark; });
		if(u)
		{
			u->mark = false;
			if(u->IsAlive())
				u->RemoveQuestItem(id);
		}
	}

	OnUpdate(questMgr->txQuest[267]);
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
		f << fromLoc;
		f << st;
	}
}

//=================================================================================================
Quest::LoadResult Quest_RetrievePackage::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(prog != Progress::Finished)
	{
		f >> fromLoc;
		f >> st;

		if(prog >= Progress::Started)
		{
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(questMgr->txQuest[8], LocationHelper::IsCity(startLoc) ? questMgr->txForMayor : questMgr->txForSoltys, startLoc->name.c_str());
			parcel.questId = id;

			itemToGive[0] = &parcel;
			unitToSpawn = UnitGroup::Get("bandits")->GetLeader(8);
			unitSpawnLevel = -3;
			spawnItem = Quest_Dungeon::Item_GiveSpawned;
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
int Quest_RetrievePackage::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2500, 5000));
}
