#include "Pch.h"
#include "Quest_Goblins.h"

#include "City.h"
#include "Encounter.h"
#include "Game.h"
#include "GameGui.h"
#include "InsideLocation.h"
#include "Journal.h"
#include "Level.h"
#include "OutsideLocation.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Goblins::Init()
{
	quest_mgr->RegisterSpecialIfHandler(this, "q_gobliny_zapytaj");
}

//=================================================================================================
void Quest_Goblins::Start()
{
	category = QuestCategory::Unique;
	type = Q_GOBLINS;
	startLoc = world->GetRandomSettlement(quest_mgr->GetUsedCities(), CITY);
	enc = -1;
	goblins_state = State::None;
	nobleman = nullptr;
	messenger = nullptr;
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[7], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Goblins' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Goblins::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = DialogContext::current->talker->data->id;

	if(id == "q_gobliny_szlachcic")
		return GameDialog::TryGet("q_goblins_nobleman");
	else if(id == "q_gobliny_mag")
		return GameDialog::TryGet("q_goblins_mage");
	else if(id == "innkeeper")
		return GameDialog::TryGet("q_goblins_innkeeper");
	else if(id == "q_gobliny_szlachcic2")
		return GameDialog::TryGet("q_goblins_boss");
	else
	{
		assert(id == "q_gobliny_poslaniec");
		return GameDialog::TryGet("q_goblins_messenger");
	}
}

//=================================================================================================
bool TeamHaveOldBow()
{
	return team->HaveQuestItem(Item::Get("q_gobliny_luk"));
}

//=================================================================================================
void DodajStraznikow()
{
	// find nobleman
	UnitData* ud = UnitData::Get("q_gobliny_szlachcic2");
	Unit* u = game_level->local_area->FindUnit(ud);
	assert(u);

	// find throne
	Usable* use = game_level->local_area->FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// warp nobleman to throne
	game_level->WarpUnit(*u, use->pos);
	u->hero->know_name = true;
	u->ApplyHumanData(quest_mgr->quest_goblins->hd_nobleman);

	// remove other units
	InsideLocation* inside = (InsideLocation*)world->GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();
	Room* room = lvl.GetNearestRoom(u->pos);
	assert(room);
	for(vector<Unit*>::iterator it = game_level->local_area->units.begin(), end = game_level->local_area->units.end(); it != end; ++it)
	{
		if((*it)->data != ud && room->IsInside((*it)->pos))
		{
			(*it)->to_remove = true;
			game_level->to_remove.push_back(*it);
		}
	}

	// spawn bodyguards
	UnitData* ud2 = UnitData::Get("q_gobliny_ochroniarz");
	for(int i = 0; i < 3; ++i)
	{
		Unit* u2 = game_level->SpawnUnitInsideRoom(*room, *ud2, 10);
		if(u2)
		{
			u2->dont_attack = true;
			u2->OrderGuard(u);
		}
	}
}

