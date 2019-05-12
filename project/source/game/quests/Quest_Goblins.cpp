#include "Pch.h"
#include "GameCore.h"
#include "Quest_Goblins.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "InsideLocation.h"
#include "GlobalGui.h"
#include "Team.h"
#include "World.h"
#include "Level.h"

//=================================================================================================
void Quest_Goblins::Init()
{
	QM.RegisterSpecialIfHandler(this, "q_gobliny_zapytaj");
}

//=================================================================================================
void Quest_Goblins::Start()
{
	type = QuestType::Unique;
	quest_id = Q_GOBLINS;
	enc = -1;
	goblins_state = State::None;
	nobleman = nullptr;
	messenger = nullptr;
	QM.AddQuestRumor(refid, Format(QM.txRumorQ[7], GetStartLocationName()));
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
	return Team.HaveQuestItem(Item::Get("q_gobliny_luk"));
}

//=================================================================================================
void DodajStraznikow()
{
	Unit* u = nullptr;
	UnitData* ud = UnitData::Get("q_gobliny_szlachcic2");

	// szukaj szlachcica
	for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			u = *it;
			break;
		}
	}
	assert(u);

	// szukaj tronu
	Usable* use = L.local_ctx.FindUsable("throne");
	assert(use);

	// przesuñ szlachcica w pobli¿e tronu
	L.WarpUnit(*u, use->pos);

	// usuñ pozosta³e osoby z pomieszczenia
	InsideLocation* inside = (InsideLocation*)W.GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();
	Room* room = lvl.GetNearestRoom(u->pos);
	assert(room);
	for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data != ud && room->IsInside((*it)->pos))
		{
			(*it)->to_remove = true;
			L.to_remove.push_back(*it);
		}
	}

	// dodaj ochronê
	UnitData* ud2 = UnitData::Get("q_gobliny_ochroniarz");
	for(int i = 0; i < 3; ++i)
	{
		Unit* u2 = L.SpawnUnitInsideRoom(*room, *ud2, 10);
		if(u2)
		{
			u2->dont_attack = true;
			u2->guard_target = u;
		}
	}

	// ustaw szlachcica
	u->hero->know_name = true;
	u->ApplyHumanData(QM.quest_goblins->hd_nobleman);
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
			if(QM.RemoveQuestRumor(refid))
				game->gui->journal->AddRumor(Format(game->txQuest[211], GetStartLocationName()));
		}
		break;
	case Progress::Started:
		// zaakceptowano
		{
			OnStart(game->txQuest[212]);
			// usuñ plotkê
			QM.RemoveQuestRumor(refid);
			// dodaj lokalizacje
			target_loc = W.GetNearestLocation(GetStartLocation().pos, 1 << L_FOREST, true);
			Location& target = GetTargetLocation();
			target.SetKnown();
			target.reset = true;
			target.active_quest = this;
			target.st = 7;
			spawn_item = Quest_Event::Item_OnGround;
			item_to_give[0] = Item::Get("q_gobliny_luk");
			// questowe rzeczy
			msgs.push_back(Format(game->txQuest[217], GetStartLocationName(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[218], GetTargetLocationName(), GetTargetLocationDir()));
			// encounter
			Encounter* e = W.AddEncounter(enc);
			e->check_func = TeamHaveOldBow;
			e->dialog = GameDialog::TryGet("q_goblins_encounter");
			e->dont_attack = true;
			e->group = UnitGroup::Get("goblins");
			e->location_event_handler = nullptr;
			e->pos = GetStartLocation().pos;
			e->quest = static_cast<Quest*>(this);
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
			Team.RemoveQuestItem(Item::Get("q_gobliny_luk"));
			OnUpdate(game->txQuest[220]);
			W.RemoveEncounter(enc);
			enc = -1;
			GetTargetLocation().active_quest = nullptr;
			W.AddNews(game->txQuest[221]);
			Team.AddExp(1000);
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
			target_loc = W.GetRandomSpawnLocation(GetStartLocation().pos, UnitGroup::Get("goblins"));
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
			Team.AddReward(500, 2500);
			OnUpdate(game->txQuest[224]);
			goblins_state = State::GivenBow;
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			W.AddNews(game->txQuest[225]);
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
			Location& target = *W.CreateLocation(L_DUNGEON, W.GetWorldPos(), 128.f, THRONE_FORT, UnitGroup::Get("goblins"), false);
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
			QM.EndUniqueQuest();
			W.AddNews(game->txQuest[231]);
			Team.AddLearningPoint();
			Team.AddExp(15000);
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
bool Quest_Goblins::Load(GameReader& f)
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
		Encounter* e = W.RecreateEncounter(enc);
		e->check_func = TeamHaveOldBow;
		e->dialog = GameDialog::TryGet("q_goblins_encounter");
		e->dont_attack = true;
		e->group = UnitGroup::Get("goblins");
		e->location_event_handler = nullptr;
		e->pos = GetStartLocation().pos;
		e->quest = reinterpret_cast<Quest_Encounter*>(this);
		e->chance = 10000;
		e->range = 32.f;
		e->text = game->txQuest[219];
		e->timed = false;
		e->st = 6;
	}

	return true;
}

//=================================================================================================
bool Quest_Goblins::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_gobliny_zapytaj") == 0)
	{
		return goblins_state >= State::MageTalked
			&& goblins_state < State::KnownLocation
			&& L.location_index == start_loc
			&& prog != Progress::TalkedWithInnkeeper;
	}
	assert(0);
	return false;
}
