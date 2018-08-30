#include "Pch.h"
#include "GameCore.h"
#include "Quest_RetrievePackage.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "GameGui.h"
#include "GameFile.h"
#include "World.h"

//=================================================================================================
void Quest_RetrievePackage::Start()
{
	quest_id = Q_RETRIEVE_PACKAGE;
	type = QuestType::Mayor;
	start_loc = game->current_location;
	from_loc = W.GetRandomSettlementIndex(start_loc);
}

//=================================================================================================
GameDialog* Quest_RetrievePackage::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_retrieve_package_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_retrieve_package_timeout");
	case QUEST_DIALOG_NEXT:
		return FindDialog("q_retrieve_package_end");
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
			target_loc = game->GetRandomSpawnLocation((W.locations[start_loc]->pos + W.locations[from_loc]->pos) / 2, SG_BANDYCI);

			Location& loc = *W.locations[start_loc];
			Location& loc2 = *W.locations[target_loc];
			bool now_known = false;
			if(loc2.state == LS_UNKNOWN)
			{
				loc2.state = LS_KNOWN;
				now_known = true;
			}

			loc2.active_quest = this;

			cstring who = (LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys);

			const Item* base_item = Item::Get("parcel");
			game->PreloadItem(base_item);
			CreateItemCopy(parcel, base_item);
			parcel.id = "$stolen_parcel";
			parcel.name = Format(game->txQuest[8], who, loc.name.c_str());
			parcel.refid = refid;
			unit_to_spawn = g_spawn_groups[SG_BANDYCI].GetSpawnLeader();
			unit_spawn_level = -3;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &parcel;
			at_level = loc2.GetRandomLevel();

			start_time = W.GetWorldtime();
			state = Quest::Started;
			name = game->txQuest[265];

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

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			quest_manager.quests_timeout.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&parcel, base_item);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::Timeout:
		// player failed to retrieve package in time
		{
			state = Quest::Failed;
			((City*)W.locations[start_loc])->quest_mayor = CityQuestState::Failed;

			if(target_loc != -1)
			{
				Location& loc = *W.locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}

			msgs.push_back(game->txQuest[24]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// player returned package to mayor, end of quest
		{
			state = Quest::Completed;
			game->AddReward(500);

			((City*)W.locations[start_loc])->quest_mayor = CityQuestState::None;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			if(target_loc != -1)
			{
				Location& loc = *W.locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}

			msgs.push_back(game->txQuest[25]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);

			if(Net::IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	}
}

//=================================================================================================
cstring Quest_RetrievePackage::FormatString(const string& str)
{
	if(str == "burmistrz_od")
		return LocationHelper::IsCity(W.locations[from_loc]) ? game->txQuest[26] : game->txQuest[27];
	else if(str == "locname_od")
		return W.locations[from_loc]->name.c_str();
	else if(str == "locname")
		return W.locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(W.locations[start_loc]->pos, W.locations[target_loc]->pos);
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
		int at_lvl = at_level;
		Unit* u = W.locations[target_loc]->FindUnit(g_spawn_groups[SG_BANDYCI].GetSpawnLeader(), at_lvl);
		if(u && u->IsAlive())
			u->RemoveQuestItem(refid);
	}

	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_RetrievePackage::IfHaveQuestItem() const
{
	return game->current_location == start_loc && prog == Progress::Started;
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
		f << from_loc;
}

//=================================================================================================
bool Quest_RetrievePackage::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(prog != Progress::Finished)
	{
		f >> from_loc;

		const Item* base_item = Item::Get("parcel");
		Location& loc = *W.locations[start_loc];
		CreateItemCopy(parcel, base_item);
		parcel.id = "$stolen_parcel";
		parcel.name = Format(game->txQuest[8], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.refid = refid;

		if(game->mp_load)
			game->Net_RegisterItem(&parcel, base_item);

		item_to_give[0] = &parcel;
		unit_to_spawn = g_spawn_groups[SG_BANDYCI].GetSpawnLeader();
		unit_spawn_level = -3;
		spawn_item = Quest_Dungeon::Item_GiveSpawned;
	}
	
	return true;
}
