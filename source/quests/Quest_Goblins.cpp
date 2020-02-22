#include "Pch.h"
#include "Quest_Goblins.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "InsideLocation.h"
#include "OutsideLocation.h"
#include "GameGui.h"
#include "Team.h"
#include "World.h"
#include "Level.h"

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
	enc = -1;
	goblins_state = State::None;
	nobleman = nullptr;
	messenger = nullptr;
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[7], GetStartLocationName()));
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
	// szukaj szlachcica
	UnitData* ud = UnitData::Get("q_gobliny_szlachcic2");
	Unit* u = game_level->local_area->FindUnit(ud);
	assert(u);

	// szukaj tronu
	Usable* use = game_level->local_area->FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// przesuñ szlachcica w pobli¿e tronu
	game_level->WarpUnit(*u, use->pos);

	// usuñ pozosta³e osoby z pomieszczenia
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

	// dodaj ochronê
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

	// ustaw szlachcica
	u->hero->know_name = true;
	u->ApplyHumanData(quest_mgr->quest_goblins->hd_nobleman);
}

//=================================================================================================
void Quest_Goblins::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::NotAccepted:
		// nie zaakceptowano
		{
			if(quest_mgr->RemoveQuestRumor(id))
				game_gui->journal->AddRumor(Format(game->txQuest[211], GetStartLocationName()));
		}
		break;
	case Progress::Started:
		// zaakceptowano
		{
			OnStart(game->txQuest[212]);
			// usuñ plotkê
			quest_mgr->RemoveQuestRumor(id);
			// dodaj lokalizacje
			target_loc = world->GetClosestLocation(L_OUTSIDE, GetStartLocation().pos, FOREST);
			Location& target = GetTargetLocation();
			target.SetKnown();
			target.reset = true;
			target.active_quest = this;
			target.st = 7;
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = Item::Get("q_gobliny_luk");
			// questowe rzeczy
			msgs.push_back(Format(game->txQuest[217], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[218], GetTargetLocationName(), GetTargetLocationDir()));
			// encounter
			Encounter* e = world->AddEncounter(enc);
			e->check_func = TeamHaveOldBow;
			e->dialog = GameDialog::TryGet("q_goblins_encounter");
			e->dont_attack = true;
			e->group = UnitGroup::Get("goblins");
			e->location_event_handler = nullptr;
			e->pos = GetStartLocation().pos;
			e->quest = this;
			e->chance = 10000;
			e->range = 32.f;
			e->text = game->txQuest[219];
			e->timed = false;
			e->st = 6;
		}
		break;
	case Progress::BowStolen:
		// gobliny ukrad³y ³uk
		{
			team->RemoveQuestItem(Item::Get("q_gobliny_luk"));
			OnUpdate(game->txQuest[220]);
			world->RemoveEncounter(enc);
			enc = -1;
			GetTargetLocation().active_quest = nullptr;
			world->AddNews(game->txQuest[221]);
			team->AddExp(1000);
		}
		break;
	case Progress::TalkedAboutStolenBow:
		// poinformowano o kradzie¿y
		{
			state = Quest::Failed;
			OnUpdate(game->txQuest[222]);
			goblins_state = State::Counting;
			days = Random(15, 30);
		}
		break;
	case Progress::InfoAboutGoblinBase:
		// pos³aniec dostarczy³ info o bazie goblinów
		{
			state = Quest::Started;
			target_loc = world->GetRandomSpawnLocation(GetStartLocation().pos, UnitGroup::Get("goblins"));
			Location& target = GetTargetLocation();
			target.state = LS_KNOWN;
			target.st = 10;
			target.reset = true;
			target.active_quest = this;
			done = false;
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = UnitGroup::Get("goblins")->GetLeader(11);
			unit_spawn_level = -3;
			item_to_give[0] = Item::Get("q_gobliny_luk");
			at_level = target.GetLastLevel();
			OnUpdate(Format(game->txQuest[223], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			goblins_state = State::MessengerTalked;
		}
		break;
	case Progress::GivenBow:
		// oddano ³uk
		{
			state = Quest::Completed;
			const Item* item = Item::Get("q_gobliny_luk");
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			DialogContext::current->talker->AddItem(item, 1, true);
			team->AddReward(500, 2500);
			OnUpdate(game->txQuest[224]);
			goblins_state = State::GivenBow;
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			world->AddNews(game->txQuest[225]);
		}
		break;
	case Progress::DidntTalkedAboutBow:
		// nie chcia³eœ powiedzieæ o ³uku
		{
			OnUpdate(game->txQuest[226]);
			goblins_state = State::MageTalkedStart;
		}
		break;
	case Progress::TalkedAboutBow:
		// powiedzia³eœ o ³uku
		{
			state = Quest::Started;
			OnUpdate(game->txQuest[227]);
			goblins_state = State::MageTalked;
		}
		break;
	case Progress::PayedAndTalkedAboutBow:
		// zap³aci³eœ i powiedzia³eœ o ³uku
		{
			DialogContext::current->pc->unit->ModGold(-100);

			state = Quest::Started;
			OnUpdate(game->txQuest[228]);
			goblins_state = State::MageTalked;
		}
		break;
	case Progress::TalkedWithInnkeeper:
		// pogadano z karczmarzem
		{
			goblins_state = State::KnownLocation;
			Location& target = *world->CreateLocation(L_DUNGEON, world->GetWorldPos(), 128.f, THRONE_FORT, UnitGroup::Get("goblins"), false);
			target.st = 12;
			target.SetKnown();
			target.active_quest = this;
			target_loc = target.index;
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
			OnUpdate(Format(game->txQuest[229], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
		}
		break;
	case Progress::KilledBoss:
		// zabito szlachcica
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[230]);
			GetTargetLocation().active_quest = nullptr;
			quest_mgr->EndUniqueQuest();
			world->AddNews(game->txQuest[231]);
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
		e->pos = GetStartLocation().pos;
		e->quest = this;
		e->chance = 10000;
		e->range = 32.f;
		e->text = game->txQuest[219];
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
			&& game_level->location_index == start_loc
			&& prog != Progress::TalkedWithInnkeeper;
	}
	assert(0);
	return false;
}
