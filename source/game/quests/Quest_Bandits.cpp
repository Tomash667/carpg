#include "Pch.h"
#include "Base.h"
#include "Quest_Bandits.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"

//=================================================================================================
void Quest_Bandits::Start()
{
	quest_id = Q_BANDITS;
	type = Type::Unique;
	enc = -1;
	other_loc = -1;
	camp_loc = -1;
	target_loc = -1;
	get_letter = false;
	bandits_state = State::None;
	timer = 0.f;
	agent = nullptr;
}

//=================================================================================================
GameDialog* Quest_Bandits::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = game->current_dialog->talker->data->id;
	cstring dialog_id;

	if(id == "mistrz_agentow")
		dialog_id = "q_bandits_master";
	else if(id == "guard_captain")
		dialog_id = "q_bandits_captain";
	else if(id == "guard_q_bandyci")
		dialog_id = "q_bandits_guard";
	else if(id == "agent")
		dialog_id = "q_bandits_agent";
	else if(id == "q_bandyci_szef")
		dialog_id = "q_bandits_boss";
	else
		dialog_id = "q_bandits_encounter";

	return FindDialog(dialog_id);
}

//=================================================================================================
void WarpToThroneBanditBoss()
{
	Game& game = Game::Get();

	// search for boss
	UnitData* ud = FindUnitData("q_bandyci_szef");
	Unit* u = nullptr;
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// search for boss
	Useable* use = nullptr;
	for(vector<Useable*>::iterator it = game.local_ctx.useables->begin(), end = game.local_ctx.useables->end(); it != end; ++it)
	{
		if((*it)->type == U_THRONE)
		{
			use = *it;
			break;
		}
	}
	assert(use);

	// warp boss to throne
	game.WarpUnit(*u, use->pos);
}

