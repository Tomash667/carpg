#include "Pch.h"
#include "Base.h"
#include "Quest_RetrivePackage.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"

//-----------------------------------------------------------------------------
DialogEntry retrive_package_start[] = {
	TALK2(50),
	CHOICE(51),
		SET_QUEST_PROGRESS(Quest_RetrivePackage::Progress::Started),
		IF_SPECIAL("is_camp"),
			TALK2(52),
		ELSE,
			TALK2(53),
		END_IF,
		TALK(54),
		END,
	END_CHOICE,
	CHOICE(55),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry retrive_package_timeout[] = {
	TALK(56),
	SET_QUEST_PROGRESS(Quest_RetrivePackage::Progress::Timeout),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry retrive_package_end[] = {
	TALK(57),
	TALK(58),
	SET_QUEST_PROGRESS(Quest_RetrivePackage::Progress::Finished),
	END,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_RetrivePackage::Start()
{
	quest_id = Q_RETRIVE_PACKAGE;
	type = Type::Mayor;
	start_loc = game->current_location;
	from_loc = game->GetRandomCityLocation(start_loc);
}

//=================================================================================================
DialogEntry* Quest_RetrivePackage::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return retrive_package_start;
	case QUEST_DIALOG_FAIL:
		return retrive_package_timeout;
	case QUEST_DIALOG_NEXT:
		return retrive_package_end;
	default:
		assert(0);
		return NULL;
	}
}

//=================================================================================================
void Quest_RetrivePackage::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// received quest from mayor
		{
			target_loc = game->GetRandomSpawnLocation((game->locations[start_loc]->pos + game->locations[from_loc]->pos)/2, SG_BANDYCI);

			Location& loc = *game->locations[start_loc];
			Location& loc2 = *game->locations[target_loc];
			bool now_known = false;
			if(loc2.state == LS_UNKNOWN)
			{
				loc2.state = LS_KNOWN;
				now_known = true;
			}

			loc2.active_quest = this;

			cstring who = (loc.type == L_CITY ? game->txForMayor : game->txForSoltys);

			parcel.name = Format(game->txQuest[8], who, loc.name.c_str());
			parcel.type = IT_OTHER;
			parcel.weight = 10;
			parcel.value = 0;
			parcel.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			parcel.id = "$stolen_parcel";
			parcel.mesh.clear();
			parcel.tex = game->tPaczka;
			parcel.refid = refid;
			parcel.other_type = OtherItems;
			unit_to_spawn = FindUnitData("bandit_hegemon_q");
			unit_spawn_level = -3;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &parcel;
			at_level = loc2.GetRandomLevel();

			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[265];

			msgs.push_back(Format(game->txQuest[3], who, loc.name.c_str(), game->day+1, game->month+1, game->year));
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

			quest_index = game->quests.size();
			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&parcel);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::Timeout:
		// player failed to retrive package in time
		{
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::Failed;

			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}

			msgs.push_back(game->txQuest[24]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// player returned package to mayor, end of quest
		{
			state = Quest::Completed;
			game->AddReward(500);

			((City*)game->locations[start_loc])->quest_mayor = CityQuestState::None;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}

			msgs.push_back(game->txQuest[25]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
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
cstring Quest_RetrivePackage::FormatString(const string& str)
{
	if(str == "burmistrz_od")
		return game->locations[from_loc]->type == L_CITY ? game->txQuest[26] : game->txQuest[27];
	else if(str == "locname_od")
		return game->locations[from_loc]->name.c_str();
	else if(str == "locname")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else
	{
		assert(0);
		return NULL;
	}
}

//=================================================================================================
bool Quest_RetrivePackage::IsTimedout() const
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_RetrivePackage::OnTimeout(TimeoutType ttype)
{
	if(done)
	{
		int at_lvl = at_level;
		Unit* u = game->locations[target_loc]->FindUnit(FindUnitData("bandit_hegemon_q"), at_lvl);
		if(u && u->IsAlive())
			u->RemoveQuestItem(refid);
	}

	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_RetrivePackage::IfHaveQuestItem() const
{
	return game->current_location == start_loc && prog == Progress::Started;
}

//=================================================================================================
const Item* Quest_RetrivePackage::GetQuestItem()
{
	return &parcel;
}

//=================================================================================================
void Quest_RetrivePackage::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	if(prog != Progress::Finished)
		WriteFile(file, &from_loc, sizeof(from_loc), &tmp, NULL);
}

//=================================================================================================
void Quest_RetrivePackage::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(prog != Progress::Finished)
	{
		ReadFile(file, &from_loc, sizeof(from_loc), &tmp, NULL);

		Location& loc = *game->locations[start_loc];
		parcel.name = Format(game->txQuest[8], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.type = IT_OTHER;
		parcel.weight = 10;
		parcel.value = 0;
		parcel.flags = ITEM_QUEST | ITEM_DONT_DROP | ITEM_IMPORTANT | ITEM_TEX_ONLY;
		parcel.id = "$stolen_parcel";
		parcel.mesh.clear();
		parcel.tex = game->tPaczka;
		parcel.refid = refid;
		parcel.other_type = OtherItems;

		if(game->mp_load)
			game->Net_RegisterItem(&parcel);

		item_to_give[0] = &parcel;
		unit_to_spawn = FindUnitData("bandit_hegemon_q");
		unit_spawn_level = -3;
		spawn_item = Quest_Dungeon::Item_GiveSpawned;
	}
}
