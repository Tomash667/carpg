#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "DialogDefine.h"
#include "SaveState.h"
#include "Journal.h"

Game* Quest::game;
extern DWORD tmp;

void Quest::Save(HANDLE file)
{
	WriteFile(file, &quest_id, sizeof(quest_id), &tmp, NULL);
	WriteFile(file, &state, sizeof(state), &tmp, NULL);
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, name.c_str(), len, &tmp, NULL);
	WriteFile(file, &prog, sizeof(prog), &tmp, NULL);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	WriteFile(file, &start_time, sizeof(start_time), &tmp, NULL);
	WriteFile(file, &start_loc, sizeof(start_loc), &tmp, NULL);
	WriteFile(file, &type, sizeof(type), &tmp, NULL);
	len = (byte)msgs.size();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	for(byte i=0; i<len; ++i)
	{
		word len2 = (word)msgs[i].length();
		WriteFile(file, &len2, sizeof(len2), &tmp, NULL);
		WriteFile(file, msgs[i].c_str(), len2, &tmp, NULL);
	}
}

void Quest::Load(HANDLE file)
{
	// quest_id jest ju¿ odczytane
	//ReadFile(file, &quest_id, sizeof(quest_id), &tmp, NULL);
	ReadFile(file, &state, sizeof(state), &tmp, NULL);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	name.resize(len);
	ReadFile(file, (char*)name.c_str(), len, &tmp, NULL);
	ReadFile(file, &prog, sizeof(prog), &tmp, NULL);
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	ReadFile(file, &start_time, sizeof(start_time), &tmp, NULL);
	ReadFile(file, &start_loc, sizeof(start_loc), &tmp, NULL);
	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	msgs.resize(len);
	for(byte i=0; i<len; ++i)
	{
		word len2;
		ReadFile(file, &len2, sizeof(len2), &tmp, NULL);
		msgs[i].resize(len2);
		ReadFile(file, (char*)msgs[i].c_str(), len2, &tmp, NULL);
	}
	if(LOAD_VERSION == V_0_2)
	{
		bool ended;
		ReadFile(file, &ended, sizeof(ended), &tmp, NULL);
	}
}

Location& Quest::GetStartLocation()
{
	return *game->locations[start_loc];
}

const Location& Quest::GetStartLocation() const
{
	return *game->locations[start_loc];
}

cstring Quest::GetStartLocationName() const
{
	return GetStartLocation().name.c_str();
}

void Quest_Dungeon::Save(HANDLE file)
{
	Quest::Save(file);

	WriteFile(file, &target_loc, sizeof(target_loc), &tmp, NULL);
	WriteFile(file, &done, sizeof(done), &tmp, NULL);

	if(!done)
		WriteFile(file, &at_level, sizeof(at_level), &tmp, NULL);
}

void Quest_Dungeon::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &target_loc, sizeof(target_loc), &tmp, NULL);
	ReadFile(file, &done, sizeof(done), &tmp, NULL);

	if(!done)
		ReadFile(file, &at_level, sizeof(at_level), &tmp, NULL);
}

Location& Quest_Dungeon::GetTargetLocation()
{
	return *game->locations[target_loc];
}

const Location& Quest_Dungeon::GetTargetLocation() const
{
	return *game->locations[target_loc];
}

cstring Quest_Dungeon::GetTargetLocationName() const
{
	return GetTargetLocation().name.c_str();
}

cstring Quest_Dungeon::GetTargetLocationDir() const
{
	return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
}

Quest_Event* Quest_Dungeon::GetEvent(int current_loc)
{
	Quest_Event* event = this;

	while(event)
	{
		if(event->target_loc == current_loc || event->target_loc == -1)
			return event;
		event = event->next_event;
	}

	return NULL;
}

void Quest_Encounter::RemoveEncounter()
{
	if(enc == -1)
		return;
	game->RemoveEncounter(enc);
	enc = -1;
}

void Quest_Encounter::Save(HANDLE file)
{
	Quest::Save(file);

	WriteFile(file, &enc, sizeof(enc), &tmp, NULL);
}

void Quest_Encounter::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &enc, sizeof(enc), &tmp, NULL);
}

