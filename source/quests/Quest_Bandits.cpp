#include "Pch.h"
#include "Quest_Bandits.h"

#include "AIController.h"
#include "City.h"
#include "Encounter.h"
#include "Game.h"
#include "GameResources.h"
#include "Journal.h"
#include "Level.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Bandits::Init()
{
	questMgr->RegisterSpecialIfHandler(this, "q_bandyci_straznikow_daj");
}

//=================================================================================================
void Quest_Bandits::Start()
{
	type = Q_BANDITS;
	category = QuestCategory::Unique;
	startLoc = world->GetRandomSettlement(questMgr->GetUsedCities(), CITY);
	enc = -1;
	otherLoc = -1;
	campLoc = -1;
	getLetter = false;
	banditsState = State::None;
	timer = 0.f;
	agent = nullptr;
	questMgr->AddQuestRumor(id, Format(questMgr->txRumorQ[3], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Bandits' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Bandits::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	const string& id = DialogContext::current->talker->data->id;
	cstring dialogId;

	if(id == "mistrz_agentow")
		dialogId = "q_bandits_master";
	else if(id == "guard_captain")
		dialogId = "q_bandits_captain";
	else if(id == "guard_q_bandyci")
		dialogId = "q_bandits_guard";
	else if(id == "agent")
		dialogId = "q_bandits_agent";
	else if(id == "q_bandyci_szef")
		dialogId = "q_bandits_boss";
	else
		dialogId = "q_bandits_encounter";

	return GameDialog::TryGet(dialogId);
}

//=================================================================================================
void WarpToThroneBanditBoss()
{
	// search for boss
	UnitData* ud = UnitData::Get("q_bandyci_szef");
	Unit* u = gameLevel->localPart->FindUnit(ud);
	assert(u);

	// search for throne
	Usable* use = gameLevel->localPart->FindUsable(BaseUsable::Get("throne"));
	assert(use);

	// warp boss to throne
	gameLevel->WarpUnit(*u, use->pos);
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
			Location& other = *world->GetLocation(otherLoc);
			Encounter* e = world->AddEncounter(enc);
			e->dialog = GameDialog::TryGet("q_bandits");
			e->dontAttack = true;
			e->group = UnitGroup::Get("bandits");
			e->locationEventHandler = nullptr;
			e->pos = (startLoc->pos + other.pos) / 2;
			e->quest = this;
			e->chance = 60;
			e->text = questMgr->txQuest[11];
			e->timed = false;
			e->range = 72;
			e->st = 8;
			OnUpdate(questMgr->txQuest[152]);
		}
		else
		{
			OnStart(questMgr->txQuest[153]);
			questMgr->RemoveQuestRumor(id);

			const Item* item = Item::Get("q_bandyci_paczka");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
			Location* other = world->GetRandomSettlement(startLoc);
			otherLoc = other->index;
			Encounter* e = world->AddEncounter(enc);
			e->dialog = GameDialog::TryGet("q_bandits");
			e->dontAttack = true;
			e->group = UnitGroup::Get("bandits");
			e->locationEventHandler = nullptr;
			e->pos = (startLoc->pos + other->pos) / 2;
			e->quest = this;
			e->chance = 60;
			e->text = questMgr->txQuest[11];
			e->timed = false;
			e->range = 72;
			e->st = 8;
			msgs.push_back(Format(questMgr->txQuest[154], startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[155], startLoc->name.c_str(), other->name.c_str(), GetLocationDirName(startLoc->pos, other->pos)));
			world->AddNews(Format(questMgr->txQuest[156], GetStartLocationName()));
		}
		break;
	case Progress::FoundBandits:
		// podczas rozmowy z bandytami, 66% szansy na znalezienie przy nich listu za 1 razem
		{
			if(getLetter || Rand() % 3 != 0)
			{
				const Item* item = Item::Get("q_bandyci_list");
				gameRes->PreloadItem(item);
				DialogContext::current->talker->AddItem(item, 1, true);
			}
			getLetter = true;
			OnUpdate(questMgr->txQuest[157]);
			world->RemoveEncounter(enc);
			enc = -1;
		}
		break;
	case Progress::TalkAboutLetter:
		break;
	case Progress::NeedTalkWithCaptain:
		// info o obozie
		{
			Vec2 pos = world->FindPlace(startLoc->pos, 64.f);
			targetLoc = world->CreateCamp(pos, UnitGroup::Get("bandits"));
			targetLoc->st = 10;
			targetLoc->state = LS_HIDDEN;
			targetLoc->activeQuest = this;
			OnUpdate(questMgr->txQuest[158]);
			campLoc = targetLoc->index;
			locationEventHandler = this;
			DialogContext::current->pc->unit->RemoveItem(Item::Get("q_bandyci_list"), 1);
			team->AddExp(5000);
		}
		break;
	case Progress::NeedClearCamp:
		// pozycja obozu
		{
			Location& camp = *world->GetLocation(campLoc);
			OnUpdate(Format(questMgr->txQuest[159], GetLocationDirName(startLoc->pos, camp.pos)));
			camp.SetKnown();
			banditsState = State::GenerateGuards;
		}
		break;
	case Progress::KilledBandits:
		{
			banditsState = State::Counting;
			timer = 7.5f;

			// change ai of following guards
			UnitData* ud = UnitData::Get("guard_q_bandyci");
			for(Unit* unit : gameLevel->localPart->units)
			{
				if(unit->data == ud)
				{
					unit->assist = false;
					unit->ai->changeAiMode = true;
				}
			}

			world->AddNews(Format(questMgr->txQuest[160], GetStartLocationName()));
		}
		break;
	case Progress::TalkedWithAgent:
		// talked with agent, he told where is bandit secret hideout is, now he is leaving
		{
			banditsState = State::AgentTalked;
			DialogContext::current->talker->OrderLeave();
			DialogContext::current->talker->eventHandler = this;
			const Vec2 pos = world->FindPlace(startLoc->pos, 64.f);
			Location& target = *world->CreateLocation(L_DUNGEON, pos, THRONE_VAULT);
			target.group = UnitGroup::Get("bandits");
			target.activeQuest = this;
			target.SetKnown();
			target.st = 12;
			targetLoc = &target;
			world->GetLocation(campLoc)->activeQuest = nullptr;
			OnUpdate(Format(questMgr->txQuest[161], target.name.c_str(), GetLocationDirName(startLoc->pos, target.pos)));
			done = false;
			atLevel = 1;
			unitToSpawn = UnitData::Get("q_bandyci_szef");
			unitSpawnRoom = RoomTarget::Throne;
			unitDontAttack = true;
			locationEventHandler = nullptr;
			unitEventHandler = this;
			unitAutoTalk = true;
			callback = WarpToThroneBanditBoss;
			team->AddExp(7500);
		}
		break;
	case Progress::KilledBoss:
		// zabito szefa
		{
			campLoc = -1;
			OnUpdate(Format(questMgr->txQuest[162], GetStartLocationName()));
			world->AddNews(questMgr->txQuest[163]);
			team->AddLearningPoint();
		}
		break;
	case Progress::Finished:
		// ukoñczono
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[164]);
			// ustaw arto na temporary ¿eby sobie poszed³
			DialogContext::current->talker->temporary = true;
			team->AddReward(10000, 25000);
			questMgr->EndUniqueQuest();
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
		return prog == Progress::NeedTalkWithCaptain && world->GetCurrentLocation() == startLoc;
	assert(0);
	return false;
}

