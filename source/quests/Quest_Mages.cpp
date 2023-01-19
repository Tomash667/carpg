#include "Pch.h"
#include "Quest_Mages.h"

#include "AIController.h"
#include "Encounter.h"
#include "Game.h"
#include "Journal.h"
#include "Level.h"
#include "NameHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Mages::Start()
{
	type = Q_MAGES;
	category = QuestCategory::Unique;
	startLoc = world->GetRandomSettlement(questMgr->GetUsedCities());
	questMgr->AddQuestRumor(id, Format(questMgr->txRumorQ[4], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Mages' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Mages::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(DialogContext::current->talker->data->id == "qMagesScholar")
		return GameDialog::TryGet("qMagesScholar");
	else
		return GameDialog::TryGet("qMagesGolem");
}

//=================================================================================================
void Quest_Mages::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(questMgr->txQuest[165]);

			targetLoc = world->GetClosestLocation(L_DUNGEON, startLoc->pos, { HERO_CRYPT, MONSTER_CRYPT });
			targetLoc->activeQuest = this;
			targetLoc->reset = true;
			targetLoc->group = UnitGroup::Get("undead");
			targetLoc->st = 8;
			targetLoc->SetKnown();

			atLevel = targetLoc->GetLastLevel();
			itemToGive[0] = Item::Get("qMagesBall");
			spawnItem = Quest_Event::Item_InTreasure;

			msgs.push_back(Format(questMgr->txQuest[166], startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[167], targetLoc->name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			const Item* item = Item::Get("qMagesBall");
			DialogContext::current->talker->AddItem(item, 1, true);
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			questMgr->questMages2->scholar = DialogContext::current->talker;
			questMgr->questMages2->magesState = Quest_Mages2::State::ScholarWaits;

			targetLoc->activeQuest = nullptr;

			team->AddReward(4000, 12000);
			OnUpdate(questMgr->txQuest[168]);
			questMgr->RemoveQuestRumor(id);
		}
		break;
	case Progress::EncounteredGolem:
		{
			questMgr->questMages2->OnStart(questMgr->txQuest[169]);
			Quest_Mages2* q = questMgr->questMages2;
			q->magesState = Quest_Mages2::State::EncounteredGolem;
			questMgr->AddQuestRumor(q->id, questMgr->txRumorQ[5]);
			q->msgs.push_back(Format(questMgr->txQuest[170], world->GetDate()));
			q->msgs.push_back(questMgr->txQuest[171]);
			world->AddNews(questMgr->txQuest[172]);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Mages::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Mages::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "magowie") == 0;
}

//=================================================================================================
bool Quest_Mages::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_zaplac") == 0)
	{
		ctx.talker->gold += ctx.pc->unit->gold;
		ctx.pc->unit->SetGold(0);
		questMgr->questMages2->paid = true;
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Mages::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_zaplacono") == 0)
		return questMgr->questMages2->paid;
	assert(0);
	return false;
}

//=================================================================================================
Quest::LoadResult Quest_Mages::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(!done)
	{
		itemToGive[0] = Item::Get("qMagesBall");
		spawnItem = Quest_Event::Item_InTreasure;
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Mages2::Init()
{
	questMgr->RegisterSpecialIfHandler(this, "q_magowie_to_miasto");
	questMgr->RegisterSpecialIfHandler(this, "q_magowie_poinformuj");
	questMgr->RegisterSpecialIfHandler(this, "q_magowie_kup_miksture");
	questMgr->RegisterSpecialIfHandler(this, "q_magowie_kup");
	questMgr->RegisterSpecialIfHandler(this, "q_magowie_nie_ukonczono");
}

//=================================================================================================
void Quest_Mages2::Start()
{
	category = QuestCategory::Unique;
	type = Q_MAGES2;
	talked = Quest_Mages2::Talked::No;
	magesState = State::None;
	scholar = nullptr;
	paid = false;
}

//=================================================================================================
GameDialog* Quest_Mages2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(DialogContext::current->talker->data->id == "qMagesScholar")
		return GameDialog::TryGet("qMages2Mage");
	else if(DialogContext::current->talker->data->id == "qMagesBoss")
		return GameDialog::TryGet("qMages2Boss");
	else
		return GameDialog::TryGet("qMages2Captain");
}

//=================================================================================================
void Quest_Mages2::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::Started:
		// talked with guard captain, send to mage
		{
			startLoc = world->GetCurrentLocation();
			Location* ml = world->GetRandomSettlement(startLoc);
			mageLoc = ml->index;

			OnUpdate(Format(questMgr->txQuest[173], startLoc->name.c_str(), ml->name.c_str(), GetLocationDirName(startLoc->pos, ml->pos)));

			magesState = State::TalkedWithCaptain;
			team->AddExp(2500);
		}
		break;
	case Progress::MageWantsBeer:
		{
			OnUpdate(Format(questMgr->txQuest[174], DialogContext::current->talker->hero->name.c_str()));
		}
		break;
	case Progress::MageWantsVodka:
		{
			const Item* beer = Item::Get("beer");
			DialogContext::current->pc->unit->RemoveItem(beer, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(beer->ToConsumable());
			DialogContext::current->Wait(2.5f);
			OnUpdate(questMgr->txQuest[175]);
		}
		break;
	case Progress::GivenVodka:
		{
			const Item* vodka = Item::Get("vodka");
			DialogContext::current->pc->unit->RemoveItem(vodka, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(vodka->ToConsumable());
			DialogContext::current->Wait(2.5f);
			OnUpdate(questMgr->txQuest[176]);
		}
		break;
	case Progress::GotoTower:
		{
			Location& loc = *world->CreateLocation(L_DUNGEON, world->GetRandomPlace(), MAGE_TOWER, 2);
			loc.group = UnitGroup::empty;
			loc.st = 1;
			loc.SetKnown();
			targetLoc = &loc;
			team->AddMember(DialogContext::current->talker, HeroType::Free);
			OnUpdate(Format(questMgr->txQuest[177], DialogContext::current->talker->hero->name.c_str(), GetTargetLocationName(),
				GetLocationDirName(world->GetCurrentLocation()->pos, targetLoc->pos), world->GetCurrentLocation()->name.c_str()));
			magesState = State::OldMageJoined;
			timer = 0.f;
			scholar = DialogContext::current->talker;
		}
		break;
	case Progress::MageTalkedAboutTower:
		{
			magesState = State::OldMageRemembers;
			OnUpdate(Format(questMgr->txQuest[178], DialogContext::current->talker->hero->name.c_str(), GetStartLocationName()));
			team->AddExp(1000);
		}
		break;
	case Progress::TalkedWithCaptain:
		{
			magesState = State::BuyPotion;
			OnUpdate(questMgr->txQuest[179]);
		}
		break;
	case Progress::BoughtPotion:
		{
			if(prog != Progress::BoughtPotion)
				OnUpdate(questMgr->txQuest[180]);
			const Item* item = Item::Get("qMagesPotion");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 0u);
			DialogContext::current->pc->unit->ModGold(-150);
		}
		break;
	case Progress::MageDrinkPotion:
		{
			const Item* mikstura = Item::Get("qMagesPotion");
			DialogContext::current->pc->unit->RemoveItem(mikstura, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(mikstura->ToConsumable());
			DialogContext::current->Wait(3.f);
			magesState = State::MageCured;
			OnUpdate(questMgr->txQuest[181]);
			targetLoc->activeQuest = nullptr;
			Location& loc = *world->CreateLocation(L_DUNGEON, world->GetRandomPlace(), MAGE_TOWER);
			loc.group = UnitGroup::Get("magesAndGolems");
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.activeQuest = this;
			targetLoc = &loc;
			do
			{
				NameHelper::GenerateHeroName(Class::TryGet("mage"), false, evilMageName);
			}
			while(goodMageName == evilMageName);
			done = false;
			unitEventHandler = this;
			unitAutoTalk = true;
			atLevel = loc.GetLastLevel();
			unitToSpawn = UnitData::Get("qMagesBoss");
			unitDontAttack = true;
			unitToSpawn2 = UnitData::Get("golemIron");
			spawnGuards = true;
		}
		break;
	case Progress::NotRecruitMage:
		{
			Unit* u = DialogContext::current->talker;
			team->RemoveMember(u);
			magesState = State::MageLeaving;
			goodMageName = u->hero->name;
			hdMage.Get(*u->humanData);

			if(world->GetCurrentLocationIndex() == mageLoc)
				u->OrderGoToInn();
			else
			{
				u->OrderLeave();
				u->eventHandler = this;
			}

			targetLoc->SetKnown();

			OnUpdate(Format(questMgr->txQuest[182], u->hero->name.c_str(), evilMageName.c_str(), targetLoc->name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
		}
		break;
	case Progress::RecruitMage:
		{
			Unit* u = DialogContext::current->talker;

			if(prog == Progress::MageDrinkPotion)
			{
				targetLoc->SetKnown();
				OnUpdate(Format(questMgr->txQuest[183], u->hero->name.c_str(), evilMageName.c_str(), targetLoc->name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			}
			else
			{
				OnUpdate(Format(questMgr->txQuest[184], u->hero->name.c_str()));
				goodMageName = u->hero->name;
				team->AddMember(u, HeroType::Free);
			}

			magesState = State::MageRecruited;
		}
		break;
	case Progress::KilledBoss:
		{
			if(magesState == State::MageRecruited)
				scholar->OrderAutoTalk();
			magesState = State::Completed;
			OnUpdate(questMgr->txQuest[185]);
			world->AddNews(questMgr->txQuest[186]);
			team->AddLearningPoint();
		}
		break;
	case Progress::TalkedWithMage:
		{
			OnUpdate(Format(questMgr->txQuest[187], DialogContext::current->talker->hero->name.c_str(), evilMageName.c_str()));
			Unit* u = DialogContext::current->talker;
			team->RemoveMember(u);
			u->OrderLeave();
			scholar = nullptr;
		}
		break;
	case Progress::Finished:
		{
			targetLoc->activeQuest = nullptr;
			state = Quest::Completed;
			if(scholar)
			{
				scholar->temporary = true;
				scholar = nullptr;
			}
			world->RemoveGlobalEncounter(this);
			team->AddReward(10000, 25000);
			OnUpdate(questMgr->txQuest[188]);
			questMgr->EndUniqueQuest();
			questMgr->RemoveQuestRumor(id);
		}
		break;
	}

	prog = prog2;
}

//=================================================================================================
cstring Quest_Mages2::FormatString(const string& str)
{
	if(str == "start_loc")
		return GetStartLocationName();
	else if(str == "mage_loc")
		return world->GetLocation(mageLoc)->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(startLoc->pos, world->GetLocation(mageLoc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(world->GetCurrentLocation()->pos, targetLoc->pos);
	else if(str == "name")
		return DialogContext::current->talker->hero->name.c_str();
	else if(str == "enemy")
		return evilMageName.c_str();
	else if(str == "dobry")
		return goodMageName.c_str();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Mages2::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "magowie2") == 0;
}

//=================================================================================================
bool Quest_Mages2::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_u_bossa") == 0)
		return targetLoc == world->GetCurrentLocation();
	else if(strcmp(msg, "q_magowie_u_siebie") == 0)
		return targetLoc == world->GetCurrentLocation();
	else if(strcmp(msg, "q_magowie_czas") == 0)
		return timer >= 30.f;
	else if(strcmp(msg, "q_magowie_to_miasto") == 0)
		return magesState >= State::TalkedWithCaptain && world->GetCurrentLocation() == startLoc;
	else if(strcmp(msg, "q_magowie_poinformuj") == 0)
		return magesState == State::EncounteredGolem;
	else if(strcmp(msg, "q_magowie_kup_miksture") == 0)
		return magesState == State::BuyPotion;
	else if(strcmp(msg, "q_magowie_kup") == 0)
	{
		if(ctx.pc->unit->gold >= 150)
		{
			SetProgress(Progress::BoughtPotion);
			return true;
		}
		return false;
	}
	else if(strcmp(msg, "q_magowie_nie_ukonczono") == 0)
		return magesState != State::Completed;
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Mages2::HandleUnitEvent(UnitEventHandler::TYPE eventType, Unit* unit)
{
	if(unit == scholar)
	{
		if(eventType == UnitEventHandler::LEAVE)
		{
			unit->ApplyHumanData(hdMage);
			magesState = State::MageLeft;
			scholar = nullptr;
		}
	}
	else if(unit->data->id == "qMagesBoss" && eventType == UnitEventHandler::DIE && prog != Progress::KilledBoss)
	{
		SetProgress(Progress::KilledBoss);
		unit->eventHandler = nullptr;
	}
}

//=================================================================================================
void Quest_Mages2::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << mageLoc;
	f << talked;
	f << magesState;
	f << days;
	f << paid;
	f << timer;
	f << scholar;
	f << evilMageName;
	f << goodMageName;
	f << hdMage;
}

//=================================================================================================
Quest::LoadResult Quest_Mages2::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> mageLoc;
	f >> talked;
	f >> magesState;
	f >> days;
	f >> paid;
	f >> timer;
	f >> scholar;
	f >> evilMageName;
	f >> goodMageName;
	f >> hdMage;

	if(!done && prog >= Progress::MageDrinkPotion)
	{
		unitEventHandler = this;
		unitAutoTalk = true;
		atLevel = targetLoc->GetLastLevel();
		unitToSpawn = UnitData::Get("qMagesBoss");
		unitDontAttack = true;
		unitToSpawn2 = UnitData::Get("golemIron");
		spawnGuards = true;
	}

	if(magesState >= State::Encounter && magesState < State::Completed)
	{
		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Mages2::OnEncounter);
		globalEnc->chance = 33;
		globalEnc->quest = this;
		globalEnc->text = questMgr->txQuest[215];
		world->AddGlobalEncounter(globalEnc);
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Mages2::Update(float dt)
{
	if(magesState == State::OldMageJoined && gameLevel->location == targetLoc)
	{
		timer += dt;
		if(timer >= 30.f && scholar->GetOrder() != ORDER_AUTO_TALK)
			scholar->OrderAutoTalk();
	}
}

//=================================================================================================
void Quest_Mages2::OnProgress(int d)
{
	if(magesState == State::Counting)
	{
		days -= d;
		if(days <= 0)
		{
			// from now golem can be encountered on road
			magesState = Quest_Mages2::State::Encounter;

			GlobalEncounter* globalEnc = new GlobalEncounter;
			globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Mages2::OnEncounter);
			globalEnc->chance = 33;
			globalEnc->quest = this;
			globalEnc->text = questMgr->txQuest[215];
			world->AddGlobalEncounter(globalEnc);
		}
	}
}

//=================================================================================================
void Quest_Mages2::OnEncounter(EncounterSpawn& spawn)
{
	paid = false;

	int pts = team->GetStPoints();
	if(pts >= 60)
		pts = 60;
	pts = int(Random(0.5f, 0.75f) * pts);
	spawn.count = max(1, pts / 8);
	spawn.level = 8;
	spawn.groupName = "qMagesGolems";
	spawn.dontAttack = true;
	spawn.dialog = GameDialog::TryGet("qMages");
}
