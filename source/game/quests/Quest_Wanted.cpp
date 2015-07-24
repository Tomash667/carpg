#include "Pch.h"
#include "Base.h"
#include "Quest_Wanted.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"

//-----------------------------------------------------------------------------
DialogEntry wanted_start[] = {
	TALK(689),
	TALK2(690),
	TALK(691),
	CHOICE(692),
		SET_QUEST_PROGRESS(Quest_Wanted::Progress::Started),
		TALK2(693),
		TALK(694),
		TALK(695),
		END,
	END_CHOICE,
	CHOICE(696),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry wanted_timeout[] = {
	SET_QUEST_PROGRESS(Quest_Wanted::Progress::Timeout),
	TALK2(697),
	TALK(698),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry wanted_end[] = {
	TALK2(699),
	SET_QUEST_PROGRESS(Quest_Wanted::Progress::Finished),
	TALK2(700),
	END,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_Wanted::Start()
{
	start_loc = game->current_location;
	quest_id = Q_WANTED;
	type = Type::Captain;
	level = random(5, 15);
	crazy = (rand2()%5 == 0);
	clas = ClassInfo::GetRandomEvil();
	target_unit = NULL;
}

//=================================================================================================
DialogEntry* Quest_Wanted::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return wanted_start;
	case QUEST_DIALOG_FAIL:
		return wanted_timeout;
	case QUEST_DIALOG_NEXT:
		return wanted_end;
	default:
		assert(0);
		return NULL;
	}
}

//=================================================================================================
void Quest_Wanted::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started: // zaakceptowano
		{
			game->GenerateHeroName(clas, crazy, unit_name);
			target_loc = game->GetFreeRandomCityLocation(start_loc);
			// jeœli nie ma wolnego miasta to powie jakieœ ale go tam nie bêdzie...
			if(target_loc == -1)
				target_loc = game->GetRandomCityLocation(start_loc);
			Location& target = GetTargetLocation();
			if(!target.active_quest)
			{
				target.active_quest = this;
				unit_to_spawn = game->GetUnitDataFromClass(clas, crazy);
				unit_dont_attack = true;
				unit_event_handler = this;
				send_spawn_event = true;
				unit_spawn_level = level;
			}

			// dane questa
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[257];

			// dodaj list
			letter.ani = NULL;
			letter.flags = ITEM_QUEST|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			letter.id2 = "$wanted_letter";
			letter.mesh2.clear();
			letter.name = game->txQuest[258];
			letter.refid = refid;
			letter.tex = game->tListGonczy;
			letter.type = IT_OTHER;
			letter.value = 0;
			letter.weight = 1;
			letter.desc = Format(game->txQuest[259], level*100, unit_name.c_str());
			letter.other_type = OtherItems;
			game->current_dialog->pc->unit->AddItem(&letter, 1, true);

			quest_index = game->quests.size();
			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			// wpis do dziennika
			msgs.push_back(Format(game->txQuest[29], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[260], level*100, unit_name.c_str(), GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&letter);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, &letter, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::Timeout: // czas min¹³
		{
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_dowodca = CityQuestState::Failed;

			Location& target = GetTargetLocation();
			if(target.active_quest == this)
				target.active_quest = NULL;

			msgs.push_back(Format(game->txQuest[261], unit_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);

			done = false;
		}
		break;
	case Progress::Killed: // zabito
		{
			msgs.push_back(Format(game->txQuest[262], unit_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished: // wykonano
		{
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_dowodca = CityQuestState::None;

			game->AddReward(level*100);

			msgs.push_back(Format(game->txQuest[263], unit_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Wanted::FormatString(const string& str)
{
	if(str == "reward")
		return Format("%d", level*100);
	else if(str == "name")
		return unit_name.c_str();
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "player")
		return game->current_dialog->pc->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

//=================================================================================================
bool Quest_Wanted::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_Wanted::IfHaveQuestItem()
{
	return game->current_dialog->talker == target_unit;
}

//=================================================================================================
bool Quest_Wanted::IfNeedTalk(cstring topic)
{
	return prog == Progress::Killed && strcmp(topic, "wanted") == 0;
}

//=================================================================================================
void Quest_Wanted::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
{
	if(type == UnitEventHandler::SPAWN)
	{
		unit->hero->name = unit_name;
		GetTargetLocation().active_quest = NULL;
		target_unit = unit;
		game->AddTimedUnit(target_unit, game->current_location, 30 - (game->worldtime - start_time));
	}
	else if(type == UnitEventHandler::DIE)
	{
		SetProgress(Progress::Killed);
		game->RemoveTimedUnit(target_unit);
		target_unit = NULL;
	}
}

//=================================================================================================
void Quest_Wanted::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &level, sizeof(level), &tmp, NULL);
	WriteFile(file, &crazy, sizeof(crazy), &tmp, NULL);
	WriteFile(file, &clas, sizeof(clas), &tmp, NULL);
	WriteString1(file, unit_name);
	int unit_refid = (target_unit ? target_unit->refid : -1);
	WriteFile(file, &unit_refid, sizeof(unit_refid), &tmp, NULL);
}

//=================================================================================================
void Quest_Wanted::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &level, sizeof(level), &tmp, NULL);
	ReadFile(file, &crazy, sizeof(crazy), &tmp, NULL);
	ReadFile(file, &clas, sizeof(clas), &tmp, NULL);
	if(LOAD_VERSION < V_DEVEL)
		clas = ClassInfo::OldToNew(clas);
	ReadString1(file, unit_name);
	int unit_refid;
	ReadFile(file, &unit_refid, sizeof(unit_refid), &tmp, NULL);
	target_unit = Unit::GetByRefid(unit_refid);

	// list
	letter.ani = NULL;
	letter.flags = ITEM_QUEST|ITEM_IMPORTANT|ITEM_TEX_ONLY;
	letter.id2 = "$wanted_letter";
	letter.mesh2.clear();
	letter.name = game->txQuest[258];
	letter.refid = refid;
	letter.tex = game->tListGonczy;
	letter.type = IT_OTHER;
	letter.value = 0;
	letter.weight = 1;
	letter.desc = Format(game->txQuest[259], level*100, unit_name.c_str());
	letter.other_type = OtherItems;

	if(game->mp_load)
		game->Net_RegisterItem(&letter);
}
