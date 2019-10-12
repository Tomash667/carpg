#include "Pch.h"
#include "GameCore.h"
#include "Quest_Bandits.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "AIController.h"
#include "World.h"
#include "Level.h"
#include "Team.h"
#include "GameResources.h"

//=================================================================================================
void Quest_Bandits::Init()
{
	quest_mgr->RegisterSpecialIfHandler(this, "q_bandyci_straznikow_daj");
}

//=================================================================================================
void Quest_Bandits::Start()
{
	type = Q_BANDITS;
	category = QuestCategory::Unique;
	enc = -1;
	other_loc = -1;
	camp_loc = -1;
	target_loc = -1;
	get_letter = false;
	bandits_state = State::None;
	timer = 0.f;
	agent = nullptr;
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[3], GetStartLocationName()));
}

//=================================================================================================
GameDialog* Quest_Bandits::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = DialogContext::current->talker->data->id;
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

	return GameDialog::TryGet(dialog_id);
}

//=================================================================================================
void WarpToThroneBanditBoss()
{
	// search for boss
	UnitData* ud = UnitData::Get("q_bandyci_szef");
	Unit* u = game_level->local_area->FindUnit(ud);
	assert(u);

	// search for throne
	Usable* use = game_level->local_area->FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// warp boss to throne
	game_level->WarpUnit(*u, use->pos);
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
			if(!DialogContext::current->pc->unit->HaveItem(item))
				DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
			Location& sl = GetStartLocation();
			Location& other = *world->GetLocation(other_loc);
			Encounter* e = world->AddEncounter(enc);
			e->dialog = GameDialog::TryGet("q_bandits");
			e->dont_attack = true;
			e->group = UnitGroup::Get("bandits");
			e->location_event_handler = nullptr;
			e->pos = (sl.pos + other.pos) / 2;
			e->quest = this;
			e->chance = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->range = 72;
			e->st = 8;
			OnUpdate(game->txQuest[152]);
		}
		else
		{
			OnStart(game->txQuest[153]);
			quest_mgr->RemoveQuestRumor(id);

			const Item* item = Item::Get("q_bandyci_paczka");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
			other_loc = world->GetRandomSettlementIndex(start_loc);
			Location& sl = GetStartLocation();
			Location& other = *world->GetLocation(other_loc);
			Encounter* e = world->AddEncounter(enc);
			e->dialog = GameDialog::TryGet("q_bandits");
			e->dont_attack = true;
			e->group = UnitGroup::Get("bandits");
			e->location_event_handler = nullptr;
			e->pos = (sl.pos + other.pos) / 2;
			e->quest = this;
			e->chance = 60;
			e->text = game->txQuest[11];
			e->timed = false;
			e->range = 72;
			e->st = 8;
			msgs.push_back(Format(game->txQuest[154], sl.name.c_str(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[155], sl.name.c_str(), other.name.c_str(), GetLocationDirName(sl.pos, other.pos)));
			world->AddNews(Format(game->txQuest[156], GetStartLocationName()));
		}
		break;
	case Progress::FoundBandits:
		// podczas rozmowy z bandytami, 66% szansy na znalezienie przy nich listu za 1 razem
		{
			if(get_letter || Rand() % 3 != 0)
			{
				const Item* item = Item::Get("q_bandyci_list");
				game_res->PreloadItem(item);
				DialogContext::current->talker->AddItem(item, 1, true);
			}
			get_letter = true;
			OnUpdate(game->txQuest[157]);
			world->RemoveEncounter(enc);
			enc = -1;
		}
		break;
	case Progress::TalkAboutLetter:
		break;
	case Progress::NeedTalkWithCaptain:
		// info o obozie
		{
			camp_loc = world->CreateCamp(GetStartLocation().pos, UnitGroup::Get("bandits"));
			Location& camp = *world->GetLocation(camp_loc);
			camp.st = 10;
			camp.state = LS_HIDDEN;
			camp.active_quest = this;
			OnUpdate(game->txQuest[158]);
			target_loc = camp_loc;
			location_event_handler = this;
			DialogContext::current->pc->unit->RemoveItem(Item::Get("q_bandyci_list"), 1);
			team->AddExp(5000);
		}
		break;
	case Progress::NeedClearCamp:
		// pozycja obozu
		{
			Location& camp = *world->GetLocation(camp_loc);
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
			for(Unit* unit : game_level->local_area->units)
			{
				if(unit->data == ud)
				{
					unit->assist = false;
					unit->ai->change_ai_mode = true;
				}
			}

			world->AddNews(Format(game->txQuest[160], GetStartLocationName()));
		}
		break;
	case Progress::TalkedWithAgent:
		// talked with agent, he told where is bandit secret hideout is, now he is leaving
		{
			bandits_state = State::AgentTalked;
			DialogContext::current->talker->OrderLeave();
			DialogContext::current->talker->event_handler = this;
			Location& target = *world->CreateLocation(L_DUNGEON, GetStartLocation().pos, 64.f, THRONE_VAULT, UnitGroup::Get("bandits"), false);
			target.active_quest = this;
			target.SetKnown();
			target.st = 12;
			target_loc = target.index;
			world->GetLocation(camp_loc)->active_quest = nullptr;
			OnUpdate(Format(game->txQuest[161], target.name.c_str(), GetLocationDirName(GetStartLocation().pos, target.pos)));
			done = false;
			at_level = 1;
			unit_to_spawn = UnitData::Get("q_bandyci_szef");
			spawn_unit_room = RoomTarget::Throne;
			unit_dont_attack = true;
			location_event_handler = nullptr;
			unit_event_handler = this;
			unit_auto_talk = true;
			callback = WarpToThroneBanditBoss;
			team->AddExp(7500);
		}
		break;
	case Progress::KilledBoss:
		// zabito szefa
		{
			camp_loc = -1;
			OnUpdate(Format(game->txQuest[162], GetStartLocationName()));
			world->AddNews(game->txQuest[163]);
			team->AddLearningPoint();
		}
		break;
	case Progress::Finished:
		// ukoñczono
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[164]);
			// ustaw arto na temporary ¿eby sobie poszed³
			DialogContext::current->talker->temporary = true;
			team->AddReward(10000, 25000);
			quest_mgr->EndUniqueQuest();
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
bool Quest_Bandits::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "bandyci_daj_paczke") == 0)
	{
		const Item* item = Item::Get("q_bandyci_paczka");
		ctx.talker->AddItem(item, 1, true);
		team->RemoveQuestItem(item);
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Bandits::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_bandyci_straznikow_daj") == 0)
		return prog == Progress::NeedTalkWithCaptain && world->GetCurrentLocationIndex() == start_loc;
	assert(0);
	return false;
}

