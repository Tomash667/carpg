#include "Pch.h"
#include "GameCore.h"
#include "Quest_Bandits.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "AIController.h"
#include "World.h"

//=================================================================================================
void Quest_Bandits::Init()
{
	QM.RegisterSpecialIfHandler(this, "q_bandyci_straznikow_daj");
}

//=================================================================================================
void Quest_Bandits::Start()
{
	quest_id = Q_BANDITS;
	type = QuestType::Unique;
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
	UnitData* ud = UnitData::Get("q_bandyci_szef");
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
	Usable* use = game.local_ctx.FindUsable("throne");
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
			const Item* item = Item::Get("q_bandyci_paczka");
			game->PreloadItem(item);
			if(!game->current_dialog->pc->unit->HaveItem(item))
			{
				game->current_dialog->pc->unit->AddItem(item, 1, true);
				if(Net::IsOnline() && !game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			Location& sl = GetStartLocation();
			Location& other = *W.GetLocation(other_loc);
			Encounter* e = W.AddEncounter(enc);
			e->dialog = FindDialog("q_bandits");
			e->dont_attack = true;
			e->group = SG_BANDITS;
			e->location_event_handler = nullptr;
			e->pos = (sl.pos + other.pos) / 2;
			e->quest = (Quest_Encounter*)this;
			e->chance = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->range = 72;
			OnUpdate(game->txQuest[152]);
		}
		else
		{
			OnStart(game->txQuest[153]);
			quest_manager.RemoveQuestRumor(P_BANDYCI);

			const Item* item = Item::Get("q_bandyci_paczka");
			game->PreloadItem(item);
			game->current_dialog->pc->unit->AddItem(item, 1, true);
			other_loc = W.GetRandomSettlementIndex(start_loc);
			Location& sl = GetStartLocation();
			Location& other = *W.GetLocation(other_loc);
			Encounter* e = W.AddEncounter(enc);
			e->dialog = FindDialog("q_bandits");
			e->dont_attack = true;
			e->group = SG_BANDITS;
			e->location_event_handler = nullptr;
			e->pos = (sl.pos + other.pos) / 2;
			e->quest = (Quest_Encounter*)this;
			e->chance = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->range = 72;
			msgs.push_back(Format(game->txQuest[154], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[155], sl.name.c_str(), other.name.c_str(), GetLocationDirName(sl.pos, other.pos)));
			W.AddNews(Format(game->txQuest[156], GetStartLocationName()));

			if(Net::IsOnline() && !game->current_dialog->is_local)
			{
				game->Net_AddItem(game->current_dialog->pc, item, true);
				game->Net_AddedItemMsg(game->current_dialog->pc);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::FoundBandits:
		// podczas rozmowy z bandytami, 66% szansy na znalezienie przy nich listu za 1 razem
		{
			if(get_letter || Rand() % 3 != 0)
			{
				const Item* item = Item::Get("q_bandyci_list");
				game->PreloadItem(item);
				game->current_dialog->talker->AddItem(item, 1, true);
			}
			get_letter = true;
			OnUpdate(game->txQuest[157]);
			W.RemoveEncounter(enc);
			enc = -1;
		}
		break;
	case Progress::TalkAboutLetter:
		break;
	case Progress::NeedTalkWithCaptain:
		// info o obozie
		{
			camp_loc = W.CreateCamp(GetStartLocation().pos, SG_BANDITS);
			Location& camp = *W.GetLocation(camp_loc);
			camp.st = 10;
			camp.state = LS_HIDDEN;
			camp.active_quest = this;
			OnUpdate(game->txQuest[158]);
			target_loc = camp_loc;
			location_event_handler = this;
			game->RemoveItem(*game->current_dialog->pc->unit, Item::Get("q_bandyci_list"), 1);
		}
		break;
	case Progress::NeedClearCamp:
		// pozycja obozu
		{
			Location& camp = *W.GetLocation(camp_loc);
			OnUpdate(Format(game->txQuest[159], GetLocationDirName(GetStartLocation().pos, camp.pos)));
			camp.SetKnown();
			bandits_state = State::GenerateGuards;
		}
		break;
	case Progress::KilledBandits:
		{
			bandits_state = State::Counting;
			timer = 7.5f;

			// zmieñ ai pod¹¿aj¹cych stra¿ników
			UnitData* ud = UnitData::Get("guard_q_bandyci");
			for(vector<Unit*>::iterator it = game->local_ctx.units->begin(), end = game->local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->data == ud)
				{
					(*it)->assist = false;
					(*it)->ai->change_ai_mode = true;
				}
			}

			W.AddNews(Format(game->txQuest[160], GetStartLocationName()));
		}
		break;
	case Progress::TalkedWithAgent:
		// porazmawiano z agentem, powiedzia³ gdzie jest skrytka i idzie sobie
		{
			bandits_state = State::AgentTalked;
			game->current_dialog->talker->hero->mode = HeroData::Leave;
			game->current_dialog->talker->event_handler = this;
			Location& target = *W.CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, THRONE_VAULT, SG_BANDITS, false);
			target.active_quest = this;
			target.SetKnown();
			target.st = 10;
			target_loc = target.index;
			W.GetLocation(camp_loc)->active_quest = nullptr;
			OnUpdate(Format(game->txQuest[161], target.name.c_str(), GetLocationDirName(GetStartLocation().pos, target.pos)));
			done = false;
			at_level = 0;
			unit_to_spawn = UnitData::Get("q_bandyci_szef");
			spawn_unit_room = RoomTarget::Throne;
			unit_dont_attack = true;
			location_event_handler = nullptr;
			unit_event_handler = this;
			unit_auto_talk = true;
			callback = WarpToThroneBanditBoss;
		}
		break;
	case Progress::KilledBoss:
		// zabito szefa
		{
			camp_loc = -1;
			OnUpdate(Format(game->txQuest[162], GetStartLocationName()));
			W.AddNews(game->txQuest[163]);
		}
		break;
	case Progress::Finished:
		// ukoñczono
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[164]);
			// ustaw arto na temporary ¿eby sobie poszed³
			game->current_dialog->talker->temporary = true;
			game->AddReward(5000);
			quest_manager.EndUniqueQuest();
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
		const Item* item = Item::Get("q_bandyci_paczka");
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
		return W.GetLocation(other_loc)->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(GetStartLocation().pos, W.GetLocation(other_loc)->pos);
	else if(str == "camp_dir")
		return GetLocationDirName(GetStartLocation().pos, W.GetLocation(camp_loc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir_camp")
		return GetLocationDirName(W.GetLocation(camp_loc)->pos, GetTargetLocation().pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Bandits::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::NeedClearCamp && event == LocationEventHandler::CLEARED)
	{
		SetProgress(Progress::KilledBandits);
		return true;
	}
	return false;
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
void Quest_Bandits::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << enc;
	f << other_loc;
	f << camp_loc;
	f << get_letter;
	f << bandits_state;
	f << timer;
	f << agent;
}

//=================================================================================================
bool Quest_Bandits::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

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
		Encounter* e = W.RecreateEncounter(enc);
		e->dialog = FindDialog("q_bandits");
		e->dont_attack = true;
		e->group = SG_BANDITS;
		e->location_event_handler = nullptr;
		e->pos = (GetStartLocation().pos + W.GetLocation(other_loc)->pos) / 2;
		e->quest = (Quest_Encounter*)this;
		e->chance = 60;
		e->text = game->txQuest[11];
		e->timed = false;
		e->range = 72;
	}

	if(prog == Progress::NeedTalkWithCaptain || prog == Progress::NeedClearCamp)
		location_event_handler = this;
	else
		location_event_handler = nullptr;

	if(prog == Progress::TalkedWithAgent && !done)
	{
		unit_to_spawn = UnitData::Get("q_bandyci_szef");
		spawn_unit_room = RoomTarget::Throne;
		unit_dont_attack = true;
		location_event_handler = nullptr;
		unit_event_handler = this;
		unit_auto_talk = true;
		callback = WarpToThroneBanditBoss;
	}

	return true;
}

//=================================================================================================
void Quest_Bandits::LoadOld(GameReader& f)
{
	int old_refid, city, where;

	f >> old_refid;
	f >> city;
	f >> where;
	f >> bandits_state;
	f >> timer;
	f >> agent;
}

//=================================================================================================
bool Quest_Bandits::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_bandyci_straznikow_daj") == 0)
		return prog == Progress::NeedTalkWithCaptain && W.GetCurrentLocationIndex() == start_loc;
	return false;
}
