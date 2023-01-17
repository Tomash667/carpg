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
	questMgr->RegisterSpecialIfHandler(this, "q_gobliny_zapytaj");
}

//=================================================================================================
void Quest_Goblins::Start()
{
	category = QuestCategory::Unique;
	type = Q_GOBLINS;
	startLoc = world->GetRandomSettlement(questMgr->GetUsedCities(), CITY);
	enc = -1;
	goblinsState = State::None;
	nobleman = nullptr;
	messenger = nullptr;
	questMgr->AddQuestRumor(id, Format(questMgr->txRumorQ[7], GetStartLocationName()));

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
	return team->HaveQuestItem(Item::Get("qGoblinsBow"));
}

//=================================================================================================
void DodajStraznikow()
{
	// find nobleman
	UnitData* ud = UnitData::Get("q_gobliny_szlachcic2");
	Unit* u = gameLevel->localPart->FindUnit(ud);
	assert(u);

	// find throne
	Usable* use = gameLevel->localPart->FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// warp nobleman to throne
	gameLevel->WarpUnit(*u, use->pos);
	u->hero->knowName = true;
	u->ApplyHumanData(questMgr->questGoblins->hdNobleman);

	// remove other units
	InsideLocation* inside = (InsideLocation*)world->GetCurrentLocation();
	InsideLocationLevel& lvl = inside->GetLevelData();
	Room* room = lvl.GetNearestRoom(u->pos);
	assert(room);
	for(vector<Unit*>::iterator it = gameLevel->localPart->units.begin(), end = gameLevel->localPart->units.end(); it != end; ++it)
	{
		if((*it)->data != ud && room->IsInside((*it)->pos))
		{
			(*it)->toRemove = true;
			gameLevel->toRemove.push_back(*it);
		}
	}

	// spawn bodyguards
	UnitData* ud2 = UnitData::Get("q_gobliny_ochroniarz");
	for(int i = 0; i < 3; ++i)
	{
		Unit* u2 = gameLevel->SpawnUnitInsideRoom(*room, *ud2, 10);
		if(u2)
		{
			u2->dontAttack = true;
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
			if(questMgr->RemoveQuestRumor(id))
				gameGui->journal->AddRumor(Format(questMgr->txQuest[211], GetStartLocationName()));
		}
		break;
	case Progress::Started:
		{
			OnStart(questMgr->txQuest[212]);
			questMgr->RemoveQuestRumor(id);
			// add location
			targetLoc = world->GetClosestLocation(L_OUTSIDE, startLoc->pos, FOREST);
			targetLoc->SetKnown();
			targetLoc->reset = true;
			targetLoc->activeQuest = this;
			targetLoc->st = 7;
			spawnItem = Quest_Event::Item_OnGround;
			itemToGive[0] = Item::Get("qGoblinsBow");
			// add journal entry
			msgs.push_back(Format(questMgr->txQuest[217], GetStartLocationName(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[218], GetTargetLocationName(), GetTargetLocationDir()));
			// encounter
			Encounter* e = world->AddEncounter(enc);
			e->checkFunc = TeamHaveOldBow;
			e->dialog = GameDialog::TryGet("q_goblins_encounter");
			e->dontAttack = true;
			e->group = UnitGroup::Get("goblins");
			e->locationEventHandler = nullptr;
			e->pos = startLoc->pos;
			e->quest = this;
			e->chance = 10000;
			e->range = 32.f;
			e->text = questMgr->txQuest[219];
			e->timed = false;
			e->st = 6;
		}
		break;
	case Progress::BowStolen:
		{
			team->RemoveQuestItem(Item::Get("qGoblinsBow"));
			OnUpdate(questMgr->txQuest[220]);
			world->RemoveEncounter(enc);
			enc = -1;
			targetLoc->activeQuest = nullptr;
			world->AddNews(questMgr->txQuest[221]);
			team->AddExp(1000);
		}
		break;
	case Progress::TalkedAboutStolenBow:
		{
			state = Quest::Failed;
			OnUpdate(questMgr->txQuest[222]);
			goblinsState = State::Counting;
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
			targetLoc->activeQuest = this;
			done = false;
			spawnItem = Quest_Event::Item_GiveSpawned;
			unitToSpawn = UnitGroup::Get("goblins")->GetLeader(11);
			unitSpawnLevel = -3;
			itemToGive[0] = Item::Get("qGoblinsBow");
			atLevel = targetLoc->GetLastLevel();
			OnUpdate(Format(questMgr->txQuest[223], targetLoc->name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			goblinsState = State::MessengerTalked;
		}
		break;
	case Progress::GivenBow:
		{
			state = Quest::Completed;
			const Item* item = Item::Get("qGoblinsBow");
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			DialogContext::current->talker->AddItem(item, 1, true);
			team->AddReward(500, 2500);
			OnUpdate(questMgr->txQuest[224]);
			goblinsState = State::GivenBow;
			targetLoc->activeQuest = nullptr;
			targetLoc = nullptr;
			world->AddNews(questMgr->txQuest[225]);
		}
		break;
	case Progress::DidntTalkedAboutBow:
		{
			OnUpdate(questMgr->txQuest[226]);
			goblinsState = State::MageTalkedStart;
		}
		break;
	case Progress::TalkedAboutBow:
		{
			state = Quest::Started;
			OnUpdate(questMgr->txQuest[227]);
			goblinsState = State::MageTalked;
		}
		break;
	case Progress::PayedAndTalkedAboutBow:
		{
			DialogContext::current->pc->unit->ModGold(-100);

			state = Quest::Started;
			OnUpdate(questMgr->txQuest[228]);
			goblinsState = State::MageTalked;
		}
		break;
	case Progress::TalkedWithInnkeeper:
		{
			goblinsState = State::KnownLocation;
			const Vec2 pos = world->FindPlace(world->GetWorldPos(), 128.f);
			Location& target = *world->CreateLocation(L_DUNGEON, pos, THRONE_FORT);
			target.group = UnitGroup::Get("goblins");
			target.st = 12;
			target.SetKnown();
			target.activeQuest = this;
			targetLoc = &target;
			done = false;
			unitToSpawn = UnitData::Get("q_gobliny_szlachcic2");
			unitSpawnRoom = RoomTarget::Throne;
			callback = DodajStraznikow;
			unitDontAttack = true;
			unitAutoTalk = true;
			unitEventHandler = this;
			itemToGive[0] = nullptr;
			spawnItem = Quest_Event::Item_DontSpawn;
			atLevel = target.GetLastLevel();
			OnUpdate(Format(questMgr->txQuest[229], target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
		}
		break;
	case Progress::KilledBoss:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[230]);
			targetLoc->activeQuest = nullptr;
			questMgr->EndUniqueQuest();
			world->AddNews(questMgr->txQuest[231]);
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
		unit->eventHandler = nullptr;
	}
}

//=================================================================================================
void Quest_Goblins::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << enc;
	f << goblinsState;
	f << days;
	f << nobleman;
	f << messenger;
	f << hdNobleman;
}

//=================================================================================================
Quest::LoadResult Quest_Goblins::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> enc;
	f >> goblinsState;
	f >> days;
	f >> nobleman;
	f >> messenger;
	f >> hdNobleman;

	if(!done)
	{
		if(prog == Progress::Started)
		{
			spawnItem = Quest_Event::Item_OnGround;
			itemToGive[0] = Item::Get("qGoblinsBow");
		}
		else if(prog == Progress::InfoAboutGoblinBase)
		{
			spawnItem = Quest_Event::Item_GiveSpawned;
			unitToSpawn = UnitGroup::Get("goblins")->GetLeader(11);
			unitSpawnLevel = -3;
			itemToGive[0] = Item::Get("qGoblinsBow");
		}
		else if(prog == Progress::TalkedWithInnkeeper)
		{
			unitToSpawn = UnitData::Get("q_gobliny_szlachcic2");
			unitSpawnRoom = RoomTarget::Throne;
			callback = DodajStraznikow;
			unitDontAttack = true;
			unitAutoTalk = true;
		}
	}

	unitEventHandler = this;

	if(enc != -1)
	{
		Encounter* e = world->RecreateEncounter(enc);
		e->checkFunc = TeamHaveOldBow;
		e->dialog = GameDialog::TryGet("q_goblins_encounter");
		e->dontAttack = true;
		e->group = UnitGroup::Get("goblins");
		e->locationEventHandler = nullptr;
		e->pos = startLoc->pos;
		e->quest = this;
		e->chance = 10000;
		e->range = 32.f;
		e->text = questMgr->txQuest[219];
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
		return goblinsState >= State::MageTalked
			&& goblinsState < State::KnownLocation
			&& gameLevel->location == startLoc
			&& prog != Progress::TalkedWithInnkeeper;
	}
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Goblins::OnProgress(int d)
{
	if(goblinsState == Quest_Goblins::State::Counting || goblinsState == Quest_Goblins::State::NoblemanLeft)
		days -= d;
}