//=================================================================================================
cstring Quest_Bandits::FormatString(const string& str)
{
	if(str == "start_loc")
		return GetStartLocationName();
	else if(str == "other_loc")
		return world->GetLocation(otherLoc)->name.c_str();
	else if(str == "other_dir")
		return GetLocationDirName(startLoc->pos, world->GetLocation(otherLoc)->pos);
	else if(str == "camp_dir")
		return GetLocationDirName(startLoc->pos, world->GetLocation(campLoc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir_camp")
		return GetLocationDirName(world->GetLocation(campLoc)->pos, targetLoc->pos);
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
			banditsState = State::AgentLeft;
		}
	}
	else if(event == UnitEventHandler::DIE)
	{
		if(prog == Progress::TalkedWithAgent)
		{
			SetProgress(Progress::KilledBoss);
			unit->eventHandler = nullptr;
		}
	}
}

//=================================================================================================
void Quest_Bandits::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << enc;
	f << otherLoc;
	f << campLoc;
	f << getLetter;
	f << banditsState;
	f << timer;
	f << agent;
}

//=================================================================================================
Quest::LoadResult Quest_Bandits::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> enc;
	f >> otherLoc;
	f >> campLoc;
	f >> getLetter;
	f >> banditsState;
	f >> timer;
	f >> agent;

	if(enc != -1)
	{
		Encounter* e = world->RecreateEncounter(enc);
		e->dialog = GameDialog::TryGet("q_bandits");
		e->dontAttack = true;
		e->group = UnitGroup::Get("bandits");
		e->locationEventHandler = nullptr;
		e->pos = (startLoc->pos + world->GetLocation(otherLoc)->pos) / 2;
		e->quest = this;
		e->chance = 60;
		e->text = questMgr->txQuest[11];
		e->timed = false;
		e->range = 72;
		e->st = 8;
	}

	if(prog == Progress::NeedTalkWithCaptain || prog == Progress::NeedClearCamp)
		locationEventHandler = this;
	else
		locationEventHandler = nullptr;

	if(prog == Progress::TalkedWithAgent && !done)
	{
		unitToSpawn = UnitData::Get("q_bandyci_szef");
		unitSpawnRoom = RoomTarget::Throne;
		atLevel = 1;
		unitDontAttack = true;
		locationEventHandler = nullptr;
		unitEventHandler = this;
		unitAutoTalk = true;
		callback = WarpToThroneBanditBoss;
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Bandits::Update(float dt)
{
	if(banditsState == State::Counting)
	{
		timer -= dt;
		if(timer <= 0.f)
		{
			// spawn agent
			agent = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("agent"), &team->leader->pos, -2, 2.f);
			if(agent)
			{
				banditsState = State::AgentCome;
				agent->OrderAutoTalk(true);
			}
		}
	}
}
