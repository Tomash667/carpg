#include "Pch.h"
#include "GameCore.h"
#include "Quest_Goblins.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "InsideLocation.h"
#include "GameGui.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Goblins::Start()
{
	type = QuestType::Unique;
	quest_id = Q_GOBLINS;
	enc = -1;
	goblins_state = State::None;
	nobleman = nullptr;
	messenger = nullptr;
}

//=================================================================================================
GameDialog* Quest_Goblins::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = game->current_dialog->talker->data->id;

	if(id == "q_gobliny_szlachcic")
		return FindDialog("q_goblins_nobleman");
	else if(id == "q_gobliny_mag")
		return FindDialog("q_goblins_mage");
	else if(id == "innkeeper")
		return FindDialog("q_goblins_innkeeper");
	else if(id == "q_gobliny_szlachcic2")
		return FindDialog("q_goblins_boss");
	else
	{
		assert(id == "q_gobliny_poslaniec");
		return FindDialog("q_goblins_messenger");
	}
}

//=================================================================================================
bool CzyMajaStaryLuk()
{
	return Team.HaveQuestItem(Item::Get("q_gobliny_luk"));
}

//=================================================================================================
void DodajStraznikow()
{
	Game& game = Game::Get();
	Unit* u = nullptr;
	UnitData* ud = UnitData::Get("q_gobliny_szlachcic2");

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
	Usable* use = game.local_ctx.FindUsable("throne");
	assert(use);

	// przesuñ szlachcica w pobli¿e tronu
	game.WarpUnit(*u, use->pos);

	// usuñ pozosta³e osoby z pomieszczenia
	InsideLocation* inside = (InsideLocation*)W.GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();
	Room* room = lvl.GetNearestRoom(u->pos);
	assert(room);
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data != ud && room->IsInside((*it)->pos))
		{
			(*it)->to_remove = true;
			game.to_remove.push_back(*it);
		}
	}

	// dodaj ochronê
	UnitData* ud2 = UnitData::Get("q_gobliny_ochroniarz");
	for(int i = 0; i < 3; ++i)
	{
		Unit* u2 = game.SpawnUnitInsideRoom(*room, *ud2, 10);
		if(u2)
		{
			u2->dont_attack = true;
			u2->guard_target = u;
		}
	}

	// ustaw szlachcica
	u->hero->name = game.txQuest[215];
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
			if(quest_manager.RemoveQuestRumor(P_GOBLINY))
				game->game_gui->journal->AddRumor(Format(game->txQuest[211], GetStartLocationName()));
		}
		break;
	case Progress::Started:
		// zaakceptowano
		{
			state = Quest::Started;
			name = game->txQuest[212];
			start_time = W.GetWorldtime();
			// usuñ plotkê
			quest_manager.RemoveQuestRumor(P_GOBLINY);
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
			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			msgs.push_back(Format(game->txQuest[217], GetStartLocationName(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[218], GetTargetLocationName(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// encounter
			Encounter* e = W.AddEncounter(enc);
			e->check_func = CzyMajaStaryLuk;
			e->dialog = FindDialog("q_goblins_encounter");
			e->dont_attack = true;
			e->group = SG_GOBLINS;
			e->location_event_handler = nullptr;
			e->pos = GetStartLocation().pos;
			e->quest = (Quest_Encounter*)this;
			e->chance = 10000;
			e->range = 32.f;
			e->text = game->txQuest[219];
			e->timed = false;

			if(Net::IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::BowStolen:
		// gobliny ukrad³y ³uk
		{
			game->RemoveQuestItem(Item::Get("q_gobliny_luk"));
			msgs.push_back(game->txQuest[220]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			W.RemoveEncounter(enc);
			enc = -1;
			GetTargetLocation().active_quest = nullptr;
			W.AddNews(game->txQuest[221]);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedAboutStolenBow:
		// poinformowano o kradzie¿y
		{
			state = Quest::Failed;
			msgs.push_back(game->txQuest[222]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			goblins_state = State::Counting;
			days = Random(15, 30);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::InfoAboutGoblinBase:
		// pos³aniec dostarczy³ info o bazie goblinów
		{
			state = Quest::Started;
			target_loc = W.GetRandomSpawnLocation(GetStartLocation().pos, SG_GOBLINS);
			Location& target = GetTargetLocation();
			target.state = LS_KNOWN;
			target.st = 11;
			target.reset = true;
			target.active_quest = this;
			done = false;
			spawn_item = Quest_Event::Item_GiveSpawned;
			unit_to_spawn = g_spawn_groups[SG_GOBLINS].GetSpawnLeader();
			unit_spawn_level = -3;
			item_to_give[0] = Item::Get("q_gobliny_luk");
			at_level = target.GetLastLevel();
			msgs.push_back(Format(game->txQuest[223], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			goblins_state = State::MessengerTalked;

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GivenBow:
		// oddano ³uk
		{
			state = Quest::Completed;
			const Item* item = Item::Get("q_gobliny_luk");
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->current_dialog->talker->AddItem(item, 1, true);
			game->AddReward(500);
			msgs.push_back(game->txQuest[224]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			goblins_state = State::GivenBow;
			GetTargetLocation().active_quest = nullptr;
			target_loc = -1;
			W.AddNews(game->txQuest[225]);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::DidntTalkedAboutBow:
		// nie chcia³eœ powiedzieæ o ³uku
		{
			msgs.push_back(game->txQuest[226]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			goblins_state = State::MageTalkedStart;

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedAboutBow:
		// powiedzia³eœ o ³uku
		{
			state = Quest::Started;
			msgs.push_back(game->txQuest[227]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			goblins_state = State::MageTalked;

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::PayedAndTalkedAboutBow:
		// zap³aci³eœ i powiedzia³eœ o ³uku
		{
			game->current_dialog->pc->unit->ModGold(-100);

			state = Quest::Started;
			msgs.push_back(game->txQuest[228]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			goblins_state = State::MageTalked;

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithInnkeeper:
		// pogadano z karczmarzem
		{
			goblins_state = State::KnownLocation;
			Location& target = *W.CreateLocation(L_DUNGEON, W.GetWorldPos(), 128.f, THRONE_FORT, SG_GOBLINS, false);
			target.st = 13;
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
			msgs.push_back(Format(game->txQuest[229], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::KilledBoss:
		// zabito szlachcica
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[230]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			GetTargetLocation().active_quest = nullptr;
			quest_manager.EndUniqueQuest();
			W.AddNews(game->txQuest[231]);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
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

	if(LOAD_VERSION >= V_0_4)
	{
		f >> goblins_state;
		f >> days;
		f >> nobleman;
		f >> messenger;
		f >> hd_nobleman;
	}

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
			unit_to_spawn = g_spawn_groups[SG_GOBLINS].GetSpawnLeader();
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
		e->check_func = CzyMajaStaryLuk;
		e->dialog = FindDialog("q_goblins_encounter");
		e->dont_attack = true;
		e->group = SG_GOBLINS;
		e->location_event_handler = nullptr;
		e->pos = GetStartLocation().pos;
		e->quest = (Quest_Encounter*)this;
		e->chance = 10000;
		e->range = 32.f;
		e->text = game->txQuest[219];
		e->timed = false;
	}

	return true;
}

//=================================================================================================
void Quest_Goblins::LoadOld(GameReader& f)
{
	int old_refid, city;

	f >> goblins_state;
	f >> old_refid;
	f >> city;
	f >> days;
	f >> nobleman;
	f >> messenger;
	f >> hd_nobleman;
}