//=================================================================================================
cstring Quest_Bandits::FormatString(const string& str)
{
	if(str == "start_loc")
		return GetStartLocationName();
	else if(str == "other_loc")
		return world->GetLocation(other_loc)->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(GetStartLocation().pos, world->GetLocation(other_loc)->pos);
	else if(str == "camp_dir")
		return GetLocationDirName(GetStartLocation().pos, world->GetLocation(camp_loc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir_camp")
		return GetLocationDirName(world->GetLocation(camp_loc)->pos, GetTargetLocation().pos);
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
	f >> bandits_state;
	f >> timer;
	f >> agent;

	if(enc != -1)
	{
		Encounter* e = world->RecreateEncounter(enc);
		e->dialog = GameDialog::TryGet("q_bandits");
		e->dont_attack = true;
		e->group = UnitGroup::Get("bandits");
		e->location_event_handler = nullptr;
		e->pos = (GetStartLocation().pos + world->GetLocation(other_loc)->pos) / 2;
		e->quest = this;
		e->chance = 60;
		e->text = game->txQuest[11];
		e->timed = false;
		e->range = 72;
		e->st = 8;
	}

	if(prog == Progress::NeedTalkWithCaptain || prog == Progress::NeedClearCamp)
		location_event_handler = this;
	else
		location_event_handler = nullptr;

	if(prog == Progress::TalkedWithAgent && !done)
	{
		unit_to_spawn = UnitData::Get("q_bandyci_szef");
		spawn_unit_room = RoomTarget::Throne;
		at_level = 1;
		unit_dont_attack = true;
		location_event_handler = nullptr;
		unit_event_handler = this;
		unit_auto_talk = true;
		callback = WarpToThroneBanditBoss;
	}

	return true;
}

//=================================================================================================
void Quest_Bandits::Update(float dt)
{
	if(bandits_state == State::Counting)
	{
		timer -= dt;
		if(timer <= 0.f)
		{
			// spawn agent
			agent = game_level->SpawnUnitNearLocation(*team->leader->area, team->leader->pos, *UnitData::Get("agent"), &team->leader->pos, -2, 2.f);
			if(agent)
			{
				bandits_state = State::AgentCome;
				agent->OrderAutoTalk(true);
			}
		}
	}
}
