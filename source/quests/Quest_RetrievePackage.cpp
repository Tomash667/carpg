#include "Pch.h"
#include "GameCore.h"
#include "Quest_RetrievePackage.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "GameFile.h"
#include "World.h"
#include "Team.h"
#include "ItemHelper.h"
#include "SaveState.h"

//=================================================================================================
void Quest_RetrievePackage::Start()
{
	quest_id = Q_RETRIEVE_PACKAGE;
	type = QuestType::Mayor;
	start_loc = W.GetCurrentLocationIndex();
	from_loc = W.GetRandomSettlementIndex(start_loc);
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
			QM.quests_timeout.push_back(this);

			target_loc = W.GetRandomSpawnLocation((GetStartLocation().pos + W.GetLocation(from_loc)->pos) / 2, UnitGroup::Get("bandits"));

			Location& loc = GetStartLocation();
			Location& loc2 = GetTargetLocation();
			loc2.SetKnown();

			loc2.active_quest = this;

			cstring who = (LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys);

			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(game->txQuest[8], who, loc.name.c_str());
			parcel.refid = refid;
			unit_to_spawn = UnitGroup::Get("bandits")->GetLeader(8);
			unit_spawn_level = -3;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &parcel;
			at_level = loc2.GetRandomLevel();

			msgs.push_back(Format(game->txQuest[3], who, loc.name.c_str(), W.GetDate()));
			if(loc2.type == L_CAMP)
			{
				game->target_loc_is_camp = true;
				msgs.push_back(Format(game->txQuest[22], who, loc.name.c_str(), GetLocationDirName(loc.pos, loc2.pos)));
			}
			else
			{
				game->target_loc_is_camp = false;
				msgs.push_back(Format(game->txQuest[23], who, loc.name.c_str(), loc2.name.c_str(), GetLocationDirName(loc.pos, loc2.pos)));
			}
			st = loc2.st;
		}
		break;
	case Progress::Timeout:
		// player failed to retrieve package in time
		{
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::Failed;

			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}

			OnUpdate(game->txQuest[24]);
			RemoveElementTry<Quest_Dungeon*>(QM.quests_timeout, this);
		}
		break;
	case Progress::Finished:
		// player returned package to mayor, end of quest
		{
			state = Quest::Completed;
			int reward = GetReward();
			Team.AddReward(reward, reward * 3);

			((City&)GetStartLocation()).quest_mayor = CityQuestState::None;
			DialogContext::current->pc->unit->RemoveQuestItem(refid);
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}

			OnUpdate(game->txQuest[25]);
			RemoveElementTry<Quest_Dungeon*>(QM.quests_timeout, this);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_RetrievePackage::FormatString(const string& str)
{
	if(str == "burmistrz_od")
		return LocationHelper::IsCity(W.GetLocation(from_loc)) ? game->txQuest[26] : game->txQuest[27];
	else if(str == "locname_od")
		return W.GetLocation(from_loc)->name.c_str();
	else if(str == "locname")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
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
	return W.GetWorldtime() - start_time > 30;
}

//=================================================================================================
bool Quest_RetrievePackage::OnTimeout(TimeoutType ttype)
{
	if(done)
	{
		Unit* u = ForLocation(target_loc, at_level)->FindUnit([](Unit* u) {return u->mark; });
		if(u)
		{
			u->mark = false;
			if(u->IsAlive())
				u->RemoveQuestItem(refid);
		}
	}

	OnUpdate(game->txQuest[277]);
	return true;
}

//=================================================================================================
bool Quest_RetrievePackage::IfHaveQuestItem() const
{
	return W.GetCurrentLocationIndex() == start_loc && prog == Progress::Started;
}

//=================================================================================================
const Item* Quest_RetrievePackage::GetQuestItem()
{
	return &parcel;
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
bool Quest_RetrievePackage::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(prog != Progress::Finished)
	{
		f >> from_loc;
		if(LOAD_VERSION >= V_0_8)
			f >> st;
		else if(target_loc != -1)
			st = GetTargetLocation().st;
		else
			st = 10;

		if(prog >= Progress::Started)
		{
			Location& loc = GetStartLocation();
			Item::Get("parcel")->CreateCopy(parcel);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			parcel.refid = refid;

			item_to_give[0] = &parcel;
			unit_to_spawn = UnitGroup::Get("bandits")->GetLeader(8);
			unit_spawn_level = -3;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
		}
	}

	return true;
}

//=================================================================================================
int Quest_RetrievePackage::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2500, 5000));
}