//=================================================================================================
void Quest_Goblins::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::NotAccepted:
		{
			if(quest_mgr->RemoveQuestRumor(id))
				game_gui->journal->AddRumor(Format(quest_mgr->txQuest[211], GetStartLocationName()));
		}
		break;
	case Progress::Started:
		{
			OnStart(quest_mgr->txQuest[212]);
			quest_mgr->RemoveQuestRumor(id);
			// add location
			targetLoc = world->GetClosestLocation(L_OUTSIDE, startLoc->pos, FOREST);
			targetLoc->SetKnown();
			targetLoc->reset = true;
			targetLoc->active_quest = this;
			targetLoc->st = 7;
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = Item::Get("q_gobliny_luk");
			// add journal entry
			msgs.push_back(Format(quest_mgr->txQuest[217], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(quest_mgr->txQuest[218], GetTargetLocationName(), GetTargetLocationDir()));
			// encounter
			Encounter* e = world->AddEncounter(enc);
			e->check_func = TeamHaveOldBow;
			e->dialog = GameDialog::TryGet("q_goblins_encounter");
			e->dont_attack = true;
			e->group = UnitGroup::Get("goblins");
			e->location_event_handler = nullptr;
			e->pos = startLoc->pos;
			e->quest = this;
			e->chance = 10000;
			e->range = 32.f;
			e->text = quest_mgr->txQuest[219];
			e->timed = false;
			e->st = 6;
		}
		break;
	case Progress::BowStolen:
		{
			team->RemoveQuestItem(Item::Get("q_gobliny_luk"));
			OnUpdate(quest_mgr->txQuest[220]);
			world->RemoveEncounter(enc);
			enc = -1;
			targetLoc->active_quest = nullptr;
			world->AddNews(quest_mgr->txQuest[221]);
			team->AddExp(1000);
		}
		break;
	case Progress::TalkedAboutStolenBow:
		{
			state = Quest::Failed;
			OnUpdate(quest_mgr->txQuest[222]);
			goblins_state = State::Counting;
			days = Random(15, 30);
		}
		break;
	case Progress::InfoAboutGoblinBase:
		{
			state = Quest::Started;
			targetLoc = world->GetRandomSpawnLocation(startLoc->pos, UnitGroup::Get("goblins"));
			targetLoc->state = LS_KNOWN;
			targetLoc->st = 10;
			targetLoc->reset = true;
			targetLoc->active_quest = this;
			done = false;
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = UnitGroup::Get("goblins")->GetLeader(11);
			unit_spawn_level = -3;
			item_to_give[0] = Item::Get("q_gobliny_luk");
			at_level = targetLoc->GetLastLevel();
			OnUpdate(Format(quest_mgr->txQuest[223], targetLoc->name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			goblins_state = State::MessengerTalked;
		}
		break;
	case Progress::GivenBow:
		{
			state = Quest::Completed;
			const Item* item = Item::Get("q_gobliny_luk");
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			DialogContext::current->talker->AddItem(item, 1, true);
			team->AddReward(500, 2500);
			OnUpdate(quest_mgr->txQuest[224]);
			goblins_state = State::GivenBow;
			targetLoc->active_quest = nullptr;
			targetLoc = nullptr;
			world->AddNews(quest_mgr->txQuest[225]);
		}
		break;
	case Progress::DidntTalkedAboutBow:
		{
			OnUpdate(quest_mgr->txQuest[226]);
			goblins_state = State::MageTalkedStart;
		}
		break;
	case Progress::TalkedAboutBow:
		{
			state = Quest::Started;
			OnUpdate(quest_mgr->txQuest[227]);
			goblins_state = State::MageTalked;
		}
		break;
	case Progress::PayedAndTalkedAboutBow:
		{
			DialogContext::current->pc->unit->ModGold(-100);

			state = Quest::Started;
			OnUpdate(quest_mgr->txQuest[228]);
			goblins_state = State::MageTalked;
		}
		break;
	case Progress::TalkedWithInnkeeper:
		{
			goblins_state = State::KnownLocation;
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 128.f);
			Location& target = *world->CreateLocation(L_DUNGEON, pos, THRONE_FORT);
			target.group = UnitGroup::Get("goblins");
			target.st = 12;
			target.SetKnown();
			target.active_quest = this;
			targetLoc = &target;
			done = false;
			unit_to_spawn = UnitData::Get("q_gobliny_szlachcic2");
			spawn_unit_room = RoomTarget::Throne;
			callback = DodajStraznikow;
			unit_dont_attack = true;
			unit_auto_talk = true;
			unit_event_handler = this;
			item_to_give[0] = nullptr;
			spawn_item = Quest_Event::Item_DontSpawn;
			at_level = target.GetLastLevel();
			OnUpdate(Format(quest_mgr->txQuest[229], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
		}
		break;
	case Progress::KilledBoss:
		{
			state = Quest::Completed;
			OnUpdate(quest_mgr->txQuest[230]);
			targetLoc->active_quest = nullptr;
			quest_mgr->EndUniqueQuest();
			world->AddNews(quest_mgr->txQuest[231]);
			team->AddLearningPoint();
			team->AddExp(15000);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Goblins::FormatString(const string& str)
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
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Goblins::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "gobliny") == 0;
}

//=================================================================================================
void Quest_Goblins::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::DIE && prog == Progress::TalkedWithInnkeeper)
	{
		SetProgress(Progress::KilledBoss);
		unit->event_handler = nullptr;
	}
}

//=================================================================================================
void Quest_Goblins::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << enc;
	f << goblins_state;
	f << days;
	f << nobleman;
	f << messenger;
	f << hd_nobleman;
}

//=================================================================================================
Quest::LoadResult Quest_Goblins::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> enc;
	f >> goblins_state;
	f >> days;
	f >> nobleman;
	f >> messenger;
	f >> hd_nobleman;

	if(!done)
	{
		if(prog == Progress::Started)
		{
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = Item::Get("q_gobliny_luk");
		}
		else if(prog == Progress::InfoAboutGoblinBase)
		{
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = UnitGroup::Get("goblins")->GetLeader(11);
			unit_spawn_level = -3;
			item_to_give[0] = Item::Get("q_gobliny_luk");
		}
		else if(prog == Progress::TalkedWithInnkeeper)
		{
			unit_to_spawn = UnitData::Get("q_gobliny_szlachcic2");
			spawn_unit_room = RoomTarget::Throne;
			callback = DodajStraznikow;
			unit_dont_attack = true;
			unit_auto_talk = true;
		}
	}

	unit_event_handler = this;

	if(enc != -1)
	{
		Encounter* e = world->RecreateEncounter(enc);
		e->check_func = TeamHaveOldBow;
		e->dialog = GameDialog::TryGet("q_goblins_encounter");
		e->dont_attack = true;
		e->group = UnitGroup::Get("goblins");
		e->location_event_handler = nullptr;
		e->pos = startLoc->pos;
		e->quest = this;
		e->chance = 10000;
		e->range = 32.f;
		e->text = quest_mgr->txQuest[219];
		e->timed = false;
		e->st = 6;
	}

	return LoadResult::Ok;
}

//=================================================================================================
bool Quest_Goblins::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_gobliny_zapytaj") == 0)
	{
		return goblins_state >= State::MageTalked
			&& goblins_state < State::KnownLocation
			&& game_level->location == startLoc
			&& prog != Progress::TalkedWithInnkeeper;
	}
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Goblins::OnProgress(int d)
{
	if(goblins_state == Quest_Goblins::State::Counting || goblins_state == Quest_Goblins::State::NoblemanLeft)
		days -= d;
}
