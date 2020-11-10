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
	startLoc = world->GetRandomSettlement(quest_mgr->GetUsedCities());
	quest_mgr->AddQuestRumor(id, Format(quest_mgr->txRumorQ[4], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Mages' - %s.", GetStartLocationName());
}

//=================================================================================================
GameDialog* Quest_Mages::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(DialogContext::current->talker->data->id == "q_magowie_uczony")
		return GameDialog::TryGet("q_mages_scholar");
	else
		return GameDialog::TryGet("q_mages_golem");
}

//=================================================================================================
void Quest_Mages::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(quest_mgr->txQuest[165]);

			targetLoc = world->GetClosestLocation(L_DUNGEON, startLoc->pos, { HERO_CRYPT, MONSTER_CRYPT });
			targetLoc->active_quest = this;
			targetLoc->reset = true;
			targetLoc->group = UnitGroup::Get("undead");
			targetLoc->st = 8;
			targetLoc->SetKnown();

			at_level = targetLoc->GetLastLevel();
			item_to_give[0] = Item::Get("q_magowie_kula");
			spawn_item = Quest_Event::Item_InTreasure;

			msgs.push_back(Format(quest_mgr->txQuest[166], startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(quest_mgr->txQuest[167], targetLoc->name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			const Item* item = Item::Get("q_magowie_kula");
			DialogContext::current->talker->AddItem(item, 1, true);
			DialogContext::current->pc->unit->RemoveItem(item, 1);
			quest_mgr->quest_mages2->scholar = DialogContext::current->talker;
			quest_mgr->quest_mages2->mages_state = Quest_Mages2::State::ScholarWaits;

			targetLoc->active_quest = nullptr;

			team->AddReward(4000, 12000);
			OnUpdate(quest_mgr->txQuest[168]);
			quest_mgr->RemoveQuestRumor(id);
		}
		break;
	case Progress::EncounteredGolem:
		{
			quest_mgr->quest_mages2->OnStart(quest_mgr->txQuest[169]);
			Quest_Mages2* q = quest_mgr->quest_mages2;
			q->mages_state = Quest_Mages2::State::EncounteredGolem;
			quest_mgr->AddQuestRumor(q->id, quest_mgr->txRumorQ[5]);
			q->msgs.push_back(Format(quest_mgr->txQuest[170], world->GetDate()));
			q->msgs.push_back(quest_mgr->txQuest[171]);
			world->AddNews(quest_mgr->txQuest[172]);
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
		quest_mgr->quest_mages2->paid = true;
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Mages::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_zaplacono") == 0)
		return quest_mgr->quest_mages2->paid;
	assert(0);
	return false;
}

//=================================================================================================
Quest::LoadResult Quest_Mages::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(!done)
	{
		item_to_give[0] = Item::Get("q_magowie_kula");
		spawn_item = Quest_Event::Item_InTreasure;
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Mages2::Init()
{
	quest_mgr->RegisterSpecialIfHandler(this, "q_magowie_to_miasto");
	quest_mgr->RegisterSpecialIfHandler(this, "q_magowie_poinformuj");
	quest_mgr->RegisterSpecialIfHandler(this, "q_magowie_kup_miksture");
	quest_mgr->RegisterSpecialIfHandler(this, "q_magowie_kup");
	quest_mgr->RegisterSpecialIfHandler(this, "q_magowie_nie_ukonczono");
}

//=================================================================================================
void Quest_Mages2::Start()
{
	category = QuestCategory::Unique;
	type = Q_MAGES2;
	talked = Quest_Mages2::Talked::No;
	mages_state = State::None;
	scholar = nullptr;
	paid = false;
}

//=================================================================================================
GameDialog* Quest_Mages2::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(DialogContext::current->talker->data->id == "q_magowie_stary")
		return GameDialog::TryGet("q_mages2_mage");
	else if(DialogContext::current->talker->data->id == "q_magowie_boss")
		return GameDialog::TryGet("q_mages2_boss");
	else
		return GameDialog::TryGet("q_mages2_captain");
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
			mage_loc = ml->index;

			OnUpdate(Format(quest_mgr->txQuest[173], startLoc->name.c_str(), ml->name.c_str(), GetLocationDirName(startLoc->pos, ml->pos)));

			mages_state = State::TalkedWithCaptain;
			team->AddExp(2500);
		}
		break;
	case Progress::MageWantsBeer:
		{
			OnUpdate(Format(quest_mgr->txQuest[174], DialogContext::current->talker->hero->name.c_str()));
		}
		break;
	case Progress::MageWantsVodka:
		{
			const Item* beer = Item::Get("beer");
			DialogContext::current->pc->unit->RemoveItem(beer, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(beer->ToConsumable());
			DialogContext::current->dialog_wait = 2.5f;
			DialogContext::current->can_skip = false;
			OnUpdate(quest_mgr->txQuest[175]);
		}
		break;
	case Progress::GivenVodka:
		{
			const Item* vodka = Item::Get("vodka");
			DialogContext::current->pc->unit->RemoveItem(vodka, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(vodka->ToConsumable());
			DialogContext::current->dialog_wait = 2.5f;
			DialogContext::current->can_skip = false;
			OnUpdate(quest_mgr->txQuest[176]);
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
			OnUpdate(Format(quest_mgr->txQuest[177], DialogContext::current->talker->hero->name.c_str(), GetTargetLocationName(),
				GetLocationDirName(world->GetCurrentLocation()->pos, targetLoc->pos), world->GetCurrentLocation()->name.c_str()));
			mages_state = State::OldMageJoined;
			timer = 0.f;
			scholar = DialogContext::current->talker;
		}
		break;
	case Progress::MageTalkedAboutTower:
		{
			mages_state = State::OldMageRemembers;
			OnUpdate(Format(quest_mgr->txQuest[178], DialogContext::current->talker->hero->name.c_str(), GetStartLocationName()));
			team->AddExp(1000);
		}
		break;
	case Progress::TalkedWithCaptain:
		{
			mages_state = State::BuyPotion;
			OnUpdate(quest_mgr->txQuest[179]);
		}
		break;
	case Progress::BoughtPotion:
		{
			if(prog != Progress::BoughtPotion)
				OnUpdate(quest_mgr->txQuest[180]);
			const Item* item = Item::Get("q_magowie_potion");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 0u);
			DialogContext::current->pc->unit->ModGold(-150);
		}
		break;
	case Progress::MageDrinkPotion:
		{
			const Item* mikstura = Item::Get("q_magowie_potion");
			DialogContext::current->pc->unit->RemoveItem(mikstura, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(mikstura->ToConsumable());
			DialogContext::current->dialog_wait = 3.f;
			DialogContext::current->can_skip = false;
			mages_state = State::MageCured;
			OnUpdate(quest_mgr->txQuest[181]);
			targetLoc->active_quest = nullptr;
			Location& loc = *world->CreateLocation(L_DUNGEON, world->GetRandomPlace(), MAGE_TOWER);
			loc.group = UnitGroup::Get("mages_and_golems");
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.active_quest = this;
			targetLoc = &loc;
			do
			{
				NameHelper::GenerateHeroName(Class::TryGet("mage"), false, evil_mage_name);
			}
			while(good_mage_name == evil_mage_name);
			done = false;
			unit_event_handler = this;
			unit_auto_talk = true;
			at_level = loc.GetLastLevel();
			unit_to_spawn = UnitData::Get("q_magowie_boss");
			unit_dont_attack = true;
			unit_to_spawn2 = UnitData::Get("golem_iron");
			spawn_2_guard_1 = true;
		}
		break;
	case Progress::NotRecruitMage:
		{
			Unit* u = DialogContext::current->talker;
			team->RemoveMember(u);
			mages_state = State::MageLeaving;
			good_mage_name = u->hero->name;
			hd_mage.Get(*u->human_data);

			if(world->GetCurrentLocationIndex() == mage_loc)
				u->OrderGoToInn();
			else
			{
				u->OrderLeave();
				u->event_handler = this;
			}

			targetLoc->SetKnown();

			OnUpdate(Format(quest_mgr->txQuest[182], u->hero->name.c_str(), evil_mage_name.c_str(), targetLoc->name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
		}
		break;
	case Progress::RecruitMage:
		{
			Unit* u = DialogContext::current->talker;

			if(prog == Progress::MageDrinkPotion)
			{
				targetLoc->SetKnown();
				OnUpdate(Format(quest_mgr->txQuest[183], u->hero->name.c_str(), evil_mage_name.c_str(), targetLoc->name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			}
			else
			{
				OnUpdate(Format(quest_mgr->txQuest[184], u->hero->name.c_str()));
				good_mage_name = u->hero->name;
				team->AddMember(u, HeroType::Free);
			}

			mages_state = State::MageRecruited;
		}
		break;
	case Progress::KilledBoss:
		{
			if(mages_state == State::MageRecruited)
				scholar->OrderAutoTalk();
			mages_state = State::Completed;
			OnUpdate(quest_mgr->txQuest[185]);
			world->AddNews(quest_mgr->txQuest[186]);
			team->AddLearningPoint();
		}
		break;
	case Progress::TalkedWithMage:
		{
			OnUpdate(Format(quest_mgr->txQuest[187], DialogContext::current->talker->hero->name.c_str(), evil_mage_name.c_str()));
			Unit* u = DialogContext::current->talker;
			team->RemoveMember(u);
			u->OrderLeave();
			scholar = nullptr;
		}
		break;
	case Progress::Finished:
		{
			targetLoc->active_quest = nullptr;
			state = Quest::Completed;
			if(scholar)
			{
				scholar->temporary = true;
				scholar = nullptr;
			}
			world->RemoveGlobalEncounter(this);
			team->AddReward(10000, 25000);
			OnUpdate(quest_mgr->txQuest[188]);
			quest_mgr->EndUniqueQuest();
			quest_mgr->RemoveQuestRumor(id);
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
		return world->GetLocation(mage_loc)->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(startLoc->pos, world->GetLocation(mage_loc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(world->GetCurrentLocation()->pos, targetLoc->pos);
	else if(str == "name")
		return DialogContext::current->talker->hero->name.c_str();
	else if(str == "enemy")
		return evil_mage_name.c_str();
	else if(str == "dobry")
		return good_mage_name.c_str();
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
		return mages_state >= State::TalkedWithCaptain && world->GetCurrentLocation() == startLoc;
	else if(strcmp(msg, "q_magowie_poinformuj") == 0)
		return mages_state == State::EncounteredGolem;
	else if(strcmp(msg, "q_magowie_kup_miksture") == 0)
		return mages_state == State::BuyPotion;
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
		return mages_state != State::Completed;
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Mages2::HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit)
{
	if(unit == scholar)
	{
		if(event_type == UnitEventHandler::LEAVE)
		{
			unit->ApplyHumanData(hd_mage);
			mages_state = State::MageLeft;
			scholar = nullptr;
		}
	}
	else if(unit->data->id == "q_magowie_boss" && event_type == UnitEventHandler::DIE && prog != Progress::KilledBoss)
	{
		SetProgress(Progress::KilledBoss);
		unit->event_handler = nullptr;
	}
}

//=================================================================================================
void Quest_Mages2::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << mage_loc;
	f << talked;
	f << mages_state;
	f << days;
	f << paid;
	f << timer;
	f << scholar;
	f << evil_mage_name;
	f << good_mage_name;
	f << hd_mage;
}

//=================================================================================================
Quest::LoadResult Quest_Mages2::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> mage_loc;
	f >> talked;
	f >> mages_state;
	f >> days;
	f >> paid;
	f >> timer;
	f >> scholar;
	f >> evil_mage_name;
	f >> good_mage_name;
	f >> hd_mage;

	if(!done && prog >= Progress::MageDrinkPotion)
	{
		unit_event_handler = this;
		unit_auto_talk = true;
		at_level = targetLoc->GetLastLevel();
		unit_to_spawn = UnitData::Get("q_magowie_boss");
		unit_dont_attack = true;
		unit_to_spawn2 = UnitData::Get("golem_iron");
		spawn_2_guard_1 = true;
	}

	if(mages_state >= State::Encounter && mages_state < State::Completed)
	{
		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Mages2::OnEncounter);
		globalEnc->chance = 33;
		globalEnc->quest = this;
		globalEnc->text = quest_mgr->txQuest[215];
		world->AddGlobalEncounter(globalEnc);
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Mages2::Update(float dt)
{
	if(mages_state == State::OldMageJoined && game_level->location == targetLoc)
	{
		timer += dt;
		if(timer >= 30.f && scholar->GetOrder() != ORDER_AUTO_TALK)
			scholar->OrderAutoTalk();
	}
}

//=================================================================================================
void Quest_Mages2::OnProgress(int d)
{
	if(mages_state == State::Counting)
	{
		days -= d;
		if(days <= 0)
		{
			// from now golem can be encountered on road
			mages_state = Quest_Mages2::State::Encounter;

			GlobalEncounter* globalEnc = new GlobalEncounter;
			globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Mages2::OnEncounter);
			globalEnc->chance = 33;
			globalEnc->quest = this;
			globalEnc->text = quest_mgr->txQuest[215];
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
	spawn.group_name = "q_magowie_golems";
	spawn.dont_attack = true;
	spawn.dialog = GameDialog::TryGet("q_mages");
}