//====================================================================================================================================================================================
// 1 - dosta³eœ list
// 2 - czas min¹³
// 3 - zanios³eœ list
// 4 - dostarczy³eœ odpowiedŸ
DialogEntry dostarcz_list_start[] = {
	TALK2(0),
	CHOICE(1),
		RANDOM_TEXT(2),
			TALK(2),
			TALK(3),
		TALK2(4),
		SET_QUEST_PROGRESS(1),
		END,
	END_CHOICE,
	CHOICE(5),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dostarcz_list_czas_minal[] = {
	TALK2(6),
	SET_QUEST_PROGRESS(2),
	END,
	END_OF_DIALOG
};

DialogEntry dostarcz_list_daj[] = {
	TALK(7),
	TALK(8),
	IF_QUEST_TIMEOUT,
		TALK(9),
		SET_QUEST_PROGRESS(2),
		END,
	END_IF,
	RANDOM_TEXT(3),
		TALK(10),
		TALK(11),
		TALK(12),
	TALK(13),
	SET_QUEST_PROGRESS(3),
	END,
	END_OF_DIALOG
};

DialogEntry dostarcz_list_koniec[] = {
	TALK(14),
	TALK(15),
	TALK(16),
	SET_QUEST_PROGRESS(4),
	END,
	END_OF_DIALOG
};

void Quest_DostarczList::Start()
{
	start_loc = game->current_location;
	end_loc = game->GetRandomCityLocation(start_loc);
	quest_id = Q_DOSTARCZ_LIST;
	type = 0;
}

DialogEntry* Quest_DostarczList::GetDialog(int type)
{
	switch(type)
	{
	case QUEST_DIALOG_START:
		return dostarcz_list_start;
	case QUEST_DIALOG_FAIL:
		return dostarcz_list_czas_minal;
	case QUEST_DIALOG_NEXT:
		if(prog == 1)
			return dostarcz_list_daj;
		else
			return dostarcz_list_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_DostarczList::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// koniec rozmowy z osob¹ daj¹c¹ questa
		{
			prog = 1;
			Location& loc = *game->locations[end_loc];
			letter.name = Format(game->txQuest[0], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			letter.type = IT_OTHER;
			letter.weight = 1;
			letter.value = 0;
			letter.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			letter.id = "$letter";
			letter.mesh = NULL;
			letter.tex = game->tList;
			letter.refid = refid;
			letter.other_type = OtherItems;
			game->current_dialog->pc->unit->AddItem(&letter, 1, true);
			start_time = game->worldtime;
			state = Quest::Started;

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			Location& loc2 = *game->locations[start_loc];
			name = game->txQuest[2];
			msgs.push_back(Format(game->txQuest[3], loc2.type == L_CITY ? game->txForMayor : game->txForSoltys, loc2.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[4], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str(), kierunek_nazwa[GetLocationDir(loc2.pos, loc.pos)]));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
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
	case 2:
		// nie zd¹¿y³eœ dostarczyæ listu na czas
		{
			prog = 2;
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_burmistrz = 2;

			msgs.push_back(game->txQuest[5]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// dostarczy³eœ list, zanieœ odpowiedŸ
		{
			prog = 3;
			Location& loc = *game->locations[end_loc];
			letter.name = Format(game->txQuest[1], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());

			msgs.push_back(game->txQuest[6]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_RenameItem(&letter);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	case 4:
		// koniec questa
		{
			prog = 4;
			state = Quest::Completed;
			game->AddReward(100, 250);

			((City*)game->locations[start_loc])->quest_burmistrz = 0;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			msgs.push_back(game->txQuest[7]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	}
}

cstring Quest_DostarczList::FormatString(const string& str)
{
	Location& loc = *game->locations[end_loc];
	if(str == "target_burmistrza")
		return (loc.type == L_CITY ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return kierunek_nazwa[GetLocationDir(game->locations[start_loc]->pos, loc.pos)];
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_DostarczList::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

bool Quest_DostarczList::IfHaveQuestItem()
{
	if(prog == 1)
		return game->current_location == end_loc;
	else
		return game->current_location == start_loc && prog == 3;
}

const Item* Quest_DostarczList::GetQuestItem()
{
	return &letter;
}

void Quest_DostarczList::Save(HANDLE file)
{
	Quest::Save(file);

	if(prog < 4)
	{
		WriteFile(file, &end_loc, sizeof(end_loc), &tmp, NULL);
	}
}

void Quest_DostarczList::Load(HANDLE file)
{
	Quest::Load(file);

	if(prog < 4)
	{
		ReadFile(file, &end_loc, sizeof(end_loc), &tmp, NULL);

		if(prog == 3)
		{
			Location& loc = *game->locations[start_loc];
			letter.name = Format(game->txQuest[1], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		}
		else
		{
			Location& loc = *game->locations[end_loc];
			letter.name = Format(game->txQuest[0], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		}

		letter.type = IT_OTHER;
		letter.weight = 1;
		letter.value = 0;
		letter.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
		letter.id = "$letter";
		letter.mesh = NULL;
		letter.tex = game->tList;
		letter.refid = refid;
		letter.other_type = OtherItems;

		if(game->mp_load)
			game->Net_RegisterItem(&letter);
	}
}

//====================================================================================================================================================================================
// 1 - dosta³eœ paczkê
// 2 - paczka dostarczona po czasie
// 3 - czas min¹³
// 4 - paczka dostarczona
// 5 - zaatakowa³em bandytów
// 6 - odda³em paczkê bandyt¹
// 7 - brak paczki, atak bandytów
DialogEntry dostarcz_paczke_start[] = {
	TALK2(17),
	CHOICE(18),
		TALK2(19),
		TALK(20),
		TALK2(21),
		SET_QUEST_PROGRESS(1),
		END,
	END_CHOICE,
	CHOICE(22),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dostarcz_paczke_daj[] = {
	TALK(23),
	IF_QUEST_TIMEOUT,
		IF_RAND(2),
			TALK(24),
			TALK(25),
			SET_QUEST_PROGRESS(2),
			END,
		ELSE,
			TALK(26),
			SET_QUEST_PROGRESS(3),
			END,
		END_IF,
	END_IF,
	TALK(27),
	SET_QUEST_PROGRESS(4),
	END,
	END_OF_DIALOG
};

DialogEntry dostarcz_paczke_czas_minal[] = {
	TALK2(28),
	SET_QUEST_PROGRESS(3),
	END,
	END_OF_DIALOG
};

DialogEntry dostarcz_paczke_bandyci[] = {
	IF_QUEST_PROGRESS(1),
		IF_HAVE_QUEST_ITEM("$parcel"),
			TALK2(29),
			TALK(30),
			CHOICE(31),
				TALK(32),
				SET_QUEST_PROGRESS(6),
				END,
			END_CHOICE,
			CHOICE(33),
				SPECIAL("attack"),
				SET_QUEST_PROGRESS(5),
				END,
			END_CHOICE,
			SHOW_CHOICES,
		ELSE,
			SET_QUEST_PROGRESS(7),
			SPECIAL("attack"),
		END_IF,
	ELSE,
		TALK(34),
		END,
	END_IF,
	END_OF_DIALOG
};

void Quest_DostarczPaczke::Start()
{
	start_loc = game->current_location;
	end_loc = game->GetRandomCityLocation(start_loc);
	quest_id = Q_DOSTARCZ_PACZKE;
	type = 0;
}

DialogEntry* Quest_DostarczPaczke::GetDialog(int type)
{
	switch(type)
	{
	case QUEST_DIALOG_START:
		return dostarcz_paczke_start;
	case QUEST_DIALOG_FAIL:
		return dostarcz_paczke_czas_minal;
	case QUEST_DIALOG_NEXT:
		return dostarcz_paczke_daj;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_DostarczPaczke::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// koniec rozmowy z osob¹ daj¹c¹ questa
		{
			prog = 1;
			Location& loc = *game->locations[end_loc];
			parcel.name = Format(game->txQuest[8], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
			parcel.type = IT_OTHER;
			parcel.weight = 10;
			parcel.value = 0;
			parcel.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			parcel.id = "$parcel";
			parcel.mesh = NULL;
			parcel.tex = game->tPaczka;
			parcel.refid = refid;
			parcel.other_type = OtherItems;
			game->current_dialog->pc->unit->AddItem(&parcel, 1, true);
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[9];

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			Location& loc2 = *game->locations[start_loc];
			msgs.push_back(Format(game->txQuest[3], loc2.type == L_CITY ? game->txForMayor : game->txForSoltys, loc2.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[10], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str(), GetLocationDirName(loc2.pos, loc.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(rand2()%4 != 0)
			{
				Encounter* e = game->AddEncounter(enc);
				e->pos = (loc.pos+loc2.pos)/2;
				e->zasieg = 64;
				e->szansa = 45;
				e->dont_attack = true;
				e->dialog = dostarcz_paczke_bandyci;
				e->grupa = SG_BANDYCI;
				e->text = game->txQuest[11];
				e->quest = this;
				e->timed = true;
				e->location_event_handler = NULL;
			}

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&parcel);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, &parcel, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case 2:
		// nie zd¹¿y³em dostarczyæ, po³owa zap³aty
		{
			prog = 2;
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_burmistrz = 2;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(125, 300);

			msgs.push_back(game->txQuest[12]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			RemoveEncounter();

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case 3:
		// nie zd¹¿y³em dostarczyæ, brak zap³aty
		{
			prog = 3;
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_burmistrz = 2;

			msgs.push_back(game->txQuest[13]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			RemoveEncounter();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// dostarczy³em paczkê, koniec questa
		{
			prog = 4;
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_burmistrz = 0;

			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			game->AddReward(250, 620);

			RemoveEncounter();

			msgs.push_back(game->txQuest[14]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case 5:
		// nie oddawaj paczki bandytom
		{
			RemoveEncounter();

			msgs.push_back(game->txQuest[15]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 6:
		// odda³em paczkê bandyt¹
		{
			prog = 6;

			RemoveEncounter();

			game->current_dialog->talker->AddItem(&parcel, 1, true);
			game->RemoveQuestItem(&parcel, refid);

			msgs.push_back(game->txQuest[16]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// atak bandytów
		RemoveEncounter();
		break;
	}
}

cstring Quest_DostarczPaczke::FormatString(const string& str)
{
	Location& loc = *game->locations[end_loc];
	if(str == "target_burmistrza")
		return (loc.type == L_CITY ? game->txForMayor : game->txForSoltys);
	else if(str == "target_locname")
		return loc.name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, loc.pos);
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_DostarczPaczke::IsTimedout()
{
	return game->worldtime - start_time > 15;
}

bool Quest_DostarczPaczke::IfHaveQuestItem()
{
	if(game->current_location == Game::Get().encounter_loc && prog == 1)
		return true;
	return game->current_location == end_loc && (prog == 1 || prog == 6);
}

const Item* Quest_DostarczPaczke::GetQuestItem()
{
	return &parcel;
}

void Quest_DostarczPaczke::Save(HANDLE file)
{
	Quest_Encounter::Save(file);

	if(prog != 2 && prog != 4)
	{
		WriteFile(file, &end_loc, sizeof(end_loc), &tmp, NULL);
	}
}

void Quest_DostarczPaczke::Load(HANDLE file)
{
	Quest_Encounter::Load(file);

	if(prog != 2 && prog != 4)
	{
		ReadFile(file, &end_loc, sizeof(end_loc), &tmp, NULL);

		Location& loc = *game->locations[end_loc];
		parcel.name = Format(game->txQuest[8], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.type = IT_OTHER;
		parcel.weight = 10;
		parcel.value = 0;
		parcel.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
		parcel.id = "$parcel";
		parcel.mesh = NULL;
		parcel.tex = game->tPaczka;
		parcel.refid = refid;
		parcel.other_type = OtherItems;

		if(game->mp_load)
			game->Net_RegisterItem(&parcel);
	}

	if(enc != -1)
	{
		Location& loc = *game->locations[end_loc];
		Location& loc2 = *game->locations[start_loc];
		Encounter* e = game->RecreateEncounter(enc);
		e->pos = (loc.pos+loc2.pos)/2;
		e->zasieg = 64;
		e->szansa = 45;
		e->dont_attack = true;
		e->dialog = dostarcz_paczke_bandyci;
		e->grupa = SG_BANDYCI;
		e->text = game->txQuest[11];
		e->quest = this;
		e->timed = true;
		e->location_event_handler = NULL;
	}
}

//====================================================================================================================================================================================
DialogEntry roznies_wiesci_start[] = {
	TALK(35),
	CHOICE(36),
		IF_RAND(3),
			TALK2(37),
		ELSE,
			RANDOM_TEXT(4),
				TALK(38),
				TALK(39),
				TALK2(40),
				TALK(41),
		END_IF,
		TALK(42),
		TALK2(43),
		TALK(44),
		SET_QUEST_PROGRESS(1),
		END,
	END_CHOICE,
	CHOICE(45),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry roznies_wiesci_daj[] = {
	TALK(46),
	SET_QUEST_PROGRESS(2),
	END,
	END_OF_DIALOG
};

DialogEntry roznies_wiesci_po_czasie[] = {
	TALK(47),
	SET_QUEST_PROGRESS(3),
	END,
	END_OF_DIALOG
};

DialogEntry roznies_wiesci_koniec[] = {
	TALK(48),
	TALK(49),
	SET_QUEST_PROGRESS(4),
	END,
	END_OF_DIALOG
};

bool SortEntries(const Quest_RozniesWiesci::Entry& e1, const Quest_RozniesWiesci::Entry& e2)
{
	return e1.dist < e2.dist;
}

void Quest_RozniesWiesci::Start()
{
	type = 0;
	quest_id = Q_ROZNIES_WIESCI;
	start_loc = game->current_location;
	VEC2 pos = game->locations[start_loc]->pos;
	bool sorted = false;
	for(uint i=0, count = game->cities; i<count; ++i)
	{
		if(i == start_loc)
			continue;
		Location& loc = *game->locations[i];
		if(loc.type != L_CITY && loc.type != L_VILLAGE)
			continue;
		float dist = distance(pos, loc.pos);
		bool ok = false;
		if(entries.size() < 5)
			ok = true;
		else
		{
			if(!sorted)
			{
				std::sort(entries.begin(), entries.end(), SortEntries);
				sorted = true;
			}
			if(entries.back().dist > dist)
			{
				ok = true;
				sorted = false;
				entries.pop_back();
			}
		}
		if(ok)
		{
			Entry& e = Add1(entries);
			e.location = i;
			e.given = false;
			e.dist = dist;
		}
	}
}

DialogEntry* Quest_RozniesWiesci::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return roznies_wiesci_start;
	case QUEST_DIALOG_FAIL:
		return roznies_wiesci_po_czasie;
	case QUEST_DIALOG_NEXT:
		if(prog == 1)
			return roznies_wiesci_daj;
		else
			return roznies_wiesci_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_RozniesWiesci::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// dosta³eœ zadanie i list
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			Location& loc = *game->locations[start_loc];
			name = game->txQuest[213];
			msgs.push_back(Format(game->txQuest[3], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[17], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str(), FormatString("targets")));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case 2:
		// przekaza³em wieœci
		{
			uint ile = 0;
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(game->current_location == it->location)
				{
					it->given = true;
					++ile;
				}
				else if(it->given)
					++ile;
			}

			Location& loc = *game->locations[game->current_location];
			msgs.push_back(Format(game->txQuest[18], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str()));

			if(ile == entries.size())
			{
				prog = 2;
				msgs.push_back(Format(game->txQuest[19], game->locations[start_loc]->name.c_str()));
			}

			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				if(prog == 2)
					game->Net_UpdateQuestMulti(refid, 2);
				else
					game->Net_UpdateQuest(refid);
			}
		}
		break;
	case 3:
		// czas min¹³
		{
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_burmistrz = 2;
			prog = 3;

			msgs.push_back(game->txQuest[20]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// zadanie wykonane
		{
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_burmistrz = 0;
			prog = 4;
			game->AddReward(200, 500);

			msgs.push_back(game->txQuest[21]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_RozniesWiesci::FormatString(const string& str)
{
	if(str == "targets")
	{
		static string s;
		s.clear();
		for(uint i=0, count = entries.size(); i<count; ++i)
		{
			s += game->locations[entries[i].location]->name;
			if(i == count-2)
				s += game->txQuest[264];
			else if(i != count-1)
				s += ", ";
		}
		return s.c_str();
	}
	else if(str == "start_loc")
		return game->locations[start_loc]->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_RozniesWiesci::IsTimedout()
{
	return game->worldtime - start_time > 60;
}

bool Quest_RozniesWiesci::IfNeedTalk(cstring topic)
{
	if(strcmp(topic, "tell_news") == 0)
	{
		if(prog == 1)
		{
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(it->location == game->current_location)
					return !it->given;
			}
		}
	}
	else if(strcmp(topic, "tell_news_end") == 0)
	{
		return prog == 2 && game->current_location == start_loc;
	}
	return false;
}

void Quest_RozniesWiesci::Save(HANDLE file)
{
	Quest::Save(file);

	if(IsActive())
	{
		uint count = entries.size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		WriteFile(file, &entries[0], sizeof(Entry)*count, &tmp, NULL);
	}
}

void Quest_RozniesWiesci::Load(HANDLE file)
{
	Quest::Load(file);

	if(IsActive())
	{
		uint count;
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		entries.resize(count);
		ReadFile(file, &entries[0], sizeof(Entry)*count, &tmp, NULL);
	}
}

//====================================================================================================================================================================================
DialogEntry odzyskaj_paczke_start[] = {
	TALK2(50),
	CHOICE(51),
		SET_QUEST_PROGRESS(1),
		IF_SPECIAL("czy_oboz"),
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

DialogEntry odzyskaj_paczke_po_czasie[] = {
	TALK(56),
	SET_QUEST_PROGRESS(2),
	END,
	END_OF_DIALOG
};

DialogEntry odzyskaj_paczke_koniec[] = {
	TALK(57),
	TALK(58),
	SET_QUEST_PROGRESS(3),
	END,
	END_OF_DIALOG
};

void Quest_OdzyskajPaczke::Start()
{
	quest_id = Q_ODZYSKAJ_PACZKE;
	type = 0;
	start_loc = game->current_location;
	from_loc = game->GetRandomCityLocation(start_loc);
}

DialogEntry* Quest_OdzyskajPaczke::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return odzyskaj_paczke_start;
	case QUEST_DIALOG_FAIL:
		return odzyskaj_paczke_po_czasie;
	case QUEST_DIALOG_NEXT:
		return odzyskaj_paczke_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_OdzyskajPaczke::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// otrzymano questa
		{
			target_loc = game->GetRandomSpawnLocation((game->locations[start_loc]->pos + game->locations[from_loc]->pos)/2, SG_BANDYCI);

			prog = 1;
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
			parcel.mesh = NULL;
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

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
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
	case 2:
		// czas min¹³
		{
			prog = 2;
			state = Quest::Failed;

			((City*)game->locations[start_loc])->quest_burmistrz = 2;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}

			msgs.push_back(game->txQuest[24]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// zadanie wykonane
		{
			prog = 3;
			state = Quest::Completed;
			game->AddReward(500, 1200);

			((City*)game->locations[start_loc])->quest_burmistrz = 0;
			game->current_dialog->pc->unit->RemoveQuestItem(refid);
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}

			msgs.push_back(game->txQuest[25]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
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

cstring Quest_OdzyskajPaczke::FormatString(const string& str)
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

bool Quest_OdzyskajPaczke::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

bool Quest_OdzyskajPaczke::IfHaveQuestItem()
{
	return game->current_location == start_loc && prog == 1;
}

const Item* Quest_OdzyskajPaczke::GetQuestItem()
{
	return &parcel;
}

void Quest_OdzyskajPaczke::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	if(prog != 3)
	{
		WriteFile(file, &from_loc, sizeof(from_loc), &tmp, NULL);
	}
}

void Quest_OdzyskajPaczke::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(prog != 3)
	{
		ReadFile(file, &from_loc, sizeof(from_loc), &tmp, NULL);

		Location& loc = *game->locations[start_loc];
		parcel.name = Format(game->txQuest[8], loc.type == L_CITY ? game->txForMayor : game->txForSoltys, loc.name.c_str());
		parcel.type = IT_OTHER;
		parcel.weight = 10;
		parcel.value = 0;
		parcel.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
		parcel.id = "$stolen_parcel";
		parcel.mesh = NULL;
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

//====================================================================================================================================================================================
// 0 - przed zaakceptowaniem
// 1 - zaakceptowano
// 2 - spotkanie z porwanym
// 3 - porwana osoba umar³a
// 4 - czas min¹³
// 5 - ukoñczono
// 6 - uciek³ z lokacji
// 7 - poinformowano o œmierci
// 8 - poinformowano o ucieczce
DialogEntry uratuj_porwana_osobe_start[] = {
	TALK2(59),
	TALK(60),
	CHOICE(61),
		SET_QUEST_PROGRESS(1),
		IF_SPECIAL("czy_oboz"),
			TALK2(62),
		ELSE,
			TALK2(63),
		END_IF,
		TALK(64),
		END,
	END_CHOICE,
	CHOICE(65),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry uratuj_porwana_osobe_po_czasie[] = {
	TALK(66),
	SET_QUEST_PROGRESS(4),
	END,
	END_OF_DIALOG
};

DialogEntry uratuj_porwana_osobe_koniec[] = {
	IF_QUEST_PROGRESS(2),
		SET_QUEST_PROGRESS(5),
		TALK(67),
		END,
	END_IF,
	IF_QUEST_PROGRESS(3),
		SET_QUEST_PROGRESS(7),
		TALK(68),
		TALK(69),
		END2,
	ELSE,
		SET_QUEST_PROGRESS(8),
		TALK(70),
		TALK(71),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry uratuj_porwana_osobe_rozmowa[] = {
	IF_QUEST_PROGRESS(2),
		TALK(72),
		SET_QUEST_PROGRESS(9),
		END2,
	END_IF,
	TALK(73),
	TALK2(74),
	CHOICE(75),
		SPECIAL("captive_join"),
		SET_QUEST_PROGRESS(2),
		TALK(76),
		END2,
	END_CHOICE,
	CHOICE(77),
		SPECIAL("captive_escape"),
		SET_QUEST_PROGRESS(2),
		TALK(78),
		END2,
	END_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

void Quest_UratujPorwanaOsobe::Start()
{
	quest_id = Q_URATUJ_PORWANA_OSOBE;
	type = 1;
	start_loc = game->current_location;

	switch(rand2()%4)
	{
	case 0:
	case 1:
		group = SG_BANDYCI;
		break;
	case 2:
		group = SG_ORKOWIE;
		break;
	case 3:
		group = SG_GOBLINY;
		break;
	}
}

DialogEntry* Quest_UratujPorwanaOsobe::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return uratuj_porwana_osobe_start;
	case QUEST_DIALOG_FAIL:
		return uratuj_porwana_osobe_po_czasie;
	case QUEST_DIALOG_NEXT:
		if(strcmp(game->current_dialog->talker->data->id, "captive") == 0)
			return uratuj_porwana_osobe_rozmowa;
		else
			return uratuj_porwana_osobe_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_UratujPorwanaOsobe::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// zadanie przyjête
		{
			target_loc = game->GetRandomSpawnLocation(game->locations[start_loc]->pos, group);

			prog = 1;
			Location& loc = *game->locations[start_loc];
			Location& loc2 = *game->locations[target_loc];
			bool now_known = false;
			if(loc2.state == LS_UNKNOWN)
			{
				loc2.state = LS_KNOWN;
				now_known = true;
			}

			loc2.active_quest = this;
			unit_to_spawn = FindUnitData("captive");
			unit_dont_attack = true;
			at_level = loc2.GetRandomLevel();
			unit_event_handler = this;

			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[28];
			captive = NULL;

			msgs.push_back(Format(game->txQuest[29], loc.name.c_str(), game->day+1, game->month+1, game->year));

			cstring co;
			switch(group)
			{
			case SG_BANDYCI:
			default:
				co = game->txQuest[30];
				break;
			case SG_ORKOWIE:
				co = game->txQuest[31];
				break;
			case SG_GOBLINY:
				co = game->txQuest[32];
				break;
			}

			if(loc2.type == L_CAMP)
			{
				game->target_loc_is_camp = true;
				msgs.push_back(Format(game->txQuest[33], loc.name.c_str(), co, GetLocationDirName(loc.pos, loc2.pos)));
			}
			else
			{
				game->target_loc_is_camp = false;
				msgs.push_back(Format(game->txQuest[34], loc.name.c_str(), co, loc2.name.c_str(), GetLocationDirName(loc.pos, loc2.pos)));
			}

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		// spotka³o siê porwanego
		{
			captive = game->current_dialog->talker;
			captive->event_handler = this;
			prog = 2;

			msgs.push_back(game->txQuest[35]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// porwana osoba umar³a
		{
			if(captive)
			{
				captive->event_handler = NULL;
				captive = NULL;
			}
			prog = 3;

			msgs.push_back(game->txQuest[36]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// czas min¹³
		{
			prog = 4;
			state = Quest::Failed;

			((City*)game->locations[start_loc])->quest_dowodca = 2;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			msgs.push_back(game->txQuest[37]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(captive)
			{
				captive->event_handler = NULL;
				captive = NULL;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// zadanie wykonane
		{
			prog = 5;
			state = Quest::Completed;
			game->AddReward(1000);

			((City*)game->locations[start_loc])->quest_dowodca = 0;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			RemoveElement(game->team, captive);
			
			captive->to_remove = true;
			game->to_remove.push_back(captive);
			captive->event_handler = NULL;
			captive = NULL;
			msgs.push_back(Format(game->txQuest[38], game->locations[start_loc]->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_KickNpc(captive);
				game->Net_RemoveUnit(captive);
			}
		}
		break;
	case 6:
		// porwana osoba sama uciek³a
		{
			if(captive)
			{
				captive->event_handler = NULL;
				captive = NULL;
			}
			prog = 6;

			msgs.push_back(game->txQuest[39]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// poinformowanie o œmierci porwanej osoby
		{
			prog = 7;
			state = Quest::Failed;
			if(captive)
			{
				captive->event_handler = NULL;
				captive = NULL;
			}

			((City*)game->locations[start_loc])->quest_dowodca = 2;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			msgs.push_back(game->txQuest[40]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 8:
		// zadanie wykonane
		{
			prog = 8;
			state = Quest::Completed;
			game->AddReward(250);
			if(captive)
			{
				captive->event_handler = NULL;
				captive = NULL;
			}

			((City*)game->locations[start_loc])->quest_dowodca = 0;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}

			msgs.push_back(Format(game->txQuest[41], game->locations[start_loc]->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 9:
		// zostawi³o siê porwan¹ osobê w mieœcie
		{
			prog = 9;
			captive->hero->team_member = false;
			captive->MakeItemsTeam(true);
			captive->dont_attack = false;
			captive->ai->goto_inn = true;
			captive->ai->timer = 0.f;
			captive->temporary = true;
			if(RemoveElementTry(game->team, captive) && game->IsOnline())
				game->Net_KickNpc(captive);
			captive->event_handler = NULL;
			captive = NULL;

			msgs.push_back(Format(game->txQuest[42], game->city_ctx->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_UratujPorwanaOsobe::FormatString(const string& str)
{
	if(str == "i_bandyci")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[43];
		case SG_ORKOWIE:
			return game->txQuest[44];
		case SG_GOBLINY:
			return game->txQuest[45];
		default:
			assert(0);
			return game->txQuest[46];
		}
	}
	else if(str == "ci_bandyci")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[47];
		case SG_ORKOWIE:
			return game->txQuest[48];
		case SG_GOBLINY:
			return game->txQuest[49];
		default:
			assert(0);
			return game->txQuest[50];
		}
	}
	else if(str == "locname")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "start_loc")
		return game->locations[start_loc]->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_UratujPorwanaOsobe::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

void Quest_UratujPorwanaOsobe::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
{
	assert(unit);

	switch(type)
	{
	case UnitEventHandler::DIE:
		SetProgress(3);
		break;
	case UnitEventHandler::LEAVE:
		SetProgress(6);
		break;
	}
}

bool Quest_UratujPorwanaOsobe::IfNeedTalk(cstring topic)
{
	if(strcmp(topic, "captive") != 0)
		return false;
	if(game->current_location == start_loc)
	{
		if(prog == 3 || prog == 6 || prog == 9)
			return true;
		else if(prog == 2 && game->IsTeamMember(*captive))
			return true;
		else
			return false;
	}
	else if(game->current_location == target_loc && prog == 1)
		return true;
	else
		return false;
}

void Quest_UratujPorwanaOsobe::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	if(prog != 0)
		WriteFile(file, &group, sizeof(group), &tmp, NULL);
	int crefid = (captive ? captive->refid : -1);
	WriteFile(file, &crefid, sizeof(crefid), &tmp, NULL);
}

void Quest_UratujPorwanaOsobe::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(prog != 0)
		ReadFile(file, &group, sizeof(group), &tmp, NULL);
	int crefid;
	ReadFile(file, &crefid, sizeof(crefid), &tmp, NULL);
	captive = Unit::GetByRefid(crefid);
	unit_event_handler = this;

	if(!done)
	{
		unit_to_spawn = FindUnitData("captive");
		unit_dont_attack = true;
	}
}

//====================================================================================================================================================================================
// 1 - przyjêto quest
// 2 - czas min¹³
// 3 - zabito bandytów
// 4 - poinformowano
DialogEntry dialog_bandyci_pobieraja_oplate_start[] = {
	TALK2(79),
	TALK(80),
	CHOICE(81),
		SET_QUEST_PROGRESS(1),
		TALK2(82),
		TALK(83),
		END,
	END_CHOICE,
	CHOICE(84),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_pobieraja_oplate_czas_minal[] = {
	SET_QUEST_PROGRESS(2),
	TALK(85),
	TALK(86),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_pobieraja_oplate_koniec[] = {
	SET_QUEST_PROGRESS(4),
	TALK(87),
	TALK(88),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_pobieraja_oplate_gadka[] = {
	TALK(89),
	TALK(90),
	CHOICE(91),
		IF_SPECIAL("have_500"),
			TALK(92),
			SPECIAL("pay_500"),
			END2,
		ELSE,
			TALK(93),
			TALK(94),
			SPECIAL("attack"),
			END2,
		END_IF,
	END_CHOICE,
	CHOICE(95),
		TALK(96),
		SPECIAL("attack"),
		END2,
	END_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

void Quest_BandyciPobierajaOplate::Start()
{
	quest_id = Q_BANDYCI_POBIERAJA_OPLATE;
	type = 1;
	start_loc = game->current_location;
	other_loc = game->GetRandomCityLocation(start_loc);
}

DialogEntry* Quest_BandyciPobierajaOplate::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return dialog_bandyci_pobieraja_oplate_start;
	case QUEST_DIALOG_FAIL:
		return dialog_bandyci_pobieraja_oplate_czas_minal;
	case QUEST_DIALOG_NEXT:
		return dialog_bandyci_pobieraja_oplate_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_BandyciPobierajaOplate::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// przyjêto questa
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[51];

			Location& sl = *game->locations[start_loc];
			Location& ol = *game->locations[other_loc];

			Encounter* e = game->AddEncounter(enc);
			e->dialog = dialog_bandyci_pobieraja_oplate_gadka;
			e->dont_attack = true;
			e->grupa = SG_BANDYCI;
			e->pos = (sl.pos + ol.pos)/2;
			e->quest = this;
			e->szansa = 50;
			e->text = game->txQuest[52];
			e->timed = true;
			e->zasieg = 64;
			e->location_event_handler = this;

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[53], sl.name.c_str(), ol.name.c_str(), GetLocationDirName(sl.pos, ol.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case 2:
		// czas min¹³
		{
			prog = 2;
			state = Quest::Failed;
			RemoveEncounter();
			((City*)game->locations[start_loc])->quest_dowodca = 2;
			msgs.push_back(game->txQuest[54]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// zabito bandytów
		{
			prog = 3;
			msgs.push_back(game->txQuest[55]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveEncounter();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// koniec questa
		{
			prog = 4;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[56]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(400);
			((City*)game->locations[start_loc])->quest_dowodca = 0;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_BandyciPobierajaOplate::FormatString(const string& str)
{
	if(str == "start_loc")
		return game->locations[start_loc]->name.c_str();
	else if(str == "other_loc")
		return game->locations[other_loc]->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[other_loc]->pos);
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_BandyciPobierajaOplate::IsTimedout()
{
	return game->worldtime - start_time > 15;
}

void Quest_BandyciPobierajaOplate::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == 1)
		SetProgress(3);
}

bool Quest_BandyciPobierajaOplate::IfNeedTalk(cstring topic)
{
	return (strcmp(topic, "road_bandits") == 0 && prog == 3);
}

void Quest_BandyciPobierajaOplate::Save(HANDLE file)
{
	Quest_Encounter::Save(file);

	WriteFile(file, &other_loc, sizeof(other_loc), &tmp, NULL);
}

void Quest_BandyciPobierajaOplate::Load(HANDLE file)
{
	Quest_Encounter::Load(file);

	ReadFile(file, &other_loc, sizeof(other_loc), &tmp, NULL);

	if(enc != -1)
	{
		Location& sl = *game->locations[start_loc];
		Location& ol = *game->locations[other_loc];

		Encounter* e = game->RecreateEncounter(enc);
		e->dialog = dialog_bandyci_pobieraja_oplate_gadka;
		e->dont_attack = true;
		e->grupa = SG_BANDYCI;
		e->pos = (sl.pos + ol.pos)/2;
		e->quest = this;
		e->szansa = 50;
		e->text = game->txQuest[52];
		e->timed = true;
		e->zasieg = 64;
		e->location_event_handler = this;
	}
}

//====================================================================================================================================================================================
// 1 - przyjêto questa
// 2 - oczyszczono lokacje
// 3 - ukoñczono zadanie
// 4 - czas min¹³
DialogEntry dialog_oboz_kolo_miasta_start[] = {
	TALK2(97),
	TALK2(98),
	TALK(99),
	CHOICE(100),
		SET_QUEST_PROGRESS(1),
		TALK2(101),
		TALK(102),
		END,
	END_CHOICE,
	CHOICE(103),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_oboz_kolo_miasta_czas_minal[] = {
	SET_QUEST_PROGRESS(4),
	TALK2(104),
	TALK(105),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_oboz_kolo_miasta_koniec[] = {
	SET_QUEST_PROGRESS(3),
	TALK(106),
	TALK(107),
	END,
	END_OF_DIALOG
};

void Quest_ObozKoloMiasta::Start()
{
	quest_id = Q_OBOZ_KOLO_MIASTA;
	type = 1;
	start_loc = game->current_location;
	switch(rand2()%3)
	{
	case 0:
		group = SG_BANDYCI;
		break;
	case 1:
		group = SG_GOBLINY;
		break;
	case 2:
		group = SG_ORKOWIE;
		break;
	}
}

DialogEntry* Quest_ObozKoloMiasta::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return dialog_oboz_kolo_miasta_start;
	case QUEST_DIALOG_FAIL:
		return dialog_oboz_kolo_miasta_czas_minal;
	case QUEST_DIALOG_NEXT:
		return dialog_oboz_kolo_miasta_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_ObozKoloMiasta::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// przyjêto zadanie
		{
			Location& sl = *game->locations[start_loc];

			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			if(sl.type == L_CITY)
				name = game->txQuest[57];
			else
				name = game->txQuest[58];

			target_loc = game->CreateCamp(sl.pos, group);
			location_event_handler = this;

			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			cstring gn;
			switch(group)
			{
			case SG_BANDYCI:
			default:
				gn = game->txQuest[59];
				break;
			case SG_ORKOWIE:
				gn = game->txQuest[60];
				break;
			case SG_GOBLINY:
				gn = game->txQuest[61];
				break;
			}

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[62], gn, GetLocationDirName(sl.pos, tl.pos), sl.name.c_str(), sl.type == L_CITY ? game->txQuest[63] : game->txQuest[64]));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		// oczyszczono lokacje
		{
			prog = 2;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[65]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// ukoñczono zadanie
		{
			prog = 3;
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_dowodca = 0;
			game->AddReward(2500);
			msgs.push_back(game->txQuest[66]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// czas min¹³
		{
			prog = 4;
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_dowodca = 2;
			msgs.push_back(Format(game->txQuest[67], game->locations[start_loc]->type == L_CITY ? game->txQuest[63] : game->txQuest[64]));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_ObozKoloMiasta::FormatString(const string& str)
{
	if(str == "Bandyci_zalozyli")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[68];
		case SG_ORKOWIE:
			return game->txQuest[69];
		case SG_GOBLINY:
			return game->txQuest[70];
		default:
			assert(0);
			return game->txQuest[71];
		}
	}
	else if(str == "naszego_miasta")
	{
		if(game->locations[start_loc]->type == L_CITY)
			return game->txQuest[72];
		else
			return game->txQuest[73];
	}
	else if(str == "miasto")
	{
		if(game->locations[start_loc]->type == L_CITY)
			return game->txQuest[63];
		else
			return game->txQuest[64];
	}
	else if(str == "dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "bandyci_zaatakowali")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[74];
		case SG_ORKOWIE:
			return game->txQuest[75];
		case SG_GOBLINY:
			return game->txQuest[266];
		default:
			assert(0);
			return NULL;
		}
	}
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_ObozKoloMiasta::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

void Quest_ObozKoloMiasta::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == 1)
		SetProgress(2);
}

bool Quest_ObozKoloMiasta::IfNeedTalk(cstring topic)
{
	return (strcmp(topic, "camp") == 0 && prog == 2);
}

void Quest_ObozKoloMiasta::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &group, sizeof(group), &tmp, NULL);
}

void Quest_ObozKoloMiasta::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &group, sizeof(group), &tmp, NULL);

	location_event_handler = this;
}

//====================================================================================================================================================================================
// 1 - przyjêto questa
// 2 - oczyszczono lokacje
// 3 - ukoñczono zadanie
// 4 - czas min¹³
DialogEntry dialog_zabij_zwierzeta_start[] = {
	TALK(108),
	TALK(109),
	CHOICE(110),
		SET_QUEST_PROGRESS(1),
		TALK2(111),
		TALK2(112),
		END,
	END_CHOICE,
	CHOICE(113),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_zabij_zwierzeta_koniec[] = {
	SET_QUEST_PROGRESS(3),
	TALK(114),
	TALK(115),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_zabij_zwierzeta_czas_minal[] = {
	SET_QUEST_PROGRESS(4),
	TALK2(116),
	END,
	END_OF_DIALOG
};

void Quest_ZabijZwierzeta::Start()
{
	quest_id = Q_ZABIJ_ZWIERZETA;
	type = 1;
	start_loc = game->current_location;
}

DialogEntry* Quest_ZabijZwierzeta::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return dialog_zabij_zwierzeta_start;
	case QUEST_DIALOG_FAIL:
		return dialog_zabij_zwierzeta_czas_minal;
	case QUEST_DIALOG_NEXT:
		return dialog_zabij_zwierzeta_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_ZabijZwierzeta::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// przyjêto zadanie
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[76];

			Location& sl = *game->locations[start_loc];

			target_loc = game->GetClosestLocation(rand2()%2 == 0 ? L_FOREST : L_CAVE, sl.pos);
			location_event_handler = this;
			
			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[77], sl.name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		// oczyszczono lokacje
		{
			prog = 2;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(Format(game->txQuest[78], game->locations[target_loc]->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// ukoñczono zadanie
		{
			prog = 3;
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_dowodca = 0;
			game->AddReward(1200);
			msgs.push_back(game->txQuest[79]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// czas min¹³
		{
			prog = 4;
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_dowodca = 2;
			msgs.push_back(game->txQuest[80]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_ZabijZwierzeta::FormatString(const string& str)
{
	if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_ZabijZwierzeta::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

void Quest_ZabijZwierzeta::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == 1)
		SetProgress(2);
}

bool Quest_ZabijZwierzeta::IfNeedTalk(cstring topic)
{
	return (strcmp(topic, "animals") == 0 && prog == 2);
}

void Quest_ZabijZwierzeta::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	location_event_handler = this;
}

//====================================================================================================================================================================================
// 1 - przyjêto questa
// 2 - ukoñczono questa
// 3 - czas min¹³
DialogEntry dialog_znajdz_artefakt_start[] = {
	TALK2(117),
	TALK(118),
	CHOICE(119),
		SET_QUEST_PROGRESS(1),
		TALK2(120),
		TALK(121),
		END,
	END_CHOICE,
	CHOICE(122),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_znajdz_artefakt_koniec[] = {
	SET_QUEST_PROGRESS(2),
	TALK(123),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_znajdz_artefakt_po_czasie[] = {
	SET_QUEST_PROGRESS(3),
	TALK(124),
	TALK2(125),
	TALK(126),
	END2,
	END_OF_DIALOG
};

void Quest_ZnajdzArtefakt::Start()
{
	quest_id = Q_PRZYNIES_ARTEFAKT;
	type = 2;
	start_loc = game->current_location;
	co = rand2()%21;
	item = &g_others[co+5];
}

DialogEntry* Quest_ZnajdzArtefakt::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return dialog_znajdz_artefakt_start;
	case QUEST_DIALOG_NEXT:
		return dialog_znajdz_artefakt_koniec;
	case QUEST_DIALOG_FAIL:
		return dialog_znajdz_artefakt_po_czasie;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_ZnajdzArtefakt::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[81];

			item_id = Format("$%s", item->id);
			quest_item.ani = NULL;
			quest_item.desc = "";
			quest_item.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			quest_item.id = item_id.c_str();
			quest_item.level = 0;
			quest_item.mesh = NULL;
			quest_item.name = item->name;
			quest_item.refid = refid;
			quest_item.tex = item->tex;
			quest_item.type = IT_OTHER;
			quest_item.value = item->value;
			quest_item.weight = item->weight;
			quest_item.other_type = OtherItems;
			spawn_item = Quest_Dungeon::Item_InTreasure;
			item_to_give[0] = &quest_item;

			Location& sl = *game->locations[start_loc];

			if(rand2()%4 == 0)
			{
				target_loc = game->GetClosestLocation(L_DUNGEON, sl.pos, LABIRYNTH);
				at_level = 0;
			}
			else
			{
				target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
				InsideLocation* inside = (InsideLocation*)(game->locations[target_loc]);
				if(inside->IsMultilevel())
				{
					MultiInsideLocation* multi = (MultiInsideLocation*)inside;
					at_level = multi->levels.size()-1;
				}
				else
					at_level = 0;
			}

			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = false;
			}

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[83], item->name, tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&quest_item);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		{
			prog = 2;
			state = Quest::Completed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[84]);
			game->AddReward(1000);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;
			game->current_dialog->talker->AddItem(&quest_item, 1, true);
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case 3:
		{
			prog = 3;
			state = Quest::Failed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[85]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_ZnajdzArtefakt::FormatString(const string& str)
{
	if(str == "przedmiot")
		return item->name.c_str();
	else if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "random_loc")
		return game->locations[game->GetRandomCityLocation(start_loc)]->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_ZnajdzArtefakt::IsTimedout()
{
	return game->worldtime - start_time > 60;
}

bool Quest_ZnajdzArtefakt::IfHaveQuestItem2(cstring id)
{
	return prog == 1 && strcmp(id, "$$artifact") == 0;
}

const Item* Quest_ZnajdzArtefakt::GetQuestItem()
{
	return &quest_item;
}

void Quest_ZnajdzArtefakt::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &co, sizeof(co), &tmp, NULL);
}

void Quest_ZnajdzArtefakt::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &co, sizeof(co), &tmp, NULL);

	item = &g_others[co+5];
	item_id = Format("$%s", item->id);
	quest_item.ani = NULL;
	quest_item.desc = "";
	quest_item.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
	quest_item.id = item_id.c_str();
	quest_item.level = 0;
	quest_item.mesh = NULL;
	quest_item.name = item->name;
	quest_item.refid = refid;
	quest_item.tex = item->tex;
	quest_item.type = IT_OTHER;
	quest_item.value = item->value;
	quest_item.weight = item->weight;
	quest_item.other_type = OtherItems;
	spawn_item = Quest_Dungeon::Item_InTreasure;
	item_to_give[0] = &quest_item;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item);
}

//====================================================================================================================================================================================
// 1 - przyjêto questa
// 2 - ukoñczono questa
// 3 - czas min¹³
DialogEntry dialog_ukradziony_przedmiot_start[] = {
	TALK2(127),
	TALK(128),
	CHOICE(129),
		SET_QUEST_PROGRESS(1),
		TALK2(130),
		TALK(131),
		END,
	END_CHOICE,
	CHOICE(132),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_ukradziony_przedmiot_koniec[] = {
	SET_QUEST_PROGRESS(2),
	TALK(133),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_ukradziony_przedmiot_po_czasie[] = {
	SET_QUEST_PROGRESS(3),
	TALK(134),
	TALK2(135),
	TALK(136),
	END2,
	END_OF_DIALOG
};

void Quest_UkradzionyPrzedmiot::Start()
{
	quest_id = Q_UKRADZIONY_PRZEDMIOT;
	type = 2;
	start_loc = game->current_location;
	co = rand2()%21;
	item = &g_others[co+5];
	switch(rand2()%6)
	{
	case 0:
		group = SG_BANDYCI;
		break;
	case 1:
		group = SG_ORKOWIE;
		break;
	case 2:
		group = SG_GOBLINY;
		break;
	case 3:
	case 4:
		group = SG_MAGOWIE;
		break;
	case 5:
		group = SG_ZLO;
		break;
	}
}

DialogEntry* Quest_UkradzionyPrzedmiot::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return dialog_ukradziony_przedmiot_start;
	case QUEST_DIALOG_NEXT:
		return dialog_ukradziony_przedmiot_koniec;
	case QUEST_DIALOG_FAIL:
		return dialog_ukradziony_przedmiot_po_czasie;
	default:
		assert(0);
		return NULL;
	}
}

cstring GetSpawnLeader(SPAWN_GROUP group)
{
	switch(group)
	{
	case SG_BANDYCI:
		return "bandit_hegemon_q";
	case SG_ORKOWIE:
		return "orc_chief_q";
	case SG_GOBLINY:
		return "goblin_chief_q";
	case SG_MAGOWIE:
		return "mage_q";
	case SG_ZLO:
		return "evil_cleric_q";
	default:
		assert(0);
		return "bandit_hegemon_q";
	}
}

void Quest_UkradzionyPrzedmiot::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[86];

			item_id = Format("$%s", item->id);
			quest_item.ani = NULL;
			quest_item.desc = "";
			quest_item.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			quest_item.id = item_id.c_str();
			quest_item.level = 0;
			quest_item.mesh = NULL;
			quest_item.name = item->name;
			quest_item.refid = refid;
			quest_item.tex = item->tex;
			quest_item.type = IT_OTHER;
			quest_item.value = item->value;
			quest_item.weight = item->weight;
			quest_item.other_type = OtherItems;
			spawn_item = Quest_Dungeon::Item_GiveSpawned;
			item_to_give[0] = &quest_item;
			unit_to_spawn = FindUnitData(GetSpawnLeader(group));
			unit_spawn_level = -3;

			Location& sl = *game->locations[start_loc];
			target_loc = game->GetRandomSpawnLocation(sl.pos, group);
			Location& tl = *game->locations[target_loc];
			at_level = tl.GetRandomLevel();
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			cstring kto;
			switch(group)
			{
			case SG_BANDYCI:
				kto = game->txQuest[87];
				break;
			case SG_GOBLINY:
				kto = game->txQuest[88];
				break;
			case SG_ORKOWIE:
				kto = game->txQuest[89];
				break;
			case SG_MAGOWIE:
				kto = game->txQuest[90];
				break;
			case SG_ZLO:
				kto = game->txQuest[91];
				break;
			default:
				kto = game->txQuest[92];
				break;
			}

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[93], item->name, kto, tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&quest_item);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		{
			prog = 2;
			state = Quest::Completed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[94]);
			game->AddReward(1200);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;
			game->current_dialog->talker->AddItem(&quest_item, 1, true);
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case 3:
		{
			prog = 3;
			state = Quest::Failed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[95]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_UkradzionyPrzedmiot::FormatString(const string& str)
{
	if(str == "przedmiot")
		return item->name.c_str();
	else if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "random_loc")
		return game->locations[game->GetRandomCityLocation(start_loc)]->name.c_str();
	else if(str == "Bandyci_ukradli")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[96];
		case SG_ORKOWIE:
			return game->txQuest[97];
		case SG_GOBLINY:
			return game->txQuest[98];
		case SG_MAGOWIE:
			return game->txQuest[99];
		case SG_ZLO:
			return game->txQuest[100];
		default:
			assert(0);
			return NULL;
		}
	}
	else if(str == "Ci_bandyci")
	{
		switch(group)
		{
		case SG_BANDYCI:
			return game->txQuest[101];
		case SG_ORKOWIE:
			return game->txQuest[102];
		case SG_GOBLINY:
			return game->txQuest[103];
		case SG_MAGOWIE:
			return game->txQuest[104];
		case SG_ZLO:
			return game->txQuest[105];
		default:
			assert(0);
			return NULL;
		}
	}
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_UkradzionyPrzedmiot::IsTimedout()
{
	return game->worldtime - start_time > 60;
}

bool Quest_UkradzionyPrzedmiot::IfHaveQuestItem2(cstring id)
{
	return prog == 1 && strcmp(id, "$$stolen_artifact") == 0;
}

const Item* Quest_UkradzionyPrzedmiot::GetQuestItem()
{
	return &quest_item;
}

void Quest_UkradzionyPrzedmiot::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &co, sizeof(co), &tmp, NULL);
	WriteFile(file, &group, sizeof(group), &tmp, NULL);
}

void Quest_UkradzionyPrzedmiot::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &co, sizeof(co), &tmp, NULL);
	ReadFile(file, &group, sizeof(group), &tmp, NULL);

	item = &g_others[co+5];
	item_id = Format("$%s", item->id);
	quest_item.ani = NULL;
	quest_item.desc = "";
	quest_item.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
	quest_item.id = item_id.c_str();
	quest_item.level = 0;
	quest_item.mesh = NULL;
	quest_item.name = item->name;
	quest_item.refid = refid;
	quest_item.tex = item->tex;
	quest_item.type = IT_OTHER;
	quest_item.value = item->value;
	quest_item.weight = item->weight;
	quest_item.other_type = OtherItems;
	spawn_item = Quest_Dungeon::Item_GiveSpawned;
	item_to_give[0] = &quest_item;
	unit_to_spawn = FindUnitData(GetSpawnLeader(group));
	unit_spawn_level = -3;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item);
}

//====================================================================================================================================================================================
// 1 - przyjêto questa
// 2 - ukoñczono questa
// 3 - czas min¹³
DialogEntry dialog_zgubiony_przedmiot_start[] = {
	TALK2(137),
	TALK(138),
	CHOICE(139),
		SET_QUEST_PROGRESS(1),
		TALK2(140),
		TALK(141),
		END,
	END_CHOICE,
	CHOICE(142),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_zgubiony_przedmiot_koniec[] = {
	SET_QUEST_PROGRESS(2),
	TALK(143),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_zgubiony_przedmiot_po_czasie[] = {
	SET_QUEST_PROGRESS(3),
	TALK(144),
	TALK2(145),
	TALK(146),
	END2,
	END_OF_DIALOG
};

void Quest_ZgubionyPrzedmiot::Start()
{
	quest_id = Q_ZGUBIONY_PRZEDMIOT;
	type = 2;
	start_loc = game->current_location;
	co = rand2()%21;
	item = &g_others[co+5];
}

DialogEntry* Quest_ZgubionyPrzedmiot::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return dialog_zgubiony_przedmiot_start;
	case QUEST_DIALOG_NEXT:
		return dialog_zgubiony_przedmiot_koniec;
	case QUEST_DIALOG_FAIL:
		return dialog_zgubiony_przedmiot_po_czasie;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_ZgubionyPrzedmiot::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[106];

			item_id = Format("$%s", item->id);
			quest_item.ani = NULL;
			quest_item.desc = "";
			quest_item.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
			quest_item.id = item_id.c_str();
			quest_item.level = 0;
			quest_item.mesh = NULL;
			quest_item.name = item->name;
			quest_item.refid = refid;
			quest_item.tex = item->tex;
			quest_item.type = IT_OTHER;
			quest_item.value = item->value;
			quest_item.weight = item->weight;
			quest_item.other_type = OtherItems;
			spawn_item = Quest_Dungeon::Item_OnGround;
			item_to_give[0] = &quest_item;

			Location& sl = *game->locations[start_loc];
			if(rand2()%2 == 0)
				target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
			else
				target_loc = game->GetClosestLocationNotTarget(L_DUNGEON, sl.pos, LABIRYNTH);
			Location& tl = *game->locations[target_loc];
			at_level = tl.GetRandomLevel();
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			cstring poziom;
			switch(at_level)
			{
			case 0:
				poziom = game->txQuest[107];
				break;
			case 1:
				poziom = game->txQuest[108];
				break;
			case 2:
				poziom = game->txQuest[109];
				break;
			case 3:
				poziom = game->txQuest[110];
				break;
			case 4:
				poziom = game->txQuest[111];
				break;
			case 5:
				poziom = game->txQuest[112];
				break;
			default:
				poziom = game->txQuest[113];
				break;
			}

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			game->current_dialog->talker->temporary = false;

			msgs.push_back(Format(game->txQuest[82], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[114], item->name, poziom, tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RegisterItem(&quest_item);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		{
			prog = 2;
			state = Quest::Completed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[115]);
			game->AddReward(800);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;
			game->current_dialog->talker->AddItem(&quest_item, 1, true);
			game->current_dialog->pc->unit->RemoveQuestItem(refid);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->Net_RemoveQuestItem(game->current_dialog->pc, refid);
			}
		}
		break;
	case 3:
		{
			prog = 3;
			state = Quest::Failed;
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = NULL;
			}
			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);
			msgs.push_back(game->txQuest[116]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->current_dialog->talker->temporary = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_ZgubionyPrzedmiot::FormatString(const string& str)
{
	if(str == "przedmiot")
		return item->name.c_str();
	else if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else if(str == "random_loc")
		return game->locations[game->GetRandomCityLocation(start_loc)]->name.c_str();
	else if(str == "poziomie")
	{
		switch(at_level)
		{
		case 0:
			return game->txQuest[117];
		case 1:
			return game->txQuest[118];
		case 2:
			return game->txQuest[119];
		case 3:
			return game->txQuest[120];
		case 4:
			return game->txQuest[121];
		case 5:
			return game->txQuest[122];
		default:
			return game->txQuest[123];
		}
	}
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_ZgubionyPrzedmiot::IsTimedout()
{
	return game->worldtime - start_time > 60;
}

bool Quest_ZgubionyPrzedmiot::IfHaveQuestItem2(cstring id)
{
	return prog == 1 && strcmp(id, "$$lost_item") == 0;
}

const Item* Quest_ZgubionyPrzedmiot::GetQuestItem()
{
	return &quest_item;
}

void Quest_ZgubionyPrzedmiot::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &co, sizeof(co), &tmp, NULL);
}

void Quest_ZgubionyPrzedmiot::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &co, sizeof(co), &tmp, NULL);

	item = &g_others[co+5];
	item_id = Format("$%s", item->id);
	quest_item.ani = NULL;
	quest_item.desc = "";
	quest_item.flags = ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY;
	quest_item.id = item_id.c_str();
	quest_item.level = 0;
	quest_item.mesh = NULL;
	quest_item.name = item->name;
	quest_item.refid = refid;
	quest_item.tex = item->tex;
	quest_item.type = IT_OTHER;
	quest_item.value = item->value;
	quest_item.weight = item->weight;
	quest_item.other_type = OtherItems;
	spawn_item = Quest_Dungeon::Item_OnGround;
	item_to_give[0] = &quest_item;

	if(game->mp_load)
		game->Net_RegisterItem(&quest_item);
}

//====================================================================================================================================================================================
// 0 - nie spotkano drwala
// 1 - nie zaakceptowano
// 2 - zaakceptowano
// 3 - oczyszczono lokacjê
// 4 - poinformowano o oczyszczeniu
// 5 - przyszed³ pos³anie i kasa
DialogEntry dialog_tartak[] = {
	IF_QUEST_PROGRESS(0),
		SPECIAL("tell_name"),
		TALK(147),
		TALK(148),
		TALK(149),
		TALK(150),
		TALK(151),
		TALK(152),
		CHOICE(153),
			SET_QUEST_PROGRESS(2),
			TALK2(154),
			TALK(155),
			TALK(156),
			END,
		END_CHOICE,
		CHOICE(157),
			SET_QUEST_PROGRESS(1),
			TALK(158),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	ELSE,
		IF_QUEST_PROGRESS(1),
			TALK(159),
			TALK(160),
			CHOICE(161),
				SET_QUEST_PROGRESS(2),
				TALK2(162),
				TALK(163),
				TALK(164),
				END,
			END_CHOICE,
			CHOICE(165),
				TALK(166),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		ELSE,
			IF_QUEST_PROGRESS(2),
				TALK(167),
				TALK(168),
				END,
			ELSE,
				IF_QUEST_PROGRESS(3),
					SET_QUEST_PROGRESS(4),
					TALK(169),
					TALK(170),
					TALK(171),
					TALK(172),
					END,
				ELSE,
					IF_QUEST_PROGRESS(4),
						IF_SPECIAL("czy_tartak"),
							TALK(173),
						ELSE,
							TALK(174),
						END_IF,
						END,
					ELSE,
						TALK(175),
						TALK(176),
						END,
					END_IF,
				END_IF,
			END_IF,
		END_IF,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_tartak_poslaniec[] = {
	IF_QUEST_PROGRESS(5),
		TALK(177),
		END,
	ELSE,
		SET_QUEST_PROGRESS(5),
		TALK(178),
		TALK(179),
		TALK(180),
		TALK(181),
		TALK(182),
		END,
	END_IF,
	END_OF_DIALOG
};

void Quest_Tartak::Start()
{
	quest_id = Q_TARTAK;
	type = 3;
	// start_loc ustawianie w Game::InitQuests
}

DialogEntry* Quest_Tartak::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(strcmp(game->current_dialog->talker->data->id, "artur_drwal") == 0)
			return dialog_tartak;
		else
			return dialog_tartak_poslaniec;
	}
	else
	{
		assert(0);
		return NULL;
	}
}

void Quest_Tartak::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// nie zaakceptowano
		{
			prog = 1;
		}
		break;
	case 2:
		// zakceptowano
		{
			prog = 2;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[124];

			location_event_handler = this;

			Location& sl = GetStartLocation();
			game->tartak_las = target_loc = game->GetClosestLocation(L_FOREST, sl.pos);
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			else if(tl.state >= LS_VISITED)
				tl.reset = true;
			tl.st = 8;

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[125], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[126], tl.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 3:
		// oczyszczono
		{
			prog = 3;

			msgs.push_back(Format(game->txQuest[127], GetTargetLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// poinformowano
		{
			prog = 4;
			game->tartak_dni = 0;
			game->tartak_stan = 2;
			if(!game->plotka_questowa[P_TARTAK])
			{
				game->plotka_questowa[P_TARTAK] = true;
				--game->ile_plotek_questowych;
			}
			
			msgs.push_back(game->txQuest[128]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// pierwsza kasa
		{
			prog = 5;
			state = Quest::Completed;
			game->tartak_stan = 3;
			game->tartak_dni = 0;

			msgs.push_back(game->txQuest[129]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(400);
			game->EndUniqueQuest();
			game->AddNews(Format(game->txQuest[130], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_Tartak::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Tartak::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "tartak") == 0;
}

void Quest_Tartak::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == 2 && event == LocationEventHandler::CLEARED)
		SetProgress(3);
}

void Quest_Tartak::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	location_event_handler = this;
}

//====================================================================================================================================================================================
// 1 - zaakceptowano questa
// 2 - oczyszczono lokacjê
// 3 - poinformowano o oczyszczeniu
// 4 - zap³ata
// 5 - wybrano z³oto
// 6 - pos³anie poprosi³ o spotkanie
// 7 - poinformowano o mo¿liwej rozbudowie
// 8 - nie zgodzi³eœ siê na inwestycjê
// 9 - zainwestowa³eœ
// 10 - rozbudowano
// 11 - poinformowano o portalu
// 12 - porozmawiano z szefem górników
// 13 - znaleziono artefakt
DialogEntry dialog_kopalnia_inwestor[] = {
	IF_QUEST_PROGRESS(0),
		IF_SPECIAL("is_not_known"),
			IF_ONCE,
			END_IF,
			SPECIAL("tell_name"),
			TALK(183),
			TALK(184),
		ELSE,
			IF_ONCE,
				TALK(185),
			END_IF,
		END_IF,
		CHOICE(186),
			TALK2(187),
			TALK(188),
			TALK(189),
			TALK(190),
			CHOICE(191),
				SET_QUEST_PROGRESS(1),
				TALK2(192),
				TALK(193),
				END,
			END_CHOICE,
			CHOICE(194),
				TALK(195),
				RESTART,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		END_CHOICE,
		CHOICE(196),
			TALK(197),
			RESTART,
		END_CHOICE,
		CHOICE(198),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	ELSE,
		IF_ONCE,
			IF_QUEST_PROGRESS(1),
				TALK(199),
			END_IF,
			IF_QUEST_PROGRESS(2),
				TALK(200),
			END_IF,
			IF_QUEST_PROGRESS(3),
				TALK(201),
			END_IF,
			IF_QUEST_PROGRESS(4),
				TALK(202),
			END_IF,
			IF_QUEST_PROGRESS(5),
				TALK(203),
			END_IF,
			IF_QUEST_PROGRESS(6),
				SET_QUEST_PROGRESS(7),
				TALK(204),
				TALK(205),
				TALK(206),
				TALK2(207),
				TALK(208),
				TALK(209),
				RESTART,
			END_IF,
			IF_QUEST_PROGRESS(7),
				TALK(210),
			END_IF,
			IF_QUEST_PROGRESS(8),
				TALK(211),
			END_IF,
			IF_QUEST_PROGRESS(9),
				TALK(212),
			END_IF,	
			IF_QUEST_PROGRESS(10),
				TALK(213),
			END_IF,
			IF_QUEST_PROGRESS(11),
				TALK(214),
			END_IF,
			IF_QUEST_PROGRESS(12),
				TALK(215),
			END_IF,
			IF_QUEST_PROGRESS(13),
				TALK(216),
			END_IF,
		END_IF,
		IF_QUEST_PROGRESS(7),
			IF_SPECIAL("udzialy_w_kopalni"),
				CHOICE(217),
					IF_SPECIAL("have_10000"),
						SET_QUEST_PROGRESS(9),
						TALK(218),
						END,
					ELSE,
						TALK(219),
						END,
					END_IF,
				END_CHOICE,
			ELSE,
				CHOICE(220),
					IF_SPECIAL("have_12000"),
						SET_QUEST_PROGRESS(9),
						TALK(221),
						END,
					ELSE,
						TALK(222),
						END,
					END_IF,
				END_CHOICE,
			END_IF,
			CHOICE(223),
				SET_QUEST_PROGRESS(8),
				TALK(224),
				END,
			END_CHOICE,
		END_IF,
		IF_QUEST_PROGRESS(2),
			CHOICE(225),
				TALK(226),
				TALK(227),
				CHOICE(228),
					SET_QUEST_PROGRESS(5),
					TALK(229),
					END,
				END_CHOICE,
				CHOICE(230),
					SET_QUEST_PROGRESS(3),
					TALK(231),
					TALK(232),
					END,
				END_CHOICE,
			END_CHOICE,
		END_IF,
		CHOICE(233),
			TALK(234),
			RESTART,
		END_CHOICE,
		CHOICE(235),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_kopalnia_poslaniec[] = {
	SET_QUEST_PROGRESS(4),
	TALK(236),
	TALK(237),
	TALK(238),
	TALK(239),
	TALK(240),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_kopalnia_poslaniec2[] = {
	SET_QUEST_PROGRESS(6),
	TALK(241),
	TALK(242),
	TALK(243),
	TALK(244),
	TALK(245),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_kopalnia_poslaniec3[] = {
	SET_QUEST_PROGRESS(10),
	TALK(246),
	TALK(247),
	TALK(248),
	TALK(249),
	TALK(250),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_kopalnia_poslaniec4[] = {
	SET_QUEST_PROGRESS(11),
	TALK(251),
	TALK(252),
	TALK(253),
	TALK(254),
	TALK(255),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_kopalnia_szef[] = {
	IF_QUEST_PROGRESS(3),
		TALK(256),
	END_IF,
	IF_QUEST_PROGRESS(4),
		TALK(257),
	END_IF,
	IF_QUEST_PROGRESS(5),
		TALK(258),
	END_IF,
	IF_QUEST_PROGRESS(6),
		TALK(259),
	END_IF,
	IF_QUEST_PROGRESS(7),
		TALK(260),
	END_IF,
	IF_QUEST_PROGRESS(8),
		TALK(261),
	END_IF,
	IF_QUEST_PROGRESS(9),
		TALK(262),
	END_IF,
	IF_QUEST_PROGRESS(10),
		TALK(263),
	END_IF,
	IF_QUEST_PROGRESS(11),
		SET_QUEST_PROGRESS(12),
		TALK(264),
		TALK(265),
		TALK(266),
		TALK(267),
		END,
	END_IF,
	IF_QUEST_PROGRESS(12),
		TALK(268),
	END_IF,
	IF_QUEST_PROGRESS(13),
		TALK(269),
	END_IF,
	END,
	END_OF_DIALOG
};

DialogEntry dialog_poslaniec[] = {
	TALK(270),
	END,
	END_OF_DIALOG
};

void Quest_Kopalnia::Start()
{
	quest_id = Q_KOPALNIA;
	type = 3;
	// start_loc ustawianie w Game::InitQuests
	dungeon_loc = -2;
}

DialogEntry* Quest_Kopalnia::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(strcmp(game->current_dialog->talker->data->id, "inwestor") == 0)
			return dialog_kopalnia_inwestor;
		else if(strcmp(game->current_dialog->talker->data->id, "poslaniec_kopalnia") == 0)
		{
			if(prog == 3)
				return dialog_kopalnia_poslaniec;
			else if(prog == 4 || prog == 5)
			{
				if(game->kopalnia_dni >= game->kopalnia_ile_dni)
					return dialog_kopalnia_poslaniec2;
				else
					return dialog_poslaniec;
			}
			else if(prog == 9)
			{
				if(game->kopalnia_dni >= game->kopalnia_ile_dni)
					return dialog_kopalnia_poslaniec3;
				else
					return dialog_poslaniec;
			}
			else if(prog == 10)
			{
				if(game->kopalnia_dni >= game->kopalnia_ile_dni)
					return dialog_kopalnia_poslaniec4;
				else
					return dialog_poslaniec;
			}
			else
				return dialog_poslaniec;
		}
		else
			return dialog_kopalnia_szef;
	}
	else
	{
		assert(0);
		return NULL;
	}
}

void Quest_Kopalnia::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		{
			prog = 1;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[131];

			location_event_handler = this;

			Location& sl = GetStartLocation();
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			else if(tl.state >= LS_VISITED)
				tl.reset = true;
			tl.st = 10;

			InitSub();

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[132], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[133], tl.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		{
			prog = 2;
			msgs.push_back(game->txQuest[134]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		{
			prog = 3;
			msgs.push_back(game->txQuest[135]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->kopalnia_stan = Game::KS_MASZ_UDZIALY;
			game->kopalnia_stan2 = Game::KS2_TRWA_BUDOWA;
			game->kopalnia_dni = 0;
			game->kopalnia_ile_dni = random(30,45);
			if(!game->plotka_questowa[P_KOPALNIA])
			{
				game->plotka_questowa[P_KOPALNIA] = true;
				--game->ile_plotek_questowych;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		{
			prog = 4;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[136]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(500);
			game->kopalnia_stan2 = Game::KS2_WYBUDOWANO;
			game->kopalnia_dni = 0;
			game->kopalnia_ile_dni = random(60,90);
			game->kopalnia_dni_zloto = 0;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		{
			prog = 5;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[137]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(3000);
			game->kopalnia_stan2 = Game::KS2_TRWA_BUDOWA;
			game->kopalnia_dni = 0;
			game->kopalnia_ile_dni = random(30,45);
			if(!game->plotka_questowa[P_KOPALNIA])
			{
				game->plotka_questowa[P_KOPALNIA] = true;
				--game->ile_plotek_questowych;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 6:
		{
			prog = 6;
			state = Quest::Started;
			msgs.push_back(game->txQuest[138]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->kopalnia_stan2 = Game::KS2_MOZLIWA_ROZBUDOWA;
			game->AddNews(Format(game->txQuest[139], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		{
			prog = 7;
			msgs.push_back(Format(game->txQuest[140], game->kopalnia_stan == 2 ? 10000 : 12000));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 8:
		{
			prog = 8;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[141]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 9:
		{
			if(game->kopalnia_stan == Game::KS_MASZ_UDZIALY)
				game->current_dialog->pc->unit->gold -= 10000;
			else
				game->current_dialog->pc->unit->gold -= 12000;
			prog = 9;
			msgs.push_back(game->txQuest[142]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->kopalnia_stan2 = Game::KS2_TRWA_ROZBUDOWA;
			game->kopalnia_dni = 0;
			game->kopalnia_ile_dni = random(30,45);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->GetPlayerInfo(game->current_dialog->pc).update_flags |= PlayerInfo::UF_GOLD;
			}
		}
		break;
	case 10:
		{
			prog = 10;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[143]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(1000);
			game->kopalnia_stan = Game::KS_MASZ_DUZE_UDZIALY;
			game->kopalnia_stan2 = Game::KS2_ROZBUDOWANO;
			game->kopalnia_dni = 0;
			game->kopalnia_ile_dni = random(60,90);
			game->kopalnia_dni_zloto = 0;
			game->AddNews(Format(game->txQuest[144], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 11:
		{
			prog = 11;
			state = Quest::Started;
			msgs.push_back(game->txQuest[145]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->kopalnia_stan2 = Game::KS2_ZNALEZIONO_PORTAL;
			game->AddNews(Format(game->txQuest[146], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 12:
		{
			prog = 12;
			msgs.push_back(game->txQuest[147]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			const Item* item = FindItem("key_kopalnia");
			game->current_dialog->pc->unit->AddItem(item, 1, true);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case 13:
		{
			prog = 13;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[148]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();
			game->AddNews(game->txQuest[149]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_Kopalnia::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "burmistrzem")
		return GetStartLocation().type == L_CITY ? game->txQuest[150] : game->txQuest[151];
	else if(str == "zloto")
		return Format("%d", game->kopalnia_stan == Game::KS_MASZ_UDZIALY ? 10000 : 12000);
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Kopalnia::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "kopalnia") == 0;
}

void Quest_Kopalnia::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == 1 && event == LocationEventHandler::CLEARED)
		SetProgress(2);
}

void Quest_Kopalnia::HandleChestEvent(ChestEventHandler::Event event)
{
	if(prog == 12 && event == ChestEventHandler::Opened)
		SetProgress(13);
}

void Quest_Kopalnia::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &sub.done, sizeof(sub.done), &tmp, NULL);
	WriteFile(file, &dungeon_loc, sizeof(dungeon_loc), &tmp, NULL);
}

void Quest_Kopalnia::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &sub.done, sizeof(sub.done), &tmp, NULL);
	ReadFile(file, &dungeon_loc, sizeof(dungeon_loc), &tmp, NULL);

	location_event_handler = this;
	InitSub();
}

void Quest_Kopalnia::InitSub()
{
	if(sub.done)
		return;

	sub.item_to_give[0] = FindItem("al_angelskin");
	sub.item_to_give[1] = FindItem("al_dragonskin");
	sub.item_to_give[2] = FindItem("ah_plate_adam");
	sub.spawn_item = Quest_Event::Item_InChest;
	sub.target_loc = dungeon_loc;
	sub.chest_event_handler = this;
	next_event = &sub;
}

//====================================================================================================================================================================================
// 1 - nie zaakceptowano
// 2 - zaakceptowano
// 3 - powiedzia³ co trzeba zrobiæ
// 4 - pokazano list
// 5 - gadka o liœcie
// 6 - trzeba porozmawiaæ z kapitanem stra¿y
// 7 - trzeba najechaæ obóz
// 8 - zabito bandytów, odliczanie do pojawienia siê agenta
// 9 - porozmawiano z agentem
// 10 - zabito szefa
// 11 - koniec
DialogEntry dialog_bandyci_mistrz[] = {
	IF_QUEST_PROGRESS(0),
		SPECIAL("tell_name"),
		TALK(271),
		TALK(272),
		TALK(273),
		TALK(274),
		TALK(275),
		TALK(276),
		CHOICE(277),
			SET_QUEST_PROGRESS(2),
			RESTART,
		END_CHOICE,
		CHOICE(278),
			SET_QUEST_PROGRESS(1),
			TALK(279),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(1),
		TALK(280),
		TALK(281),
		CHOICE(282),
			SET_QUEST_PROGRESS(2),
			RESTART,
		END_CHOICE,
		CHOICE(283),
			TALK(284),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(2),
		TALK(285),
		TALK(286),
		TALK(287),
		TALK(288),
		SET_QUEST_PROGRESS(3),
		TALK2(289),
		TALK(290),
		TALK(291),
		TALK(292),
		END,
	END_IF,
	IF_QUEST_PROGRESS(3),
		IF_HAVE_ITEM("q_bandyci_list"),
			TALK(293),
			CHOICE(294),
				SET_QUEST_PROGRESS(5),
				RESTART,
			END_CHOICE,
			CHOICE(295),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		END_IF,
		TALK(296),
		TALK(297),
		TALK(298),
		END,
	END_IF,
	IF_QUEST_PROGRESS(4),
		TALK(299),
		IF_HAVE_ITEM("q_bandyci_list"),
			CHOICE(300),
				SET_QUEST_PROGRESS(5),
				RESTART,
			END_CHOICE,
		END_IF,
		CHOICE(301),
			TALK(302),
			IF_HAVE_ITEM("q_bandyci_paczka"),
				SET_QUEST_PROGRESS(3),
				TALK(303),
			ELSE,
				SET_QUEST_PROGRESS(3),
				TALK(304),
			END_IF,
			TALK(305),
			END,				
		END_CHOICE,
		CHOICE(306),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(5),
		TALK(307),
		TALK(308),
		TALK(309),
		TALK(310),
		TALK(311),
		SET_QUEST_PROGRESS(6),
		TALK2(312),
		TALK(313),
		TALK(314),
		END,
	END_IF,
	IF_QUEST_PROGRESS(6),
		TALK(315),
		END,
	END_IF,
	IF_QUEST_PROGRESS(7),
		TALK(316),
		END,
	END_IF,
	IF_QUEST_PROGRESS(8),
		TALK(317),
		END,
	END_IF,
	IF_QUEST_PROGRESS(9),
		TALK(318),
		TALK(319),
		TALK2(320),
		TALK(321),
		END,
	END_IF,
	IF_QUEST_PROGRESS(10),
		TALK(322),
		TALK(323),
		TALK(324),
		TALK(325),
		TALK(326),
		SET_QUEST_PROGRESS(11),
		END,
	END_IF,
	IF_QUEST_PROGRESS(11),
		TALK(327),
		TALK(328),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_enc[] = {
	SET_QUEST_PROGRESS(4),
	TALK(329),
	TALK(330),
	IF_HAVE_ITEM("q_bandyci_paczka"),
		CHOICE(331),
			SPECIAL("bandyci_daj_paczke"),
			TALK(332),
			TALK(333),
			SPECIAL("attack"),
			END,
		END_CHOICE,
	ELSE,
		CHOICE(334),
			TALK(335),
			TALK(336),
			SPECIAL("attack"),
			END,
		END_CHOICE,
	END_IF,
	CHOICE(337),
		TALK(338),
		TALK(339),
		SPECIAL("attack"),
		END,
	END_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_kapitan_strazy[] = {
	TALK(340),
	TALK(341),
	TALK2(342),
	SET_QUEST_PROGRESS(7),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_straznik[] = {
	IF_QUEST_PROGRESS(7),
		RANDOM_TEXT(3),
			TALK(343),
			TALK(344),
			TALK(345),
	ELSE,
		RANDOM_TEXT(2),
			TALK(346),
			TALK(347),
	END_IF,
	END,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_agent[] = {
	IF_QUEST_PROGRESS(8),
		TALK(348),
		TALK(349),
		SET_QUEST_PROGRESS(9),
		TALK2(350),
		TALK(351),
		TALK(352),
	ELSE,
		TALK(353),
	END_IF,
	END,
	END_OF_DIALOG
};

DialogEntry dialog_bandyci_szef[] = {
	TALK(354),
	TALK(355),
	TALK(356),
	SPECIAL("attack"),
	END,
	END_OF_DIALOG
};

void Quest_Bandyci::Start()
{
	quest_id = Q_BANDYCI;
	type = 3;
	// start_loc ustawianie w Game::InitQuests
	enc = -1;
	other_loc = -1;
	camp_loc = -1;
	target_loc = -1;
	pewny_list = false;
}

DialogEntry* Quest_Bandyci::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);
	if(strcmp(game->current_dialog->talker->data->id, "mistrz_agentow") == 0)
		return dialog_bandyci_mistrz;
	else if(strcmp(game->current_dialog->talker->data->id, "guard_captain") == 0)
		return dialog_bandyci_kapitan_strazy;
	else if(strcmp(game->current_dialog->talker->data->id, "guard_q_bandyci") == 0)
		return dialog_bandyci_straznik;
	else if(strcmp(game->current_dialog->talker->data->id, "agent") == 0)
		return dialog_bandyci_agent;
	else if(strcmp(game->current_dialog->talker->data->id, "q_bandyci_szef") == 0)
		return dialog_bandyci_szef;
	else
		return dialog_bandyci_enc;
}

void WarpToThroneBanditBoss()
{
	Game& game = Game::Get();

	// szukaj orka
	UnitData* ud = FindUnitData("q_bandyci_szef");
	Unit* u = NULL;
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Useable* use = NULL;
	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_TRON)
		{
			use = *it;
			break;
		}
	}
	assert(use);

	// przenieœ
	game.WarpUnit(*u, use->pos);
}

void Quest_Bandyci::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		prog = 1;
		break;
	case 2:
		prog = 2;
		break;
	case 3:
		if(prog == 4)
		{
			prog = 3;
			const Item* item = FindItem("q_bandyci_paczka");
			if(!game->current_dialog->pc->unit->HaveItem(item))
			{
				game->current_dialog->pc->unit->AddItem(item, 1, true);
				if(game->IsOnline() && !game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			Location& sl = GetStartLocation();
			Location& other = *game->locations[other_loc];
			Encounter* e = game->AddEncounter(enc);
			e->dialog = dialog_q_bandyci;
			e->dont_attack = true;
			e->grupa = SG_BANDYCI;
			e->location_event_handler = NULL;
			e->pos = (sl.pos+other.pos)/2;
			e->quest = (Quest_Encounter*)this;
			e->szansa = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->zasieg = 72;
			msgs.push_back(game->txQuest[152]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		else
		{
			prog = 3;
			start_time = game->worldtime;
			state = Quest::Started;
			name = game->txQuest[153];
			const Item* item = FindItem("q_bandyci_paczka");
			game->current_dialog->pc->unit->AddItem(item, 1, true);
			other_loc = game->GetRandomCityLocation(start_loc);
			Location& sl = GetStartLocation();
			Location& other = *game->locations[other_loc];
			Encounter* e = game->AddEncounter(enc);
			e->dialog = dialog_q_bandyci;
			e->dont_attack = true;
			e->grupa = SG_BANDYCI;
			e->location_event_handler = NULL;
			e->pos = (sl.pos+other.pos)/2;
			e->quest = (Quest_Encounter*)this;
			e->szansa = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->zasieg = 72;
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[154], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[155], sl.name.c_str(), other.name.c_str(), GetLocationDirName(sl.pos, other.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(!game->plotka_questowa[P_BANDYCI])
			{
				game->plotka_questowa[P_BANDYCI] = true;
				--game->ile_plotek_questowych;
			}
			game->AddNews(Format(game->txQuest[156], GetStartLocationName()));

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case 4:
		// podczas rozmowy z bandytami, 66% szansy na znalezienie przy nich listu za 1 razem
		{
			prog = 4;
			if(pewny_list || rand2()%3 != 0)
				game->current_dialog->talker->AddItem(FindItem("q_bandyci_list"), 1, true);
			pewny_list = true;
			msgs.push_back(game->txQuest[157]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->RemoveEncounter(enc);
			enc = -1;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		prog = 5;
		break;
	case 6:
		// info o obozie
		{
			prog = 6;
			camp_loc = game->CreateCamp(GetStartLocation().pos, SG_BANDYCI);
			Location& camp = *game->locations[camp_loc];
			camp.st = 10;
			camp.state = LS_HIDDEN;
			camp.active_quest = this;
			msgs.push_back(game->txQuest[158]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			target_loc = camp_loc;
			location_event_handler = this;
			game->bandyci_gdzie = camp_loc;
			game->RemoveItem(*game->current_dialog->pc->unit, FindItem("q_bandyci_list"), 1);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// pozycja obozu
		{
			prog = 7;
			Location& camp = *game->locations[camp_loc];
			msgs.push_back(Format(game->txQuest[159], GetLocationDirName(GetStartLocation().pos, camp.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			camp.state = LS_KNOWN;
			game->bandyci_stan = Game::BS_WYGENERUJ_STRAZ;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(camp_loc, false);
			}
		}
		break;
	case 8:
		{
			prog = 8;
			game->bandyci_stan = Game::BS_ODLICZANIE;
			game->bandyci_czas = 7.5f;
			// zmieñ ai pod¹¿aj¹cych stra¿ników
			UnitData* ud = FindUnitData("guard_q_bandyci");
			for(vector<Unit*>::iterator it = game->local_ctx.units->begin(), end = game->local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->data == ud)
				{
					(*it)->assist = false;
					(*it)->ai->change_ai_mode = true;
				}
			}

			// news
			game->AddNews(Format(game->txQuest[160], GetStartLocationName()));
		}
		break;
	case 9:
		// porazmawiano z agentem, powiedzia³ gdzie jest skrytka i idzie sobie
		{
			prog = 9;
			game->bandyci_stan = Game::BS_AGENT_POGADAL;
			game->current_dialog->talker->hero->mode = HeroData::Leave;
			game->current_dialog->talker->event_handler = this;
			target_loc = game->CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, THRONE_VAULT, SG_BANDYCI, false);
			Location& target = *game->locations[target_loc];
			target.active_quest = this;
			target.state = LS_KNOWN;
			target.st = 10;
			game->locations[camp_loc]->active_quest = NULL;
			msgs.push_back(Format(game->txQuest[161], target.name.c_str(), GetLocationDirName(GetStartLocation().pos, target.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			done = false;
			at_level = 0;
			unit_to_spawn = FindUnitData("q_bandyci_szef");
			spawn_unit_room = POKOJ_CEL_TRON;
			unit_dont_attack = true;
			location_event_handler = NULL;
			unit_event_handler = this;
			unit_auto_talk = true;
			callback = WarpToThroneBanditBoss;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 10:
		// zabito szefa
		{
			camp_loc = -1;
			prog = 10;
			msgs.push_back(Format(game->txQuest[162], GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[163]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 11:
		// ukoñczono
		{
			prog = 11;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[164]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// ustaw arto na temporary ¿eby sobie poszed³
			game->current_dialog->talker->temporary = true;
			game->AddReward(5000);
			game->EndUniqueQuest();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

bool Quest_Bandyci::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "bandyci") == 0;
}

cstring Quest_Bandyci::FormatString(const string& str)
{
	if(str == "start_loc")
		return GetStartLocationName();
	else if(str == "other_loc")
		return game->locations[other_loc]->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[other_loc]->pos);
	else if(str == "camp_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[camp_loc]->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir_camp")
		return GetLocationDirName(game->locations[camp_loc]->pos, GetTargetLocation().pos);
	else
	{
		assert(0);
		return NULL;
	}
}

void Quest_Bandyci::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == 7 && event == LocationEventHandler::CLEARED)
		SetProgress(8);
}

void Quest_Bandyci::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::LEAVE)
	{
		if(unit == game->bandyci_agent)
		{
			game->bandyci_agent = NULL;
			game->bandyci_stan = Game::BS_AGENT_POSZEDL;
		}
	}
	else if(event == UnitEventHandler::DIE)
	{
		if(prog == 9)
		{
			SetProgress(10);
			unit->event_handler = NULL;
		}
	}
}

void Quest_Bandyci::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &enc, sizeof(enc), &tmp, NULL);
	WriteFile(file, &other_loc, sizeof(other_loc), &tmp, NULL);
	WriteFile(file, &camp_loc, sizeof(camp_loc), &tmp, NULL);
	WriteFile(file, &pewny_list, sizeof(pewny_list), &tmp, NULL);
}

void Quest_Bandyci::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &enc, sizeof(enc), &tmp, NULL);
	ReadFile(file, &other_loc, sizeof(other_loc), &tmp, NULL);
	ReadFile(file, &camp_loc, sizeof(camp_loc), &tmp, NULL);
	ReadFile(file, &pewny_list, sizeof(pewny_list), &tmp, NULL);

	if(enc != -1)
	{
		Encounter* e = game->RecreateEncounter(enc);
		e->dialog = dialog_q_bandyci;
		e->dont_attack = true;
		e->grupa = SG_BANDYCI;
		e->location_event_handler = NULL;
		e->pos = (GetStartLocation().pos+game->locations[other_loc]->pos)/2;
		e->quest = (Quest_Encounter*)this;
		e->szansa = 60;
		e->text = game->txQuest[11];
		e->timed = false;
		e->zasieg = 72;
	}

	if(prog == 6 || prog == 7)
		location_event_handler = this;
	else
		location_event_handler = NULL;

	if(prog == 9 && !done)
	{
		unit_to_spawn = FindUnitData("q_bandyci_szef");
		spawn_unit_room = POKOJ_CEL_TRON;
		unit_dont_attack = true;
		location_event_handler = NULL;
		unit_event_handler = this;
		unit_auto_talk = true;
		callback = WarpToThroneBanditBoss;
	}
}

//====================================================================================================================================================================================
DialogEntry dialog_magowie_uczony[] = {
	IF_QUEST_PROGRESS(0),
		TALK(357),
		TALK(358),
		TALK(359),
		TALK(360),
		TALK(361),
		CHOICE(362),
			SET_QUEST_PROGRESS(1),
			TALK2(363),
			TALK(364),
			END,
		END_CHOICE,
		CHOICE(365),
			TALK(366),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(1),
		TALK(367),
		IF_HAVE_ITEM("q_magowie_kula"),
			CHOICE(368),
				SET_QUEST_PROGRESS(2),
				TALK(369),
				TALK(370),
				END,
			END_CHOICE,
			CHOICE(371),
				TALK(372),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		ELSE,
			TALK(373),
			END,
		END_IF,
	END_IF,
	TALK(374),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_magowie_golem[] = {
	IF_QUEST_PROGRESS(2),
		SET_QUEST_PROGRESS(3),
	END_IF,
	IF_SPECIAL("q_magowie_zaplacono"),
		TALK(375),
		END,
	END_IF,
	TALK(376),
	CHOICE(377),
		SPECIAL("q_magowie_zaplac"),
		TALK(378),
		END,
	END_CHOICE,
	CHOICE(379),
		TALK(380),
		SPECIAL("attack"),
		END,
	END_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

void Quest_Magowie::Start()
{
	quest_id = Q_MAGOWIE;
	type = 3;
	// start_loc ustawiane w InitQuests
}

DialogEntry* Quest_Magowie::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_magowie_uczony") == 0)
		return dialog_magowie_uczony;
	else
		return dialog_magowie_golem;
}

void Quest_Magowie::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		{
			prog = 1;
			name = game->txQuest[165];
			start_time = game->worldtime;
			state = Quest::Started;

			Location& sl = GetStartLocation();
			target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.reset = true;
			tl.spawn = SG_NIEUMARLI;
			tl.st = 8;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			
			at_level = tl.GetLastLevel();
			item_to_give[0] = FindItem("q_magowie_kula");
			spawn_item = Quest_Event::Item_InTreasure;

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[166], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[167], tl.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2:
		{
			prog = 2;
			state = Quest::Completed;

			const Item* item = FindItem("q_magowie_kula");
			game->current_dialog->talker->AddItem(item, 1, true);
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->magowie_uczony = game->current_dialog->talker;
			game->magowie_stan = Game::MS_UCZONY_CZEKA;

			GetTargetLocation().active_quest = NULL;

			game->AddReward(1500);
			msgs.push_back(game->txQuest[168]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(!game->plotka_questowa[P_MAGOWIE])
			{
				game->plotka_questowa[P_MAGOWIE] = true;
				--game->ile_plotek_questowych;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		{
			prog = 3;

			Quest_Magowie2* q = new Quest_Magowie2;
			game->magowie_refid2 = q->refid = game->quest_counter;
			++game->quest_counter;
			q->Start();
			q->name = game->txQuest[169];
			q->start_time = game->worldtime;
			q->state = Quest::Started;
			game->magowie_stan = Game::MS_SPOTKANO_GOLEMA;
			game->quests.push_back(q);
			game->plotka_questowa[P_MAGOWIE2] = false;
			++game->ile_plotek_questowych;
			q->msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			q->msgs.push_back(game->txQuest[171]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[172]);

			if(game->IsOnline())
				game->Net_AddQuest(q->refid);
		}
		break;
	}
}

cstring Quest_Magowie::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Magowie::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "magowie") == 0;
}

void Quest_Magowie::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(!done)
	{
		item_to_give[0] = FindItem("q_magowie_kula");
		spawn_item = Quest_Event::Item_InTreasure;
	}
}

DialogEntry dialog_magowie2_kapitan[] = {
	IF_QUEST_PROGRESS(0),
		TALK(381),
		TALK(382),
		TALK(383),
		SET_QUEST_PROGRESS(1),
		TALK2(384),
		END,
	END_IF,
	IF_QUEST_PROGRESS(1),
		TALK2(385),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(2,4),
		TALK(386),
		TALK(387),
		TALK(388),
		END,
	END_IF,
	IF_QUEST_PROGRESS(5),
		TALK(389),
		END,
	END_IF,
	IF_QUEST_PROGRESS(6),
		TALK(390),
		TALK(391),
		TALK(392),
		SET_QUEST_PROGRESS(7),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(7,9),
		TALK(393),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(10,11),
		TALK(394),
		TALK(395),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(12,13),
		TALK2(396),
		TALK(397),
		TALK(398),
		SET_QUEST_PROGRESS(14),
		END,
	END_IF,
	IF_QUEST_PROGRESS(14),
		TALK(399),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_magowie2_stary[] = {
	IF_QUEST_PROGRESS_RANGE(13,14),
		TALK(400),
		END,
	END_IF,
	IF_QUEST_PROGRESS(12),
		TALK(401),
		TALK(402),
		TALK(403),
		SET_QUEST_PROGRESS(13),
		END,
	END_IF,
	IF_QUEST_PROGRESS(11),
		IF_SPECIAL("q_magowie_u_bossa"),
			TALK2(1290),
		ELSE,
			TALK2(404),
		END_IF,
		END,
	END_IF,
	IF_QUEST_PROGRESS(10),
		TALK(405),
		CHOICE(406),
			SET_QUEST_PROGRESS(11),
			TALK(407),
			END,
		END_CHOICE,
		CHOICE(408),
			TALK(409),
			TALK(410),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(9),
		TALK(411),
		TALK2(412),
		TALK(413),
		TALK(414),
		TALK2(415),
		TALK(416),
		CHOICE(417),
			SET_QUEST_PROGRESS(11),
			TALK(418),
			END,
		END_CHOICE,
		CHOICE(419),
			SET_QUEST_PROGRESS(10),
			TALK(420),
			TALK(421),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(6,8),
		TALK(422),
		IF_HAVE_ITEM("q_magowie_potion"),
			CHOICE(423),
				TALK(424),
				SET_QUEST_PROGRESS(9),
				TALK(425),
				TALK(426),
				RESTART,
			END_CHOICE,
			CHOICE(427),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		END_IF,
		END,
	END_IF,
	IF_QUEST_PROGRESS(5),
		IF_SPECIAL("q_magowie_u_siebie"),
			IF_SPECIAL("q_magowie_czas"),
				TALK(428),
				TALK(429),
				TALK(430),
				SET_QUEST_PROGRESS(6),
				END,
			ELSE,
				TALK(431),
				END,
			END_IF,
		ELSE,
			TALK2(432),
			END,
		END_IF,
	END_IF,
	IF_ONCE,
		IF_SPECIAL("is_not_known"),
			SPECIAL("tell_name"),
			TALK2(433),
		ELSE,
			TALK(434),
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(2),
		IF_HAVE_ITEM("potion_beer"),
			CHOICE(435),
				TALK(436),
				SET_QUEST_PROGRESS(3),
				TALK(437),
				TALK(438),
				END,
			END_CHOICE,
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(3),
		IF_HAVE_ITEM("potion_vodka"),
			CHOICE(439),
				TALK(440),
				SET_QUEST_PROGRESS(4),
				TALK(441),
				RESTART,
			END_CHOICE,
		END_IF,
	END_IF,
	CHOICE(442),
		IF_QUEST_PROGRESS(1),
			TALK(443),
			SET_QUEST_PROGRESS(2),
			END,
		END_IF,
		IF_QUEST_PROGRESS(2),
			TALK(444),
			END,
		END_IF,
		IF_QUEST_PROGRESS(3),
			TALK(445),
			END,
		END_IF,
		IF_QUEST_PROGRESS(4),
			TALK(446),
			TALK(447),
			TALK(448),
			TALK(449),
			TALK(450),
			TALK(451),
			CHOICE(452),
				SET_QUEST_PROGRESS(5),
				TALK2(453),
				TALK(454),
				END,
			END_CHOICE,
			CHOICE(455),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		END_IF,
	END_CHOICE,
	CHOICE(456),
		TALK(457),
		END,
	END_CHOICE,
	CHOICE(458),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_magowie2_boss[] = {
	TALK(459),
	IF_QUEST_PROGRESS(11),
		TALK2(460),
	END_IF,
	TALK(461),
	SPECIAL("attack"),
	END,
	END_OF_DIALOG
};

void Quest_Magowie2::Start()
{
	type = 3;
	quest_id = Q_MAGOWIE2;
	talked = 0;
}

DialogEntry* Quest_Magowie2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_magowie_stary") == 0)
		return dialog_magowie2_stary;
	else if(strcmp(game->current_dialog->talker->data->id, "q_magowie_boss") == 0)
		return dialog_magowie2_boss;
	else
		return dialog_magowie2_kapitan;
}

void Quest_Magowie2::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// porozmawiano ze stra¿nikiem o golemach, wys³a³ do maga
		{
			prog = 1;
			start_loc = game->current_location;
			game->magowie_miasto = start_loc;
			game->magowie_gdzie = mage_loc = game->GetRandomCityLocation(start_loc);

			Location& sl = GetStartLocation();
			Location& ml = *game->locations[mage_loc];

			msgs.push_back(Format(game->txQuest[173], sl.name.c_str(), ml.name.c_str(), GetLocationDirName(sl.pos, ml.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			game->magowie_stan = Game::MS_POROZMAWIANO_Z_KAPITANEM;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 2:
		// mag chce piwa
		{
			prog = 2;
			msgs.push_back(Format(game->txQuest[174], game->current_dialog->talker->hero->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 3:
		// daj piwo, chce wódy
		{
			const Item* piwo = FindItem("potion_beer");
			game->RemoveItem(*game->current_dialog->pc->unit, piwo, 1);
			game->current_dialog->talker->ConsumeItem(piwo->ToConsumeable());
			game->current_dialog->dialog_wait = 2.5f;
			prog = 3;
			msgs.push_back(game->txQuest[175]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// da³eœ wóde
		{
			prog = 4;
			const Item* woda = FindItem("potion_vodka");
			game->RemoveItem(*game->current_dialog->pc->unit, woda, 1);
			game->current_dialog->talker->ConsumeItem(woda->ToConsumeable());
			game->current_dialog->dialog_wait = 2.5f;
			msgs.push_back(game->txQuest[176]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// idzie za tob¹ do pustej wie¿y
		{
			prog = 5;
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -64.f, MAGE_TOWER, SG_BRAK, true, 2);
			Location& loc = *game->locations[target_loc];
			loc.st = 1;
			loc.state = LS_KNOWN;
			game->current_dialog->talker->hero->team_member = true;
			game->current_dialog->talker->hero->mode = HeroData::Follow;
			game->current_dialog->talker->hero->free = true;
			game->AddTeamMember(game->current_dialog->talker, false);
			msgs.push_back(Format(game->txQuest[177], game->current_dialog->talker->hero->name.c_str(), GetTargetLocationName(), GetLocationDirName(game->location->pos, GetTargetLocation().pos),
				game->location->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->magowie_gdzie = target_loc;
			game->magowie_stan = Game::MS_STARY_MAG_DOLACZYL;
			game->magowie_czas = 0;
			game->magowie_uczony = game->current_dialog->talker;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
				game->Net_RecruitNpc(game->current_dialog->talker);
			}
		}
		break;
	case 6:
		// mag sobie przypomnia³ ¿e to jego wie¿a
		{
			prog = 6;
			game->current_dialog->talker->auto_talk = 0;
			game->magowie_stan = Game::MS_PRZYPOMNIAL_SOBIE;
			msgs.push_back(Format(game->txQuest[178], game->current_dialog->talker->hero->name.c_str(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// cpt kaza³ pogadaæ z alchemikiem
		{
			prog = 7;
			game->magowie_stan = Game::MS_KUP_MIKSTURE;
			msgs.push_back(game->txQuest[179]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 8:
		// kupno miksturki
		// wywo³ywane z DT_IF_SPECAL q_magowie_kup
		{
			if(prog != 8)
			{
				msgs.push_back(game->txQuest[180]);
				game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
				game->AddGameMsg3(GMS_JOURNAL_UPDATED);

				if(game->IsOnline())
					game->Net_UpdateQuest(refid);
			}
			prog = 8;
			const Item* item = FindItem("q_magowie_potion");
			game->current_dialog->pc->unit->AddItem(item, 1, false);
			game->current_dialog->pc->unit->gold -= 150;

			if(game->IsOnline() && !game->current_dialog->is_local)
			{
				game->Net_AddItem(game->current_dialog->pc, item, false);
				game->Net_AddedItemMsg(game->current_dialog->pc);
				game->GetPlayerInfo(game->current_dialog->pc).update_flags |= PlayerInfo::UF_GOLD;
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case 9:
		// wypi³ miksturkê
		{
			prog = 9;
			const Item* mikstura = FindItem("q_magowie_potion");
			game->RemoveItem(*game->current_dialog->pc->unit, mikstura, 1);
			game->current_dialog->talker->ConsumeItem(mikstura->ToConsumeable());
			game->current_dialog->dialog_wait = 3.f;
			game->magowie_stan = Game::MS_MAG_WYLECZONY;
			msgs.push_back(game->txQuest[181]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			GetTargetLocation().active_quest = NULL;
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -64.f, MAGE_TOWER, SG_MAGOWIE_I_GOLEMY);
			Location& loc = GetTargetLocation();
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.active_quest = this;
			game->magowie_gdzie = mage_loc;
			do
			{
				game->GenerateHeroName(Class::MAGE, false, game->magowie_imie);
			}
			while(game->current_dialog->talker->hero->name == game->magowie_imie);
			done = false;
			unit_event_handler = this;
			unit_auto_talk = true;
			at_level = loc.GetLastLevel();
			unit_to_spawn = FindUnitData("q_magowie_boss");
			unit_dont_attack = true;
			unit_to_spawn2 = FindUnitData("golem_iron");
			spawn_2_guard_1 = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 10:
		// nie zrekrutowa³em maga
		{
			prog = 10;
			Unit* u = game->current_dialog->talker;
			u->hero->free = false;
			u->hero->team_member = false;
			u->MakeItemsTeam(true);
			RemoveElement(game->team, u);
			game->magowie_stan = Game::MS_MAG_IDZIE;

			if(game->current_location == start_loc)
			{
				// idŸ do karczmy
				u->ai->goto_inn = true;
				u->ai->timer = 0.f;
			}
			else
			{
				// idŸ do startowej lokacji do karczmy
				u->hero->mode = HeroData::Leave;
				u->event_handler = this;
			}

			Location& target = GetTargetLocation();
			target.state = LS_KNOWN;

			msgs.push_back(Format(game->txQuest[182], u->hero->name.c_str(), game->magowie_imie.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
				game->Net_KickNpc(u);
			}
		}
		break;
	case 11:
		// zrekrutowa³em maga
		{
			Unit* u = game->current_dialog->talker;
			Location& target = GetTargetLocation();

			if(prog == 9)
			{
				target.state = LS_KNOWN;
				msgs.push_back(Format(game->txQuest[183], u->hero->name.c_str(), game->magowie_imie.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			}
			else
			{
				msgs.push_back(Format(game->txQuest[184], u->hero->name.c_str()));
				u->hero->free = true;
				u->hero->mode = HeroData::Follow;
				u->hero->team_member = true;
				u->ai->goto_inn = false;
				game->AddTeamMember(u, false);
				game->magowie_imie_dobry.clear();
			}

			if(game->IsOnline())
			{
				if(prog == 9)
					game->Net_ChangeLocationState(target_loc, false);
				else
					game->Net_RecruitNpc(u);
				game->Net_UpdateQuest(refid);
			}

			prog = 11;
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->magowie_stan = Game::MS_MAG_ZREKRUTOWANY;
		}
		break;
	case 12:
		// zabito maga
		{
			prog = 12;
			if(game->magowie_stan == Game::MS_MAG_ZREKRUTOWANY)
				game->magowie_uczony->auto_talk = 1;
			game->magowie_stan = Game::MS_UKONCZONO;
			msgs.push_back(game->txQuest[185]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[186]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 13:
		// porozmawiano z magiem po
		{
			prog = 13;
			msgs.push_back(Format(game->txQuest[187], game->current_dialog->talker->hero->name.c_str(), game->magowie_imie.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// idŸ sobie
			Unit* u = game->current_dialog->talker;
			u->hero->free = false;
			u->hero->team_member = false;
			u->hero->mode = HeroData::Leave;
			u->MakeItemsTeam(true);
			RemoveElement(game->team, u);
			game->magowie_uczony = NULL;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_KickNpc(u);
			}
		}
		break;
	case 14:
		// odebrano nagrodê
		{
			prog = 14;
			GetTargetLocation().active_quest = NULL;
			state = Quest::Completed;
			if(game->magowie_uczony)
			{
				game->magowie_uczony->temporary = true;
				game->magowie_uczony = NULL;
			}
			game->AddReward(5000);
			msgs.push_back(game->txQuest[188]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();
			if(!game->plotka_questowa[P_MAGOWIE2])
			{
				game->plotka_questowa[P_MAGOWIE2] = true;
				--game->ile_plotek_questowych;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_Magowie2::FormatString(const string& str)
{
	if(str == "mage_loc")
		return game->locations[mage_loc]->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[mage_loc]->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(game->location->pos, GetTargetLocation().pos);
	else if(str == "name")
		return game->current_dialog->talker->hero->name.c_str();
	else if(str == "enemy")
		return game->magowie_imie.c_str();
	else if(str == "dobry")
		return game->magowie_uczony->hero->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Magowie2::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "magowie2") == 0;
}

void Quest_Magowie2::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
{
	if(unit == game->magowie_uczony)
	{
		if(type == UnitEventHandler::LEAVE)
		{
			game->magowie_imie_dobry = unit->hero->name;
			game->magowie_hd.Set(*unit->human_data);
			game->magowie_stan = Game::MS_MAG_POSZEDL;
			game->magowie_uczony = NULL;
		}
	}
	else if(strcmp(unit->data->id, "q_magowie_boss") == 0 && type == UnitEventHandler::DIE && prog != 12)
	{
		SetProgress(12);
		unit->event_handler = NULL;
	}
}

void Quest_Magowie2::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &mage_loc, sizeof(mage_loc), &tmp, NULL);
	WriteFile(file, &talked, sizeof(talked), &tmp, NULL);
}

void Quest_Magowie2::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &mage_loc, sizeof(mage_loc), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		talked = 0;
	else
		ReadFile(file, &talked, sizeof(talked), &tmp, NULL);

	if(!done && prog >= 9)
	{
		unit_event_handler = this;
		unit_auto_talk = true;
		at_level = GetTargetLocation().GetLastLevel();
		unit_to_spawn = FindUnitData("q_magowie_boss");
		unit_dont_attack = true;
		unit_to_spawn2 = FindUnitData("golem_iron");
		spawn_2_guard_1 = true;
	}
}

//====================================================================================================================================================================================
DialogEntry dialog_orkowie_straznik[] = {
	TALK(462),
	IF_QUEST_PROGRESS(0),
		TALK(463),
		TALK(464),
		TALK(465),
		SET_QUEST_PROGRESS(1),
	END_IF,
	END,
	END_OF_DIALOG
};

DialogEntry dialog_orkowie_kapitan[] = {
	IF_QUEST_PROGRESS_RANGE(0,2),
		TALK(466),
		TALK(467),
		CHOICE(468),
			SET_QUEST_PROGRESS(3),
			TALK2(469),
			TALK(470),
			END,
		END_CHOICE,
		CHOICE(471),
			SET_QUEST_PROGRESS(2),
			TALK(472),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	ELSE,
		IF_QUEST_PROGRESS(3),
			TALK2(473),
			END,
		END_IF,
		IF_QUEST_PROGRESS(4),
			TALK(474),
			SET_QUEST_PROGRESS(5),
			TALK(475),
			IF_SPECIAL("q_orkowie_dolaczyl"),
				TALK(476),
			END_IF,
			END,
		END_IF,
		IF_QUEST_PROGRESS(5),
			TALK2(477),
			END,
		END_IF,
	END_IF,
	END_OF_DIALOG
};

void Quest_Orkowie::Start()
{
	quest_id = Q_ORKOWIE;
	type = 3;
}

DialogEntry* Quest_Orkowie::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_orkowie_straznik") == 0)
		return dialog_orkowie_straznik;
	else
		return dialog_orkowie_kapitan;
}

void Quest_Orkowie::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		{
			if(prog != 0)
				return;
			prog = 1;
			// dodaj plotkê
			if(!game->plotka_questowa[P_ORKOWIE])
			{
				game->plotka_questowa[P_ORKOWIE] = true;
				--game->ile_plotek_questowych;
				cstring text = Format(game->txQuest[189], game->locations[start_loc]->name.c_str());
				game->plotki.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->plotki.size())-1;
				}
			}
			game->orkowie_stan = Game::OS_STRAZNIK_POGADAL;
		}
		break;
	case 2:
		{
			prog = 2;
			// dodaj plotkê
			if(!game->plotka_questowa[P_ORKOWIE])
			{
				game->plotka_questowa[P_ORKOWIE] = true;
				--game->ile_plotek_questowych;
				cstring text = Format(game->txQuest[190], game->locations[start_loc]->name.c_str());
				game->plotki.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->plotki.size())-1;
				}
			}
			// usuñ stra¿nika
			if(game->orkowie_straznik)
			{
				game->orkowie_straznik->auto_talk = 0;
				game->orkowie_stan = Game::OS_STRAZNIK_POGADAL;
				game->orkowie_straznik->temporary = true;
				game->orkowie_straznik = NULL;
			}
		}
		break;
	case 3:
		{
			prog = 3;
			// usuñ plotkê
			if(!game->plotka_questowa[P_ORKOWIE])
			{
				game->plotka_questowa[P_ORKOWIE] = true;
				--game->ile_plotek_questowych;
			}
			// usuñ stra¿nika
			if(game->orkowie_straznik)
			{
				game->orkowie_straznik->auto_talk = false;
				game->orkowie_stan = Game::OS_STRAZNIK_POGADAL;
				game->orkowie_straznik->temporary = true;
				game->orkowie_straznik = NULL;
			}
			// generuj lokacje
			target_loc = game->CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, HUMAN_FORT, SG_ORKOWIE, false);
			Location& tl = GetTargetLocation();
			tl.state = LS_KNOWN;
			tl.st = 10;
			tl.active_quest = this;
			location_event_handler = this;
			at_level = tl.GetLastLevel();
			dungeon_levels = at_level+1;
			levels_cleared = 0;
			whole_location_event_handler = true;
			item_to_give[0] = FindItem("q_orkowie_klucz");
			spawn_item = Quest_Event::Item_GiveSpawned2;
			game->orkowie_gdzie = target_loc;
			unit_to_spawn = FindUnitData("q_orkowie_gorush");
			unit_to_spawn2 = FindUnitData("orc_chief_q");
			unit_spawn_level2 = -3;
			spawn_unit_room = POKOJ_CEL_WIEZIENIE;
			game->orkowie_stan = Game::OS_ZAAKCEPTOWANO;
			// questowe rzeczy
			state = Quest::Started;
			name = game->txQuest[191];
			start_time = game->worldtime;
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[192], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[193], GetStartLocationName(), GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 4:
		// oczyszczono lokacjê
		{
			prog = 4;
			msgs.push_back(Format(game->txQuest[194], GetTargetLocationName(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// ukoñczono - nagroda
		{
			prog = 5;
			state = Quest::Completed;
			if(game->orkowie_stan == Game::OS_ORK_DOLACZYL)
			{
				game->orkowie_stan = Game::OS_UKONCZONO_DOLACZYL;
				// odliczanie
				game->orkowie_dni = random(60,90);
				GetTargetLocation().active_quest = NULL;
			}
			else
				game->orkowie_stan = Game::OS_UKONCZONO;
			game->AddReward(2500);
			msgs.push_back(game->txQuest[195]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[196], GetTargetLocationName(), GetStartLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_Orkowie::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "naszego_miasta")
		return GetStartLocation().type == L_CITY ? game->txQuest[72] : game->txQuest[73];
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Orkowie::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "orkowie") == 0;
}

void Quest_Orkowie::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == 3)
	{
		levels_cleared |= (1<<game->dungeon_level);
		if(count_bits(levels_cleared) == dungeon_levels)
			SetProgress(4);
	}
}

void Quest_Orkowie::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &dungeon_levels, sizeof(dungeon_levels), &tmp, NULL);
	WriteFile(file, &levels_cleared, sizeof(levels_cleared), &tmp, NULL);
}

void Quest_Orkowie::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &dungeon_levels, sizeof(dungeon_levels), &tmp, NULL);
	ReadFile(file, &levels_cleared, sizeof(levels_cleared), &tmp, NULL);

	location_event_handler = this;
	whole_location_event_handler = true;

	if(!done)
	{
		item_to_give[0] = FindItem("q_orkowie_klucz");
		spawn_item = Quest_Event::Item_GiveSpawned2;
		unit_to_spawn = FindUnitData("q_orkowie_gorush");
		unit_to_spawn2 = FindUnitData("orc_chief_q");
		unit_spawn_level2 = -3;
		spawn_unit_room = POKOJ_CEL_WIEZIENIE;
	}
}

DialogEntry dialog_orkowie2_gorush[] = {
	IF_QUEST_PROGRESS(16),
		TALK(478),
		END,
	END_IF,
	IF_QUEST_PROGRESS(15),
		TALK(479),
		TALK(480),
		TALK(481),
		TALK(482),
		TALK(483),
		TALK(484),
		SET_QUEST_PROGRESS(16),
		END,
	END_IF,
	IF_QUEST_PROGRESS(6),
		TALK(485),
		TALK(486),
		TALK(487),
		TALK(488),
		SET_QUEST_PROGRESS(7),
		END,
	END_IF,
	IF_QUEST_EVENT,
		IF_QUEST_PROGRESS(12),
			TALK(489),
			TALK(490),
			TALK(491),
			SET_QUEST_PROGRESS(13),
			END,
		END_IF,
		IF_QUEST_PROGRESS(7),
			TALK(492),
			TALK(493),
			TALK(494),
			CHOICE(495),
				TALK(496),
				SET_QUEST_PROGRESS(8),
				END,
			END_CHOICE,
			CHOICE(497),
				TALK(498),
				SET_QUEST_PROGRESS(9),
				END,
			END_CHOICE,
			CHOICE(499),
				TALK(500),
				SET_QUEST_PROGRESS(10),
				END,
			END_CHOICE,
			CHOICE(501),
				SET_QUEST_PROGRESS(11),
				IF_SPECIAL("q_orkowie_woj"),
					TALK(502),
					SET_QUEST_PROGRESS(8),
				ELSE,
					IF_SPECIAL("q_orkowie_lowca"),
						TALK(503),
						SET_QUEST_PROGRESS(9),
					ELSE,
						TALK(504),
						SET_QUEST_PROGRESS(10),
					END_IF,
				END_IF,
				END,
			END_CHOICE,
			SHOW_CHOICES,
		END_IF,
		TALK(505),
		TALK(506),
		TALK(507),
		TALK(508),
		TALK(509),
		SET_QUEST_PROGRESS(4),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(0,1),
		SET_QUEST_PROGRESS(1),
		TALK2(510),
		TALK(511),
		TALK(512),
		CHOICE(513),
			TALK(514),
			SET_QUEST_PROGRESS(3),
			END,
		END_CHOICE,
		CHOICE(515),
			TALK(516),
			SET_QUEST_PROGRESS(2),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(2),
		TALK(517),
		TALK(518),
		CHOICE(519),
			TALK(520),
			SET_QUEST_PROGRESS(3),
			END,
		END_CHOICE,
		CHOICE(521),
			TALK(522),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	RANDOM_TEXT(3),
		TALK(523),
		TALK(524),
		TALK(525),
	IF_QUEST_PROGRESS_RANGE(4,5),
		CHOICE(526),
			IF_SPECIAL("q_orkowie_na_miejscu"),
				TALK(527),
			ELSE,
				TALK2(528),
				SET_QUEST_PROGRESS(5),
			END_IF,
			END,
		END_CHOICE,
	END_IF,
	IF_QUEST_PROGRESS(13),
		CHOICE(529),
			TALK2(530),
			TALK(531),
			TALK(532),
			SET_QUEST_PROGRESS(14),
			END,
		END_CHOICE,
	END_IF,
	CHOICE(533),
		TALK(534),
		TALK(535),
		TALK(536),
		TALK(537),
		TALK(538),
		TALK(539),
		TALK(540),
		TALK(541),
		TALK(542),
		END,
	END_CHOICE,
	CHOICE(543),
		TALK(544),
		TALK(545),
		END,
	END_CHOICE,
	CHOICE(546),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

DialogEntry dialog_orkowie2_slaby[] = {
	IF_QUEST_PROGRESS(16),
		TALK(547),
	ELSE,
		RANDOM_TEXT(2),
			TALK(548),
			TALK(549),
	END_IF,
	END,
	END_OF_DIALOG
};

DialogEntry dialog_orkowie2_kowal[] = {
	IF_QUEST_PROGRESS(16),
		TALK(550),
		CHOICE(551),
			TRADE,
			END,
		END_CHOICE,
		CHOICE(552),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	ELSE,
		TALK(553),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_orkowie2_ork[] = {
	RANDOM_TEXT(3),
		TALK(554),
		TALK(555),
		TALK(556),
	END,
	END_OF_DIALOG
};

void Quest_Orkowie2::Start()
{
	quest_id = Q_ORKOWIE2;
	type = 3;
	start_loc = -1;
	near_loc = -1;
	talked = 0;
}

DialogEntry* Quest_Orkowie2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

#define TALKER(x) (strcmp(game->current_dialog->talker->data->id, x) == 0)

	if(TALKER("q_orkowie_slaby"))
		return dialog_orkowie2_slaby;
	else if(TALKER("q_orkowie_kowal"))
		return dialog_orkowie2_kowal;
	else if(TALKER("q_orkowie_gorush") || TALKER("q_orkowie_gorush_woj") || TALKER("q_orkowie_gorush_lowca") || TALKER("q_orkowie_gorush_szaman"))
		return dialog_orkowie2_gorush;
	else
		return dialog_orkowie2_ork;
}

void WarpToThroneOrcBoss()
{
	Game& game = Game::Get();

	// szukaj orka
	UnitData* ud = FindUnitData("q_orkowie_boss");
	Unit* u = NULL;
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Useable* use = NULL;
	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_TRON)
		{
			use = *it;
			break;
		}
	}
	assert(use);

	// przenieœ
	game.WarpUnit(*u, use->pos);
}

void Quest_Orkowie2::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// zapisz gorusha
		{
			prog = 1;
			game->orkowie_gorush = game->current_dialog->talker;
			game->current_dialog->talker->hero->know_name = true;
			game->current_dialog->talker->hero->name = game->txQuest[216];
		}
		break;
	case 2:
		prog = 2;
		break;
	case 3:
		// dodaj questa
		{
			prog = 3;
			start_time = game->worldtime;
			name = game->txQuest[214];
			state = Quest::Started;
			msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			msgs.push_back(game->txQuest[197]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			// ustaw stan
			if(game->orkowie_stan == Game::OS_ZAAKCEPTOWANO)
				game->orkowie_stan = Game::OS_ORK_DOLACZYL;
			else
			{
				game->orkowie_stan = Game::OS_UKONCZONO_DOLACZYL;
				game->orkowie_dni = random(60,90);
				((Quest_Orkowie*)game->FindQuest(game->orkowie_refid))->target_loc = NULL;
			}
			// do³¹cz do dru¿yny
			game->current_dialog->talker->hero->free = true;
			game->current_dialog->talker->hero->team_member = true;
			game->current_dialog->talker->hero->mode = HeroData::Follow;
			game->AddTeamMember(game->current_dialog->talker, false);
			game->free_recruit = false;

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				game->Net_RecruitNpc(game->current_dialog->talker);
			}
		}
		break;
	case 4:
		// powiedzia³ o obozie
		{
			prog = 4;
			target_loc = game->CreateCamp(game->world_pos, SG_ORKOWIE, 256.f, false);
			done = false;
			location_event_handler = this;
			Location& target = GetTargetLocation();
			target.state = LS_HIDDEN;
			target.st = 13;
			target.active_quest = this;
			near_loc = game->GetNearestLocation2(target.pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
			msgs.push_back(game->txQuest[198]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->orkowie_stan = Game::OS_POWIEDZIAL_O_OBOZIE;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// powiedzia³ gdzie obóz
		{
			if(prog == 5)
				break;
			prog = 5;
			Location& target = GetTargetLocation();
			Location& nearl = *game->locations[near_loc];
			target.state = LS_KNOWN;
			msgs.push_back(Format(game->txQuest[199], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 6:
		// oczyszczono obóz orków
		{
			prog = 6;
			game->orkowie_gorush->auto_talk = 1;
			game->AddNews(game->txQuest[200]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// pogada³ po oczyszczeniu
		{
			prog = 7;
			game->orkowie_stan = Game::OS_OCZYSZCZONO;
			game->orkowie_dni = random(30,60);
			GetTargetLocation().active_quest = NULL;
			target_loc = -1;
			msgs.push_back(game->txQuest[201]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 8:
		// zostañ wojownikiem
		ChangeClass(Game::GORUSH_WOJ);
		break;
	case 9:
		// zostañ ³owc¹
		ChangeClass(Game::GORUSH_LOWCA);
		break;
	case 10:
		// zostañ szamanem
		ChangeClass(Game::GORUSH_SZAMAN);
		break;
	case 11:
		// losowo
		{
			Game::GorushKlasa klasa;
			if(game->current_dialog->pc->unit->player->clas == Class::WARRIOR)
			{
				if(rand2()%2 == 0)
					klasa = Game::GORUSH_LOWCA;
				else
					klasa = Game::GORUSH_SZAMAN;
			}
			else if(game->current_dialog->pc->unit->player->clas == Class::HUNTER)
			{
				if(rand2()%2 == 0)
					klasa = Game::GORUSH_WOJ;
				else
					klasa = Game::GORUSH_SZAMAN;
			}
			else
			{
				int co = rand2()%3;
				if(co == 0)
					klasa = Game::GORUSH_WOJ;
				else if(co == 1)
					klasa = Game::GORUSH_LOWCA;
				else
					klasa = Game::GORUSH_SZAMAN;
			}

			ChangeClass(klasa);
		}
		break;
	case 13:
		// pogada³ o bazie
		{
			prog = 13;
			target_loc = game->CreateLocation(L_DUNGEON, game->world_pos, 256.f, THRONE_FORT, SG_ORKOWIE, false);
			Location& target = GetTargetLocation();
			done = false;
			target.st = 15;
			target.active_quest = this;
			target.state = LS_HIDDEN;
			msgs.push_back(game->txQuest[202]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->orkowie_stan = Game::OS_POWIEDZIAL_O_BAZIE;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 14:
		// powiedzia³ gdzie baza
		{
			prog = 14;
			Location& target = GetTargetLocation();
			target.state = LS_KNOWN;
			unit_to_spawn = FindUnitData("q_orkowie_boss");
			spawn_unit_room = POKOJ_CEL_TRON;
			callback = WarpToThroneOrcBoss;
			at_level = target.GetLastLevel();
			location_event_handler = NULL;
			unit_event_handler = this;
			near_loc = game->GetNearestLocation2(target.pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
			Location& nearl = *game->locations[near_loc];
			msgs.push_back(Format(game->txQuest[203], GetLocationDirName(nearl.pos, target.pos), nearl.name.c_str(), target.name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->orkowie_gdzie = target_loc;
			done = false;
			game->orkowie_stan = Game::OS_GENERUJ_ORKI;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 15:
		// zabito bossa
		{
			prog = 15;
			game->orkowie_gorush->auto_talk = 1;
			msgs.push_back(game->txQuest[204]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[205]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 16:
		// pogadano z gorushem
		{
			prog = 16;
			state = Quest::Completed;
			game->AddReward(random(4000, 5000));
			msgs.push_back(game->txQuest[206]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();
			// gorush
			game->orkowie_gorush->hero->team_member = false;
			RemoveElementOrder(game->team, game->orkowie_gorush);
			game->orkowie_gorush->MakeItemsTeam(true);
			Useable* tron = game->FindUseableByIdLocal(U_TRON);
			assert(tron);
			if(tron)
			{
				game->orkowie_gorush->ai->idle_action = AIController::Idle_WalkUse;
				game->orkowie_gorush->ai->idle_data.useable = tron;
				game->orkowie_gorush->ai->timer = 9999.f;
			}
			game->orkowie_gorush = NULL;
			// orki
			UnitData* ud[10] = {
				FindUnitData("orc"), FindUnitData("q_orkowie_orc"),
				FindUnitData("orc_fighter"), FindUnitData("q_orkowie_orc_fighter"),
				FindUnitData("orc_hunter"), FindUnitData("q_orkowie_orc_hunter"),
				FindUnitData("orc_shaman"), FindUnitData("q_orkowie_orc_shaman"),
				FindUnitData("orc_chief"), FindUnitData("q_orkowie_orc_chief")
			};
			UnitData* ud_slaby = FindUnitData("q_orkowie_slaby");

			for(vector<Unit*>::iterator it = game->local_ctx.units->begin(), end = game->local_ctx.units->end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsAlive())
				{
					if(u.data == ud_slaby)
					{
						// usuñ dont_attack, od tak :3
						u.dont_attack = false;
						u.ai->change_ai_mode = true;
					}
					else
					{
						for(int i=0; i<5; ++i)
						{
							if(u.data == ud[i*2])
							{
								u.data = ud[i*2+1];
								u.ai->target = NULL;
								u.ai->alert_target = NULL;
								u.ai->state = AIController::Idle;
								u.ai->change_ai_mode = true;
								if(game->IsOnline())
								{
									NetChange& c = Add1(game->net_changes);
									c.type = NetChange::CHANGE_UNIT_BASE;
									c.unit = &u;
								}
								break;
							}
						}
					}
				}
			}
			// zak³ada ¿e gadamy na ostatnim levelu, mam nadzieje ¿e gracz z tamt¹d nie spierdoli przed pogadaniem :3
			MultiInsideLocation* multi = (MultiInsideLocation*)game->location;
			for(vector<InsideLocationLevel>::iterator it = multi->levels.begin(), end = multi->levels.end()-1; it != end; ++it)
			{
				for(vector<Unit*>::iterator it2 = it->units.begin(), end2 = it->units.end(); it2 != end2; ++it2)
				{
					Unit& u = **it2;
					if(u.IsAlive())
					{
						for(int i=0; i<5; ++i)
						{
							if(u.data == ud[i*2])
							{
								u.data = ud[i*2+1];
								break;
							}
						}
					}
				}
			}
			// usuñ zw³oki po opuszczeniu lokacji
			game->orkowie_stan = Game::OS_WYCZYSC;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_KickNpc(game->current_dialog->talker);
			}
		}
		break;
	}
}

cstring Quest_Orkowie2::FormatString(const string& str)
{
	if(str == "name")
		return game->orkowie_gorush->hero->name.c_str();
	else if(str == "close")
		return game->locations[near_loc]->name.c_str();
	else if(str == "close_dir")
		return GetLocationDirName(game->locations[near_loc]->pos, GetTargetLocation().pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(game->world_pos, GetTargetLocation().pos);
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Orkowie2::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "orkowie2") == 0;
}

bool Quest_Orkowie2::IfQuestEvent()
{
	return (game->orkowie_stan == Game::OS_UKONCZONO_DOLACZYL ||
		game->orkowie_stan == Game::OS_OCZYSZCZONO ||
		game->orkowie_stan == Game::OS_WYBRAL_KLASE)
		&& game->orkowie_dni <= 0;
}

void Quest_Orkowie2::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == 5)
		SetProgress(6);
}

void Quest_Orkowie2::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::DIE && prog == 14)
	{
		SetProgress(15);
		unit->event_handler = NULL;
	}
}

void Quest_Orkowie2::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &near_loc, sizeof(near_loc), &tmp, NULL);
	WriteFile(file, &talked, sizeof(talked), &tmp, NULL);
}

void Quest_Orkowie2::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &near_loc, sizeof(near_loc), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		talked = 0;
	else
		ReadFile(file, &talked, sizeof(talked), &tmp, NULL);

	if(!done)
	{
		if(prog == 4)
			location_event_handler = this;
		else if(prog == 14)
		{
			unit_to_spawn = FindUnitData("q_orkowie_boss");
			spawn_unit_room = POKOJ_CEL_TRON;
			location_event_handler = NULL;
			unit_event_handler = this;
			callback = WarpToThroneOrcBoss;
		}
	}
}

void Quest_Orkowie2::ChangeClass(int klasa)
{
	cstring nazwa, udi;
	Class clas;

	switch(klasa)
	{
	case Game::GORUSH_WOJ:
	default:
		nazwa = game->txQuest[207];
		udi = "q_orkowie_gorush_woj";
		clas = Class::WARRIOR;
		break;
	case Game::GORUSH_LOWCA:
		nazwa = game->txQuest[208];
		udi = "q_orkowie_gorush_lowca";
		clas = Class::HUNTER;
		break;
	case Game::GORUSH_SZAMAN:
		nazwa = game->txQuest[209];
		udi = "q_orkowie_gorush_szaman";
		clas = Class::MAGE;
		break;
	}

	UnitData* ud = FindUnitData(udi);
	Unit* u = game->orkowie_gorush;
	u->hero->clas = clas;

	u->level = ud->level.x;
	u->data->GetStatProfile().Set(u->level, u->unmod_stats.attrib, u->unmod_stats.skill);
	u->CalculateStats();
	u->RecalculateHp();
	u->data = ud;
	game->ParseItemScript(*u, ud->items);
	u->MakeItemsTeam(false);
	game->UpdateUnitInventory(*u);

	msgs.push_back(Format(game->txQuest[210], nazwa));
	game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	prog = 12;
	game->orkowie_stan = Game::OS_WYBRAL_KLASE;
	game->orkowie_dni = random(60,90);

	if(clas == Class::WARRIOR)
		u->hero->melee = true;

	if(game->IsOnline())
		game->Net_UpdateQuest(refid);
}

//====================================================================================================================================================================================
DialogEntry dialog_gobliny_szlachcic[] = {
	IF_QUEST_PROGRESS(0),
		TALK(557),
		SPECIAL("tell_name"),
		TALK(558),
		TALK(559),
		TALK(560),
		TALK(561),
		TALK(562),
		CHOICE(563),
			SET_QUEST_PROGRESS(2),
			TALK2(564),
			TALK(565),
			TALK(566),
			END,
		END_CHOICE,
		CHOICE(567),
			TALK(568),
			TALK(569),
			SET_QUEST_PROGRESS(1),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(1),
		TALK(570),
		TALK(571),
		CHOICE(572),
			SET_QUEST_PROGRESS(2),
			TALK2(573),
			TALK(574),
			TALK(575),
			END,
		END_CHOICE,
		CHOICE(576),
			TALK(577),
			TALK(578),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(2),
		TALK(579),
		END,
	END_IF,
	IF_QUEST_PROGRESS(3),
		TALK(580),
		CHOICE(581),
			SET_QUEST_PROGRESS(4),
			TALK(582),
			TALK(583),
			END,
		END_CHOICE,
		CHOICE(584),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(4),
		TALK(585),
		END,
	END_IF,
	IF_QUEST_PROGRESS(5),
		IF_HAVE_ITEM("q_gobliny_luk"),
			TALK(586),
			CHOICE(587),
				TALK(588),
				TALK(589),
				TALK(590),
				TALK(591),
				SET_QUEST_PROGRESS(6),
				END,
			END_CHOICE,
			CHOICE(592),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		ELSE,
			TALK(593),
			END,
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(6),
		TALK(594),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_gobliny_atak[] = {
	SET_QUEST_PROGRESS(3),
	TALK(595),
	TALK(596),
	SPECIAL("attack"),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_gobliny_poslaniec[] = {
	IF_QUEST_PROGRESS(4),
		TALK(597),
		TALK(598),
		TALK(599),
		SET_QUEST_PROGRESS(5),
		TALK2(600),
		TALK(601),
		END,
	ELSE,
		TALK(602),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_gobliny_mag[] = {
	IF_QUEST_PROGRESS(6),
		TALK(603),
		TALK(604),
		TALK(605),
		CHOICE(606),
			TALK(607),
			TALK(608),
			SET_QUEST_PROGRESS(8),
			END,
		END_CHOICE,
		CHOICE(609),
			SET_QUEST_PROGRESS(7),
			TALK(610),
			END,
		END_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(7),
		TALK(611),
		TALK(612),
		CHOICE(613),
			IF_SPECIAL("have_100"),
				TALK(614),
				TALK(615),
				SET_QUEST_PROGRESS(9),
				END,
			ELSE,
				TALK(616),
				END,
			END_IF,
		END_CHOICE,
		CHOICE(617),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	TALK2(618),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_gobliny_karczmarz[] = {
	TALK(619),
	TALK(620),
	SET_QUEST_PROGRESS(10),
	TALK2(621),
	TALK(622),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_gobliny_szlachcic2[] = {
	TALK(623),
	TALK(624),
	TALK(625),
	SPECIAL("attack"),
	END,
	END_OF_DIALOG
};

void Quest_Gobliny::Start()
{
	// start_loc ustawianie w InitQuests
	type = 3;
	quest_id = Q_GOBLINY;
	enc = -1;
}

DialogEntry* Quest_Gobliny::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_gobliny_szlachcic") == 0)
		return dialog_gobliny_szlachcic;
	else if(strcmp(game->current_dialog->talker->data->id, "q_gobliny_mag") == 0)
		return dialog_gobliny_mag;
	else if(strcmp(game->current_dialog->talker->data->id, "innkeeper") == 0)
		return dialog_gobliny_karczmarz;
	else if(strcmp(game->current_dialog->talker->data->id, "q_gobliny_szlachcic2") == 0)
		return dialog_gobliny_szlachcic2;
	else
	{
		assert(strcmp(game->current_dialog->talker->data->id, "q_gobliny_poslaniec") == 0);
		return dialog_gobliny_poslaniec;
	}
}

bool CzyMajaStaryLuk()
{
	return Game::Get().HaveQuestItem(FindItem("q_gobliny_luk"));
}

void DodajStraznikow()
{
	Game& game = Game::Get();
	Unit* u = NULL;
	UnitData* ud = FindUnitData("q_gobliny_szlachcic2");

	// szukaj szlachcica
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Useable* use = NULL;

	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_TRON)
		{
			use = *it;
			break;
		}
	}
	assert(use);

	// przesuñ szlachcica w poblirze tronu
	game.WarpUnit(*u, use->pos);

	// usuñ pozosta³e osoby z pomieszczenia
	InsideLocation* inside = (InsideLocation*)game.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Pokoj* pokoj = lvl.GetNearestRoom(u->pos);
	assert(pokoj);
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data != ud && pokoj->IsInside((*it)->pos))
		{
			(*it)->to_remove = true;
			game.to_remove.push_back(*it);
		}
	}

	// dodaj ochronê
	UnitData* ud2 = FindUnitData("q_gobliny_ochroniarz");
	for(int i=0; i<3; ++i)
	{
		Unit* u2 = game.SpawnUnitInsideRoom(*pokoj, *ud2, 10);
		if(u2)
		{
			u2->dont_attack = true;
			u2->guard_target = u;
		}
	}
	
	// ustaw szlachcica
	u->hero->name = game.txQuest[215];
	u->hero->know_name = true;
	game.gobliny_hd.Set(*u->human_data);
}

void Quest_Gobliny::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// nie zaakceptowano
		{
			prog = 1;
			// dodaj plotkê
			if(!game->plotka_questowa[P_GOBLINY])
			{
				game->plotka_questowa[P_GOBLINY] = true;
				--game->ile_plotek_questowych;
				cstring text = Format(game->txQuest[211], game->locations[start_loc]->name.c_str());
				game->plotki.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->plotki.size())-1;
				}
			}
		}
		break;
	case 2:
		// zaakceptowano
		{
			prog = 2;
			state = Quest::Started;
			name = game->txQuest[212];
			start_time = game->worldtime;
			// usuñ plotkê
			if(!game->plotka_questowa[P_GOBLINY])
			{
				game->plotka_questowa[P_GOBLINY] = true;
				--game->ile_plotek_questowych;
			}
			// dodaj lokalizacje
			target_loc = game->GetNearestLocation2(GetStartLocation().pos, 1<<L_FOREST, true);
			Location& target = GetTargetLocation();
			if(target.state == LS_UNKNOWN)
				target.state = LS_KNOWN;
			target.reset = true;
			target.active_quest = this;
			target.st = 7;
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = FindItem("q_gobliny_luk");
			// questowe rzeczy
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[217], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[218], GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// encounter
			Encounter* e = game->AddEncounter(enc);
			e->check_func = CzyMajaStaryLuk;
			e->dialog = dialog_gobliny_atak;
			e->dont_attack = true;
			e->grupa = SG_GOBLINY;
			e->location_event_handler = NULL;
			e->pos = GetStartLocation().pos;
			e->quest = (Quest_Encounter*)this;
			e->szansa = 10000;
			e->zasieg = 32.f;
			e->text = game->txQuest[219];
			e->timed = false;

			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case 3:
		// gobliny ukrad³y ³uk
		{
			prog = 3;
			game->RemoveQuestItem(FindItem("q_gobliny_luk"));
			msgs.push_back(game->txQuest[220]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->RemoveEncounter(enc);
			enc = -1;
			GetTargetLocation().active_quest = NULL;
			game->AddNews(game->txQuest[221]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4:
		// poinformowano o kradzie¿y
		{
			prog = 4;
			state = Quest::Failed;
			msgs.push_back(game->txQuest[222]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->gobliny_stan = Game::GS_ODLICZANIE;
			game->gobliny_dni = random(15,30);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// pos³aniec dostarczy³ info o bazie goblinów
		{
			prog = 5;
			state = Quest::Started;
			target_loc = game->GetRandomSpawnLocation(GetStartLocation().pos, SG_GOBLINY);
			Location& target = GetTargetLocation();
			bool now_known = false;
			if(target.state == LS_UNKNOWN)
			{
				target.state = LS_KNOWN;
				now_known = true;
			}
			target.st = 11;
			target.reset = true;
			target.active_quest = this;
			done = false;
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = FindUnitData(GetSpawnLeader(SG_GOBLINY));
			unit_spawn_level = -3;
			item_to_give[0] = FindItem("q_gobliny_luk");
			at_level = target.GetLastLevel();
			msgs.push_back(Format(game->txQuest[223], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->gobliny_stan = Game::GS_POSLANIEC_POGADAL;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 6:
		// oddano ³uk
		{
			prog = 6;
			state = Quest::Completed;
			const Item* item = FindItem("q_gobliny_luk");
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->current_dialog->talker->AddItem(item, 1, true);
			game->AddReward(500);
			msgs.push_back(game->txQuest[224]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->gobliny_stan = Game::GS_ODDANO_LUK;
			GetTargetLocation().active_quest = NULL;
			target_loc = -1;
			game->AddNews(game->txQuest[225]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// nie chcia³eœ powiedzieæ o ³uku
		{
			prog = 7;
			msgs.push_back(game->txQuest[226]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->gobliny_stan = Game::GS_MAG_POGADAL_START;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 8:
		// powiedzia³eœ o ³uku
		{
			prog = 8;
			state = Quest::Started;
			msgs.push_back(game->txQuest[227]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->gobliny_stan = Game::GS_MAG_POGADAL;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 9:
		// zap³aci³eœ i powiedzia³eœ o ³uku
		{
			game->current_dialog->pc->unit->gold -= 100;

			prog = 9;
			state = Quest::Started;
			msgs.push_back(game->txQuest[228]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->gobliny_stan = Game::GS_MAG_POGADAL;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->GetPlayerInfo(game->current_dialog->pc).UpdateGold();
			}
		}
		break;
	case 10:
		// pogadano z karczmarzem
		{
			prog = 10;
			game->gobliny_stan = Game::GS_POZNANO_LOKACJE;
			target_loc = game->CreateLocation(L_DUNGEON, game->world_pos, 128.f, THRONE_FORT, SG_GOBLINY, false);
			Location& target = GetTargetLocation();
			target.st = 13;
			target.state = LS_KNOWN;
			target.active_quest = this;
			done = false;
			unit_to_spawn = FindUnitData("q_gobliny_szlachcic2");
			spawn_unit_room = POKOJ_CEL_TRON;
			callback = DodajStraznikow;
			unit_dont_attack = true;
			unit_auto_talk = true;
			unit_event_handler = this;
			item_to_give[0] = NULL;
			spawn_item = Quest_Event::Item_DontSpawn;
			at_level = target.GetLastLevel();
			msgs.push_back(Format(game->txQuest[229], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 11:
		// zabito szlachcica
		{
			prog = 11;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[230]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			GetTargetLocation().active_quest = NULL;
			game->EndUniqueQuest();
			game->AddNews(game->txQuest[231]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_Gobliny::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "start_loc")
		return GetStartLocationName();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Gobliny::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "gobliny") == 0;
}

void Quest_Gobliny::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::DIE && prog == 10)
	{
		SetProgress(11);
		unit->event_handler = NULL;
	}
}

void Quest_Gobliny::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &enc, sizeof(enc), &tmp, NULL);
}

void Quest_Gobliny::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &enc, sizeof(enc), &tmp, NULL);

	if(!done)
	{
		if(prog == 2)
		{
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = FindItem("q_gobliny_luk");
		}
		else if(prog == 5)
		{
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = FindUnitData(GetSpawnLeader(SG_GOBLINY));
			unit_spawn_level = -3;
			item_to_give[0] = FindItem("q_gobliny_luk");
		}
		else if(prog == 10)
		{
			unit_to_spawn = FindUnitData("q_gobliny_szlachcic2");
			spawn_unit_room = POKOJ_CEL_BRAK;
			callback = DodajStraznikow;
			unit_dont_attack = true;
			unit_auto_talk = true;
		}
	}

	unit_event_handler = this;

	if(enc != -1)
	{
		Encounter* e = game->RecreateEncounter(enc);
		e->check_func = CzyMajaStaryLuk;
		e->dialog = dialog_gobliny_atak;
		e->dont_attack = true;
		e->grupa = SG_GOBLINY;
		e->location_event_handler = NULL;
		e->pos = GetStartLocation().pos;
		e->quest = (Quest_Encounter*)this;
		e->szansa = 10000;
		e->zasieg = 32.f;
		e->text = game->txQuest[219];
		e->timed = false;
	}
}

//====================================================================================================================================================================================
DialogEntry dialog_zlo_kaplan[] = {
	IF_QUEST_EVENT,
		IF_SPECIAL("q_zlo_clear1"),
			TALK(626),
			SET_QUEST_PROGRESS(11),
			END,
		ELSE,
			IF_SPECIAL("q_zlo_clear2"),
				TALK(627),
				SET_QUEST_PROGRESS(11),
				END,
			ELSE,
				TALK(628),
				TALK2(629),
				SET_QUEST_PROGRESS(12),
				END,
			END_IF,
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(0),
		TALK(630),
		TALK(631),
		TALK(632),
		TALK(633),
		CHOICE(634),
			SET_QUEST_PROGRESS(2),
			RESTART,
		END_CHOICE,
		CHOICE(635),
			SET_QUEST_PROGRESS(1),
			TALK(636),
			END,
		END_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(1),
		TALK(637),
		CHOICE(638),
			SET_QUEST_PROGRESS(2),
			RESTART,
		END_CHOICE,
		CHOICE(639),
			TALK(640),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(2),
		TALK(641),
		SET_QUEST_PROGRESS(3),
		TALK2(642),
		TALK(643),
		SPECIAL("tell_name"),
		TALK(644),
		END,
	END_IF,
	IF_QUEST_PROGRESS(3),
		TALK(645),
		END,
	END_IF,
	IF_QUEST_PROGRESS(4),
		TALK(646),
		TALK(647),
		SET_QUEST_PROGRESS(5),
		TALK2(648),
		TALK(649),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(5,9),
		IF_HAVE_ITEM("q_zlo_ksiega"),
			TALK(650),
			SET_QUEST_PROGRESS(10),
			TALK2(651),
			TALK(652),
			TALK2(653),
			TALK2(654),
			TALK2(655),
			TALK(656),
			END,
		ELSE,
			TALK(657),
			END,
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(10),
		IF_SPECIAL("q_zlo_tutaj"),
			TALK(658),
		ELSE,
			TALK2(659),
		END_IF,
		END,
	END_IF,
	IF_QUEST_PROGRESS(12),
		IF_SPECIAL("q_zlo_tutaj"),
			TALK2(660),
		ELSE,
			TALK(661),
		END_IF,
		END,
	END_IF,
	IF_QUEST_PROGRESS(13),
		TALK(662),
		TALK(663),
		TALK(664),
		SET_QUEST_PROGRESS(14),
		END,
	END_IF,
	TALK(665),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_zlo_mag[] = {
	IF_QUEST_PROGRESS(5),
		TALK(666),
		CHOICE(667),
			TALK(668),
			TALK(669),
			TALK(670),
			TALK(671),
			SET_QUEST_PROGRESS(6),
			END,
		END_CHOICE,
		CHOICE(672),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	TALK(673),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_zlo_kapitan[] = {
	IF_QUEST_PROGRESS(6),
		TALK(674),
		TALK(675),
		TALK2(676),
		SET_QUEST_PROGRESS(7),
		END,
	END_IF,
	IF_QUEST_PROGRESS(7),
		TALK2(677),
		END,
	END_IF,
	IF_QUEST_PROGRESS(8),
		TALK(678),
		TALK(679),
		SET_QUEST_PROGRESS(9),
		END,
	END_IF,
	END_OF_DIALOG
};

DialogEntry dialog_zlo_burmistrz[] = {
	TALK(680),
	TALK(681),
	SET_QUEST_PROGRESS(8),
	END,
	END_OF_DIALOG
};

DialogEntry dialog_zlo_boss[] = {
	TALK(682),
	TALK(683),
	SPECIAL("attack"),
	END,
	END_OF_DIALOG
};

void Quest_Zlo::Start()
{
	type = 3;
	quest_id = Q_ZLO;
	// start_loc ustawiane w InitQuests
	mage_loc = -1;
	closed = 0;
	changed = false;
	for(int i=0; i<3; ++i)
	{
		loc[i].state = 0;
		loc[i].target_loc = -1;
	}
	told_about_boss = false;
}

DialogEntry* Quest_Zlo::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_zlo_kaplan") == 0)
		return dialog_zlo_kaplan;
	else if(strcmp(game->current_dialog->talker->data->id, "q_zlo_mag") == 0)
		return dialog_zlo_mag;
	else if(strcmp(game->current_dialog->talker->data->id, "q_zlo_boss") == 0)
		return dialog_zlo_boss;
	else if(strcmp(game->current_dialog->talker->data->id, "guard_captain") == 0)
		return dialog_zlo_kapitan;
	else if(strcmp(game->current_dialog->talker->data->id, "mayor") == 0 || strcmp(game->current_dialog->talker->data->id, "soltys") == 0)
		return dialog_zlo_burmistrz;
	else
	{
		assert(0);
		return NULL;
	}
}

void GenerujKrwawyOltarz()
{
	Game& game = Game::Get();
	InsideLocation* inside = (InsideLocation*)game.location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	// szukaj zwyk³ego o³tarza blisko œrodka
	VEC3 center(float(lvl.w+1),0,float(lvl.h+1));
	int best_index=-1, index=0;
	float best_dist=999.f;
	Obj* oltarz = FindObject("altar");
	for(vector<Object>::iterator it = lvl.objects.begin(), end = lvl.objects.end(); it != end; ++it, ++index)
	{
		if(it->base == oltarz)
		{
			float dist = distance(it->pos, center);
			if(dist < best_dist)
			{
				best_dist = dist;
				best_index = index;
			}
		}
	}
	assert(best_index != -1);

	// zmieñ typ obiektu
	Object& o = lvl.objects[best_index];
	o.base = FindObject("bloody_altar");
	o.mesh = o.base->ani;

	// dodaj cz¹steczki
	ParticleEmitter* pe = new ParticleEmitter;
	pe->alpha = 0.8f;
	pe->emision_interval = 0.1f;
	pe->emisions = -1;
	pe->life = -1;
	pe->max_particles = 50;
	pe->op_alpha = POP_LINEAR_SHRINK;
	pe->op_size = POP_LINEAR_SHRINK;
	pe->particle_life = 0.5f;
	pe->pos = o.pos;
	pe->pos.y += o.base->centery;
	pe->pos_min = VEC3(0,0,0);
	pe->pos_max = VEC3(0,0,0);
	pe->spawn_min = 1;
	pe->spawn_max = 3;
	pe->speed_min = VEC3(-1,4,-1);
	pe->speed_max = VEC3(1,6,1);
	pe->mode = 0;
	pe->tex = game.tKrew[BLOOD_RED];
	pe->size = 0.5f;
	pe->Init();
	game.local_ctx.pes->push_back(pe);

	// dodaj krew
	vector<INT2> path;
	game.FindPath(game.local_ctx, lvl.schody_gora, pos_to_pt(o.pos), path);
	for(vector<INT2>::iterator it = path.begin(), end = path.end(); it != end; ++it)
	{
		if(it != path.begin())
		{
			Blood& b = Add1(game.local_ctx.bloods);
			b.pos = random(VEC3(-0.5f,0.f,-0.5f), VEC3(0.5f,0,0.5f))+VEC3(2.f*it->x+1+(float(it->x)-(it-1)->x)/2,0,2.f*it->y+1+(float(it->y)-(it-1)->y)/2);
			b.type = BLOOD_RED;
			b.rot = random(MAX_ANGLE);
			b.size = 1.f;
			b.pos.y = 0.05f;
			b.normal = VEC3(0,1,0);
		}
		{
			Blood& b = Add1(game.local_ctx.bloods);
			b.pos = random(VEC3(-0.5f,0.f,-0.5f), VEC3(0.5f,0,0.5f))+VEC3(2.f*it->x+1,0,2.f*it->y+1);
			b.type = BLOOD_RED;
			b.rot = random(MAX_ANGLE);
			b.size = 1.f;
			b.pos.y = 0.05f;
			b.normal = VEC3(0,1,0);
		}
	}

	// ustaw pokój na specjalny ¿eby nie by³o tam wrogów
	lvl.GetNearestRoom(o.pos)->cel = POKOJ_CEL_SKARBIEC;
	
	game.zlo_stan = Game::ZS_OLTARZ_STWORZONY;
	game.zlo_pos = o.pos;

	DEBUG_LOG(Format("Generated bloody altar (%g,%g).", o.pos.x, o.pos.z));
}

bool SortujDobrePokoje(const std::pair<int, float>& p1, const std::pair<int, float>& p2)
{
	return p1.second > p2.second;
}

void GenerujPortal()
{
	Game& game = Game::Get();
	InsideLocation* inside = (InsideLocation*)game.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	VEC3 srodek(float(lvl.w),0,float(lvl.h));

	// szukaj pokoju
	static vector<std::pair<int, float> > dobre;
	int index = 0;
	for(vector<Pokoj>::iterator it = lvl.pokoje.begin(), end = lvl.pokoje.end(); it != end; ++it, ++index)
	{
		if(!it->korytarz && it->cel == POKOJ_CEL_BRAK && it->size.x > 2 && it->size.y > 2)
		{
			float dist = distance2d(it->Srodek(), srodek);
			dobre.push_back(std::pair<int, float>(index, dist));
		}
	}
	std::sort(dobre.begin(), dobre.end(), SortujDobrePokoje);

	int id;

	while(true)
	{
		id = dobre.back().first;
		dobre.pop_back();
		Pokoj& p = lvl.pokoje[id];

		game.global_col.clear();
		game.GatherCollisionObjects(game.local_ctx, game.global_col, p.Srodek(), 2.f);
		if(game.global_col.empty())
			break;

		if(dobre.empty())
			throw "No free space to generate portal!";
	}

	dobre.clear();

	Pokoj& p = lvl.pokoje[id];
	VEC3 pos = p.Srodek();
	p.cel = POKOJ_CEL_PORTAL_STWORZ;
	float rot = PI*random(0,3);
	game.SpawnObject(game.local_ctx, FindObject("portal"), pos, rot);
	inside->portal = new Portal;
	inside->portal->target_loc = -1;
	inside->portal->next_portal = NULL;
	inside->portal->rot = rot;
	inside->portal->pos = pos;
	inside->portal->at_level = game.dungeon_level;

	Quest_Zlo* q = (Quest_Zlo*)game.FindQuest(game.zlo_refid);
	int d = q->GetLocId(game.current_location);
	q->loc[d].pos = pos;
	q->loc[d].state = 0;

	DEBUG_LOG(Format("Generated portal (%g,%g).", pos.x, pos.z));
}

void WarpEvilBossToAltar()
{
	// znajdŸ bossa
	Game& game = Game::Get();
	Unit* u = game.FindUnitByIdLocal("q_zlo_boss");
	assert(u);

	// znajdŸ krwawy o³tarz
	Object* o = game.FindObjectByIdLocal("bloody_altar");
	assert(o);

	if(u && o)
	{
		VEC3 pos = o->pos;
		pos -= VEC3(sin(o->rot.y)*1.5f, 0, cos(o->rot.y)*1.5f);
		game.WarpUnit(*u, pos);
		u->ai->start_pos = u->pos;
	}
}

void Quest_Zlo::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1:
		// nie zaakceptowano
		{
			prog = 1;
			// dodaj plotkê
			if(!game->plotka_questowa[P_ZLO])
			{
				game->plotka_questowa[P_ZLO] = true;
				--game->ile_plotek_questowych;
				cstring text = Format(game->txQuest[232], GetStartLocationName());
				game->plotki.push_back(Format(game->game_gui->journal->txAddNote, game->day+1, game->month+1, game->year, text));
				game->game_gui->journal->NeedUpdate(Journal::Rumors);
				game->AddGameMsg3(GMS_ADDED_RUMOR);
				if(game->IsOnline())
				{
					NetChange& c = Add1(game->net_changes);
					c.type = NetChange::ADD_RUMOR;
					c.id = int(game->plotki.size())-1;
				}
			}
		}
		break;
	case 2:
		// zaakceptowano
		prog = 2;
		break;
	case 3:
		// zaakceptowano
		{
			name = game->txQuest[233];
			prog = 3;
			state = Quest::Started;
			// usuñ plotkê
			if(!game->plotka_questowa[P_ZLO])
			{
				game->plotka_questowa[P_ZLO] = true;
				--game->ile_plotek_questowych;
			}
			// lokacja
			target_loc = game->CreateLocation(L_DUNGEON, game->world_pos, 128.f, OLD_TEMPLE, SG_BRAK, false, 1);
			game->zlo_gdzie = target_loc;
			Location& target = GetTargetLocation();
			bool now_known = false;
			if(target.state == LS_UNKNOWN)
			{
				target.state = LS_KNOWN;
				now_known = true;
			}
			target.st = 8;
			target.active_quest = this;
			target.dont_clean = true;
			callback = GenerujKrwawyOltarz;
			// questowe rzeczy
			msgs.push_back(Format(game->txQuest[234], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[235], GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 4:
		// zdarzenie
		{
			prog = 4;
			msgs.push_back(game->txQuest[236]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[237], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 5:
		// powiedzia³ o ksiêdze
		{
			prog = 5;
			mage_loc = game->GetRandomCityLocation(start_loc);
			Location& mage = *game->locations[mage_loc];
			msgs.push_back(Format(game->txQuest[238], mage.name.c_str(), GetLocationDirName(GetStartLocation().pos, mage.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->zlo_stan = Game::ZS_GENERUJ_MAGA;
			game->zlo_gdzie2 = mage_loc;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 6:
		// mag powiedzia³ ¿e mu zabrali ksiêge
		{
			prog = 6;
			msgs.push_back(game->txQuest[239]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(Format(game->txQuest[240], game->locations[mage_loc]->name.c_str()));
			game->current_dialog->talker->temporary = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 7:
		// pogadano z kapitanem
		{
			prog = 7;
			msgs.push_back(Format(game->txQuest[241], game->location->type == L_CITY ? game->txForMayor : game->txForSoltys));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 8:
		// pogadano z burmistrzem
		{
			prog = 8;
			msgs.push_back(Format(game->txQuest[242], game->location->type == L_CITY ? game->txForMayor : game->txForSoltys));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 9:
		// dosta³eœ ksi¹¿ke
		{
			prog = 9;
			msgs.push_back(game->txQuest[243]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			const Item* item = FindItem("q_zlo_ksiega");
			game->current_dialog->pc->unit->AddItem(item, 1, true);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case 10:
		// da³eœ ksi¹¿kê jozanowi
		{
			// dodaj lokacje
			const struct
			{
				LOCATION type;
				int target;
				SPAWN_GROUP spawn;
				int st;
			} l_info[3] = {
				L_DUNGEON, OLD_TEMPLE, SG_ZLO, 15,
				L_DUNGEON, NECROMANCER_BASE, SG_NEKRO, 14,
				L_CRYPT, MONSTER_CRYPT, SG_NIEUMARLI, 13
			};

			msgs.push_back(game->txQuest[245]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			prog = 10;
			for(int i=0; i<3; ++i)
			{
				INT2 levels = g_base_locations[l_info[i].target].levels;
				loc[i].target_loc = game->CreateLocation(l_info[i].type, VEC2(0,0), -128.f, l_info[i].target, l_info[i].spawn, true, random2(max(levels.x, 2), max(levels.y, 2)));
				Location& target = *game->locations[loc[i].target_loc];
				target.st = l_info[i].st;
				target.state = LS_KNOWN;
				target.active_quest = this;
				loc[i].near_loc = game->GetNearestCity(target.pos);
				loc[i].at_level = target.GetLastLevel();
				loc[i].callback = GenerujPortal;
				msgs.push_back(Format(game->txQuest[247], game->locations[loc[i].target_loc]->name.c_str(),
					GetLocationDirName(game->locations[loc[i].near_loc]->pos, game->locations[loc[i].target_loc]->pos),
					game->locations[loc[i].near_loc]->name.c_str()));
				game->AddNews(Format(game->txQuest[246],
					game->locations[loc[i].target_loc]->name.c_str()));
			}

			next_event = &loc[0];
			loc[0].next_event = &loc[1];
			loc[1].next_event = &loc[2];

			// dodaj jozana do dru¿yny
			Unit& u = *game->current_dialog->talker;
			const Item* item = FindItem("q_zlo_ksiega");
			u.AddItem(item, 1, true);
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			u.hero->team_member = true;
			u.hero->free = true;
			u.hero->mode = HeroData::Follow;
			game->AddTeamMember(&u, false);

			game->zlo_stan = Game::ZS_ZAMYKANIE_PORTALI;

			if(game->IsOnline())
			{
				game->Net_UpdateQuestMulti(refid, 4);
				for(int i=0; i<3; ++i)
					game->Net_ChangeLocationState(loc[i].target_loc, false);
				game->Net_RecruitNpc(&u);
			}
		}
		break;
	case 11:
		// u¿ywane tylko do czyszczenia flagi changed
		// w mp wysy³a te¿ t¹ aktualizacje z Game::UpdateGame2
		changed = false;
		if(game->IsOnline())
			game->Net_UpdateQuest(refid);
		break;
	case 12:
		// zamkniêto wszystkie portale
		{
			changed = false;
			prog = 12;
			done = false;
			unit_to_spawn = FindUnitData("q_zlo_boss");
			spawn_unit_room = POKOJ_CEL_SKARBIEC;
			unit_event_handler = this;
			unit_dont_attack = true;
			unit_auto_talk = true;
			callback = WarpEvilBossToAltar;
			Location& target = *game->locations[target_loc];
			target.st = 15;
			target.spawn = SG_ZLO;
			target.reset = true;
			game->zlo_stan = Game::ZS_ZABIJ_BOSSA;
			msgs.push_back(Format(game->txQuest[248], GetTargetLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			for(int i=0; i<3; ++i)
				game->locations[loc[i].target_loc]->active_quest = NULL;

			if(game->IsOnline())
				game->Net_UpdateQuestMulti(refid, 2);
		}
		break;
	case 13:
		// zabito bossa
		{
			prog = 13;
			state = Quest::Completed;
			msgs.push_back(game->txQuest[249]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// przywróæ stary o³tarz
			Location& loc = GetTargetLocation();
			loc.active_quest = NULL;
			loc.dont_clean = false;
			Obj* o = FindObject("bloody_altar");
			int index = 0;
			for(vector<Object>::iterator it = game->local_ctx.objects->begin(), end = game->local_ctx.objects->end(); it != end; ++it, ++index)
			{
				if(it->base == o)
					break;
			}
			Object& obj = game->local_ctx.objects->at(index);
			obj.base = FindObject("altar");
			obj.mesh = obj.base->ani;
			// usuñ cz¹steczki
			float best_dist = 999.f;
			ParticleEmitter* pe = NULL;
			for(vector<ParticleEmitter*>::iterator it = game->local_ctx.pes->begin(), end = game->local_ctx.pes->end(); it != end; ++it)
			{
				if((*it)->tex == game->tKrew[BLOOD_RED])
				{
					float dist = distance((*it)->pos, obj.pos);
					if(dist < best_dist)
					{
						best_dist = dist;
						pe = *it;
					}
				}
			}
			assert(pe);
			pe->destroy = true;
			// gadanie przez jozana
			UnitData* ud = FindUnitData("q_zlo_kaplan");
			for(vector<Unit*>::iterator it = game->team.begin(), end = game->team.end(); it != end; ++it)
			{
				if((*it)->data == ud)
				{
					(*it)->auto_talk = 1;
					break;
				}
			}

			game->EndUniqueQuest();
			game->zlo_stan = Game::ZS_JOZAN_CHCE_GADAC;
			game->AddNews(game->txQuest[250]);

			if(game->IsOnline())
			{
				game->PushNetChange(NetChange::CLEAN_ALTAR);
				game->Net_UpdateQuest(refid);
			}
		}
		break;
	case 14:
		// pogadano z jozanem
		{
			prog = 14;
			game->zlo_stan = Game::ZS_JOZAN_IDZIE;
			// usuñ jozana z dru¿yny
			Unit& u = *game->current_dialog->talker;
			u.hero->team_member = false;
			u.hero->mode = HeroData::Leave;
			u.MakeItemsTeam(true);
			RemoveElementOrder(game->team, &u);

			if(game->IsOnline())
				game->Net_KickNpc(&u);
		}
		break;
	}
}

cstring Quest_Zlo::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "mage_loc")
		return game->locations[mage_loc]->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[mage_loc]->pos);
	else if(str == "burmistrza")
		return game->location->type == L_CITY ? game->txForMayor : game->txForSoltys;
	else if(str == "burmistrzem")
		return game->location->type == L_CITY ? game->txQuest[251] : game->txQuest[252];
	else if(str == "t1")
		return game->locations[loc[0].target_loc]->name.c_str();
	else if(str == "t2")
		return game->locations[loc[1].target_loc]->name.c_str();
	else if(str == "t3")
		return game->locations[loc[2].target_loc]->name.c_str();
	else if(str == "t1n")
		return game->locations[loc[0].near_loc]->name.c_str();
	else if(str == "t2n")
		return game->locations[loc[1].near_loc]->name.c_str();
	else if(str == "t3n")
		return game->locations[loc[2].near_loc]->name.c_str();
	else if(str == "t1d")
		return GetLocationDirName(game->locations[loc[0].near_loc]->pos, game->locations[loc[0].target_loc]->pos);
	else if(str == "t2d")
		return GetLocationDirName(game->locations[loc[1].near_loc]->pos, game->locations[loc[1].target_loc]->pos);
	else if(str == "t3d")
		return GetLocationDirName(game->locations[loc[2].near_loc]->pos, game->locations[loc[2].target_loc]->pos);
	else if(str == "close_dir")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i=0; i<3; ++i)
		{
			if(loc[i].state != 3)
			{
				float dist = distance(game->world_pos, game->locations[loc[i].target_loc]->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = i;
				}
			}
		}
		Loc& l = loc[best_index];
		return GetLocationDirName(game->world_pos, game->locations[l.target_loc]->pos);
	}
	else if(str == "close_loc")
	{
		float best_dist = 999.f;
		int best_index = -1;
		for(int i=0; i<3; ++i)
		{
			if(loc[i].state != 3)
			{
				float dist = distance(game->world_pos, game->locations[loc[i].target_loc]->pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					best_index = loc[i].target_loc;
				}
			}
		}
		return game->locations[best_index]->name.c_str();
	}
	else if(str == "start_loc")
		return GetStartLocationName();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Zlo::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "zlo") == 0;
}

bool Quest_Zlo::IfQuestEvent()
{
	return changed;
}

void Quest_Zlo::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
{
	assert(unit);
	if(type == UnitEventHandler::DIE && prog == 12)
	{
		SetProgress(13);
		unit->event_handler = NULL;
	}
}

void Quest_Zlo::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &mage_loc, sizeof(mage_loc), &tmp, NULL);
	for(int i=0; i<3; ++i)
	{
		WriteFile(file, &loc[i].target_loc, sizeof(loc[i].target_loc), &tmp, NULL);
		WriteFile(file, &loc[i].done, sizeof(loc[i].done), &tmp, NULL);
		WriteFile(file, &loc[i].at_level, sizeof(loc[i].at_level), &tmp, NULL);
		WriteFile(file, &loc[i].near_loc, sizeof(loc[i].near_loc), &tmp, NULL);
		WriteFile(file, &loc[i].state, sizeof(loc[i].state), &tmp, NULL);
		WriteFile(file, &loc[i].pos, sizeof(loc[i].pos), &tmp, NULL);
	}
	WriteFile(file, &closed, sizeof(closed), &tmp, NULL);
	WriteFile(file, &changed, sizeof(changed), &tmp, NULL);
	WriteFile(file, &told_about_boss, sizeof(told_about_boss), &tmp, NULL);
}

void Quest_Zlo::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &mage_loc, sizeof(mage_loc), &tmp, NULL);
	for(int i=0; i<3; ++i)
	{
		ReadFile(file, &loc[i].target_loc, sizeof(loc[i].target_loc), &tmp, NULL);
		ReadFile(file, &loc[i].done, sizeof(loc[i].done), &tmp, NULL);
		ReadFile(file, &loc[i].at_level, sizeof(loc[i].at_level), &tmp, NULL);
		ReadFile(file, &loc[i].near_loc, sizeof(loc[i].near_loc), &tmp, NULL);
		if(LOAD_VERSION == V_0_2)
		{
			bool cleared;
			ReadFile(file, &cleared, sizeof(cleared), &tmp, NULL);
			loc[i].state = (cleared ? 3 : 0);
		}
		else
			ReadFile(file, &loc[i].state, sizeof(loc[i].state), &tmp, NULL);
		ReadFile(file, &loc[i].pos, sizeof(loc[i].pos), &tmp, NULL);
		loc[i].callback = GenerujPortal;
	}
	ReadFile(file, &closed, sizeof(closed), &tmp, NULL);
	ReadFile(file, &changed, sizeof(changed), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		told_about_boss = false;
	else
		ReadFile(file, &told_about_boss, sizeof(told_about_boss), &tmp, NULL);

	next_event = &loc[0];
	loc[0].next_event = &loc[1];
	loc[1].next_event = &loc[2];

	if(!done)
	{
		if(prog == 3)
			callback = GenerujKrwawyOltarz;
		else if(prog == 12)
		{
			unit_to_spawn = FindUnitData("q_zlo_boss");
			spawn_unit_room = POKOJ_CEL_SKARBIEC;
			unit_dont_attack = true;
			unit_auto_talk = true;
			callback = WarpEvilBossToAltar;
		}
	}

	unit_event_handler = this;
}

//====================================================================================================================================================================================
DialogEntry dialog_szaleni_trener[] = {
	TALK(684),
	TALK(685),
	TALK(686),
	SET_QUEST_PROGRESS(1),
	TALK2(687),
	TALK2(688),
	END,
	END_OF_DIALOG
};

void Quest_Szaleni::Start()
{
	type = 3;
	quest_id = Q_SZALENI;
	target_loc = -1;
	name = game->txQuest[253];
}

DialogEntry* Quest_Szaleni::GetDialog(int type2)
{
	return dialog_szaleni_trener;
}

void Quest_Szaleni::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 0: // zaatakowano przez unk
		{
			prog = 0;
			state = Quest::Started;

			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			msgs.push_back(game->txQuest[254]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			if(game->IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case 1: // trener powiedzia³ o labiryncie
		{
			prog = 1;
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -128.f, LABIRYNTH, SG_UNK, false);
			start_loc = game->current_location;
			Location& loc = GetTargetLocation();
			loc.active_quest = this;
			loc.state = LS_KNOWN;
			loc.st = 13;

			game->szaleni_stan = Game::SS_POGADANO_Z_TRENEREM;

			msgs.push_back(Format(game->txQuest[255], game->location->name.c_str(), loc.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case 2: // schowano kamieñ do skrzyni
		{
			prog = 2;
			state = Quest::Completed;
			GetTargetLocation().active_quest = NULL;

			game->szaleni_stan = Game::SS_KONIEC;

			msgs.push_back(game->txQuest[256]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
	}
}

cstring Quest_Szaleni::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return NULL;
	}
}

bool Quest_Szaleni::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "szaleni") == 0;
}

//====================================================================================================================================================================================
DialogEntry list_gonczy_start[] = {
	TALK(689),
	TALK2(690),
	TALK(691),
	CHOICE(692),
		SET_QUEST_PROGRESS(1),
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

DialogEntry list_gonczy_czas_minal[] = {
	SET_QUEST_PROGRESS(2),
	TALK2(697),
	TALK(698),
	END,
	END_OF_DIALOG
};

DialogEntry list_gonczy_koniec[] = {
	TALK2(699),
	SET_QUEST_PROGRESS(4),
	TALK2(700),
	END,
	END_OF_DIALOG
};

void Quest_ListGonczy::Start()
{
	start_loc = game->current_location;
	quest_id = Q_LIST_GONCZY;
	type = 1;
	level = random(5, 15);
	crazy = (rand2()%5 == 0);
	clas = ClassInfo::GetRandomEvil();
	target_unit = NULL;
}

DialogEntry* Quest_ListGonczy::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return list_gonczy_start;
	case QUEST_DIALOG_FAIL:
		return list_gonczy_czas_minal;
	case QUEST_DIALOG_NEXT:
		return list_gonczy_koniec;
	default:
		assert(0);
		return NULL;
	}
}

void Quest_ListGonczy::SetProgress(int prog2)
{
	switch(prog2)
	{
	case 1: // zaakceptowano
		{
			prog = 1;

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
			letter.id = "$wanted_letter";
			letter.level = 0;
			letter.mesh = NULL;
			letter.name = game->txQuest[258];
			letter.refid = refid;
			letter.tex = game->tListGonczy;
			letter.type = IT_OTHER;
			letter.value = 0;
			letter.weight = 1;
			letter.desc = Format(game->txQuest[259], level*100, unit_name.c_str());
			letter.other_type = OtherItems;
			game->current_dialog->pc->unit->AddItem(&letter, 1, true);

			game->quests.push_back(this);
			game->quests_timeout.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			// wpis do dziennika
			msgs.push_back(Format(game->txQuest[29], GetStartLocationName(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[260], level*100, unit_name.c_str(), GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->quests.size()-1);
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
	case 2: // czas min¹³
		{
			prog = 2;
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_dowodca = 2;

			Location& target = GetTargetLocation();
			if(target.active_quest == this)
				target.active_quest = NULL;

			msgs.push_back(Format(game->txQuest[261], unit_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);

			done = false;
		}
		break;
	case 3: // zabito
		{
			prog = 3;

			msgs.push_back(Format(game->txQuest[262], unit_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case 4: // wykonano
		{
			prog = 4;
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_dowodca = 0;

			game->AddReward(level*100);

			msgs.push_back(Format(game->txQuest[263], unit_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, game->GetQuestIndex(this));
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			RemoveElementTry<Quest_Dungeon*>(game->quests_timeout, this);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

cstring Quest_ListGonczy::FormatString(const string& str)
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

bool Quest_ListGonczy::IsTimedout()
{
	return game->worldtime - start_time > 30;
}

void Quest_ListGonczy::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
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
		SetProgress(3);
		game->RemoveTimedUnit(target_unit);
		target_unit = NULL;
	}
}

void Quest_ListGonczy::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &level, sizeof(level), &tmp, NULL);
	WriteFile(file, &crazy, sizeof(crazy), &tmp, NULL);
	WriteFile(file, &clas, sizeof(clas), &tmp, NULL);
	WriteString1(file, unit_name);
	int unit_refid = (target_unit ? target_unit->refid : -1);
	WriteFile(file, &unit_refid, sizeof(unit_refid), &tmp, NULL);
}

void Quest_ListGonczy::Load(HANDLE file)
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
	letter.id = "$wanted_letter";
	letter.level = 0;
	letter.mesh = NULL;
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

bool Quest_ListGonczy::IfHaveQuestItem()
{
	return game->current_dialog->talker == target_unit;
}

bool Quest_ListGonczy::IfNeedTalk(cstring topic)
{
	return prog == 3 && strcmp(topic, "wanted") == 0;
}