//=================================================================================================
void Quest_Bandits::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::NotAccepted:
	case Progress::Started:
		break;
	case Progress::Talked:
		if(prog == Progress::FoundBandits)
		{
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
			e->dialog = FindDialog("q_bandits");
			e->dont_attack = true;
			e->grupa = SG_BANDYCI;
			e->location_event_handler = nullptr;
			e->pos = (sl.pos+other.pos)/2;
			e->quest = (Quest_Encounter*)this;
			e->szansa = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->zasieg = 72;
			msgs.push_back(game->txQuest[152]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		else
		{
			name = game->txQuest[153];
			const Item* item = FindItem("q_bandyci_paczka");
			game->current_dialog->pc->unit->AddItem(item, 1, true);
			other_loc = game->GetRandomCityLocation(start_loc);
			Location& sl = GetStartLocation();
			Location& other = *game->locations[other_loc];
			Encounter* e = game->AddEncounter(enc);
			e->dialog = FindDialog("q_bandits");
			e->dont_attack = true;
			e->grupa = SG_BANDYCI;
			e->location_event_handler = nullptr;
			e->pos = (sl.pos+other.pos)/2;
			e->quest = (Quest_Encounter*)this;
			e->szansa = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->zasieg = 72;
			QM.AcceptQuest(this);
			msgs.push_back(Format(game->txQuest[154], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[155], sl.name.c_str(), other.name.c_str(), GetLocationDirName(sl.pos, other.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(!game->quest_rumor[P_BANDYCI])
			{
				game->quest_rumor[P_BANDYCI] = true;
				--game->quest_rumor_counter;
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
	case Progress::FoundBandits:
		// podczas rozmowy z bandytami, 66% szansy na znalezienie przy nich listu za 1 razem
		{
			if(get_letter || rand2()%3 != 0)
				game->current_dialog->talker->AddItem(FindItem("q_bandyci_list"), 1, true);
			get_letter = true;
			msgs.push_back(game->txQuest[157]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->RemoveEncounter(enc);
			enc = -1;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkAboutLetter:
		break;
	case Progress::NeedTalkWithCaptain:
		// info o obozie
		{
			camp_loc = game->CreateCamp(GetStartLocation().pos, SG_BANDYCI);
			Location& camp = *game->locations[camp_loc];
			camp.st = 10;
			camp.state = LS_HIDDEN;
			camp.active_quest = this;
			msgs.push_back(game->txQuest[158]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			target_loc = camp_loc;
			location_event_handler = this;
			game->RemoveItem(*game->current_dialog->pc->unit, FindItem("q_bandyci_list"), 1);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::NeedClearCamp:
		// pozycja obozu
		{
			Location& camp = *game->locations[camp_loc];
			msgs.push_back(Format(game->txQuest[159], GetLocationDirName(GetStartLocation().pos, camp.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			camp.state = LS_KNOWN;
			bandits_state = State::GenerateGuards;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(camp_loc, false);
			}
		}
		break;
	case Progress::KilledBandits:
		{
			bandits_state = State::Counting;
			timer = 7.5f;

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

			// news, remove auto created news
			delete game->news.back();
			game->news.pop_back();
			game->AddNews(Format(game->txQuest[160], GetStartLocationName()));
		}
		break;
	case Progress::TalkedWithAgent:
		// porazmawiano z agentem, powiedzia³ gdzie jest skrytka i idzie sobie
		{
			bandits_state = State::AgentTalked;
			game->current_dialog->talker->hero->mode = HeroData::Leave;
			game->current_dialog->talker->event_handler = this;
			target_loc = game->CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, THRONE_VAULT, SG_BANDYCI, false);
			Location& target = *game->locations[target_loc];
			target.active_quest = this;
			target.state = LS_KNOWN;
			target.st = 10;
			game->locations[camp_loc]->active_quest = nullptr;
			msgs.push_back(Format(game->txQuest[161], target.name.c_str(), GetLocationDirName(GetStartLocation().pos, target.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			done = false;
			at_level = 0;
			unit_to_spawn = FindUnitData("q_bandyci_szef");
			spawn_unit_room = RoomTarget::Throne;
			unit_dont_attack = true;
			location_event_handler = nullptr;
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
	case Progress::KilledBoss:
		// zabito szefa
		{
			camp_loc = -1;
			msgs.push_back(Format(game->txQuest[162], GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[163]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// ukoñczono
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[164]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// ustaw arto na temporary ¿eby sobie poszed³
			game->current_dialog->talker->temporary = true;
			game->AddReward(5000);
			QM.EndUniqueQuest();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}

	prog = prog2;
}

//=================================================================================================
bool Quest_Bandits::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "bandyci") == 0;
}

//=================================================================================================
void Quest_Bandits::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "bandyci_daj_paczke") == 0)
	{
		const Item* item = FindItem("q_bandyci_paczka");
		ctx.talker->AddItem(item, 1, true);
		game->RemoveQuestItem(item);
	}
	else
	{
		assert(0);
	}
}

//=================================================================================================
cstring Quest_Bandits::FormatString(const string& str)
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
		return nullptr;
	}
}

//=================================================================================================
void Quest_Bandits::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::NeedClearCamp && event == LocationEventHandler::CLEARED)
		SetProgress(Progress::KilledBandits);
}

//=================================================================================================
void Quest_Bandits::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::LEAVE)
	{
		if(unit == agent)
		{
			agent = nullptr;
			bandits_state = State::AgentLeft;
		}
	}
	else if(event == UnitEventHandler::DIE)
	{
		if(prog == Progress::TalkedWithAgent)
		{
			SetProgress(Progress::KilledBoss);
			unit->event_handler = nullptr;
		}
	}
}

//=================================================================================================
void Quest_Bandits::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << enc;
	f << other_loc;
	f << camp_loc;
	f << get_letter;
	f << bandits_state;
	f << timer;
	f << agent;
}

//=================================================================================================
void Quest_Bandits::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

	f >> enc;
	f >> other_loc;
	f >> camp_loc;
	f >> get_letter;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> bandits_state;
		f >> timer;
		f >> agent;
	}

	if(enc != -1)
	{
		Encounter* e = game->RecreateEncounter(enc);
		e->dialog = FindDialog("q_bandits");
		e->dont_attack = true;
		e->grupa = SG_BANDYCI;
		e->location_event_handler = nullptr;
		e->pos = (GetStartLocation().pos+game->locations[other_loc]->pos)/2;
		e->quest = (Quest_Encounter*)this;
		e->szansa = 60;
		e->text = game->txQuest[11];
		e->timed = false;
		e->zasieg = 72;
	}

	if(prog == Progress::NeedTalkWithCaptain || prog == Progress::NeedClearCamp)
		location_event_handler = this;
	else
		location_event_handler = nullptr;

	if(prog == Progress::TalkedWithAgent && !done)
	{
		unit_to_spawn = FindUnitData("q_bandyci_szef");
		spawn_unit_room = RoomTarget::Throne;
		unit_dont_attack = true;
		location_event_handler = nullptr;
		unit_event_handler = this;
		unit_auto_talk = true;
		callback = WarpToThroneBanditBoss;
	}
}

//=================================================================================================
void Quest_Bandits::LoadOld(HANDLE file)
{
	GameReader f(file);

	int refid, city, where;

	f >> refid;
	f >> city;
	f >> where;
	f >> bandits_state;
	f >> timer;
	f >> agent;
}
