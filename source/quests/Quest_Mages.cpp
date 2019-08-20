#include "Pch.h"
#include "GameCore.h"
#include "Quest_Mages.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "AIController.h"
#include "SoundManager.h"
#include "World.h"
#include "Team.h"
#include "NameHelper.h"

//=================================================================================================
void Quest_Mages::Start()
{
	quest_id = Q_MAGES;
	type = QuestType::Unique;
	quest_mgr->AddQuestRumor(refid, Format(quest_mgr->txRumorQ[4], GetStartLocationName()));
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
			OnStart(game->txQuest[165]);

			Location& sl = GetStartLocation();
			target_loc = world->GetClosestLocation(L_CRYPT, sl.pos);
			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.reset = true;
			tl.group = UnitGroup::Get("undead");
			tl.st = 8;
			tl.SetKnown();

			at_level = tl.GetLastLevel();
			item_to_give[0] = Item::Get("q_magowie_kula");
			spawn_item = Quest_Event::Item_InTreasure;

			msgs.push_back(Format(game->txQuest[166], sl.name.c_str(), world->GetDate()));
			msgs.push_back(Format(game->txQuest[167], tl.name.c_str(), GetTargetLocationDir()));
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

			GetTargetLocation().active_quest = nullptr;

			team->AddReward(4000, 12000);
			OnUpdate(game->txQuest[168]);
			quest_mgr->RemoveQuestRumor(refid);
		}
		break;
	case Progress::EncounteredGolem:
		{
			quest_mgr->quest_mages2->OnStart(game->txQuest[169]);
			Quest_Mages2* q = quest_mgr->quest_mages2;
			q->mages_state = Quest_Mages2::State::EncounteredGolem;
			quest_mgr->AddQuestRumor(q->refid, quest_mgr->txRumorQ[5]);
			q->msgs.push_back(Format(game->txQuest[170], world->GetDate()));
			q->msgs.push_back(game->txQuest[171]);
			world->AddNews(game->txQuest[172]);
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
bool Quest_Mages::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(!done)
	{
		item_to_give[0] = Item::Get("q_magowie_kula");
		spawn_item = Quest_Event::Item_InTreasure;
	}

	return true;
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
	type = QuestType::Unique;
	quest_id = Q_MAGES2;
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
		// porozmawiano ze stra¿nikiem o golemach, wys³a³ do maga
		{
			start_loc = world->GetCurrentLocationIndex();
			mage_loc = world->GetRandomSettlementIndex(start_loc);

			Location& sl = GetStartLocation();
			Location& ml = *world->GetLocation(mage_loc);

			OnUpdate(Format(game->txQuest[173], sl.name.c_str(), ml.name.c_str(), GetLocationDirName(sl.pos, ml.pos)));

			mages_state = State::TalkedWithCaptain;
			team->AddExp(2500);
		}
		break;
	case Progress::MageWantsBeer:
		// mag chce piwa
		{
			OnUpdate(Format(game->txQuest[174], DialogContext::current->talker->hero->name.c_str()));
		}
		break;
	case Progress::MageWantsVodka:
		// daj piwo, chce wódy
		{
			const Item* piwo = Item::Get("beer");
			DialogContext::current->pc->unit->RemoveItem(piwo, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(piwo->ToConsumable());
			DialogContext::current->dialog_wait = 2.5f;
			DialogContext::current->can_skip = false;
			OnUpdate(game->txQuest[175]);
		}
		break;
	case Progress::GivenVodka:
		// da³eœ wóde
		{
			const Item* woda = Item::Get("vodka");
			DialogContext::current->pc->unit->RemoveItem(woda, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(woda->ToConsumable());
			DialogContext::current->dialog_wait = 2.5f;
			DialogContext::current->can_skip = false;
			OnUpdate(game->txQuest[176]);
		}
		break;
	case Progress::GotoTower:
		// idzie za tob¹ do pustej wie¿y
		{
			Location& loc = *world->CreateLocation(L_DUNGEON, Vec2(0, 0), -64.f, MAGE_TOWER, UnitGroup::empty, true, 2);
			loc.st = 1;
			loc.SetKnown();
			target_loc = loc.index;
			team->AddTeamMember(DialogContext::current->talker, true);
			OnUpdate(Format(game->txQuest[177], DialogContext::current->talker->hero->name.c_str(), GetTargetLocationName(),
				GetLocationDirName(world->GetCurrentLocation()->pos, GetTargetLocation().pos), world->GetCurrentLocation()->name.c_str()));
			mages_state = State::OldMageJoined;
			timer = 0.f;
			scholar = DialogContext::current->talker;
		}
		break;
	case Progress::MageTalkedAboutTower:
		// mag sobie przypomnia³ ¿e to jego wie¿a
		{
			DialogContext::current->talker->auto_talk = AutoTalkMode::No;
			mages_state = State::OldMageRemembers;
			OnUpdate(Format(game->txQuest[178], DialogContext::current->talker->hero->name.c_str(), GetStartLocationName()));
			team->AddExp(1000);
		}
		break;
	case Progress::TalkedWithCaptain:
		// cpt kaza³ pogadaæ z alchemikiem
		{
			mages_state = State::BuyPotion;
			OnUpdate(game->txQuest[179]);
		}
		break;
	case Progress::BoughtPotion:
		// kupno miksturki
		// wywo³ywane z DOP_IF_SPECAL q_magowie_kup
		{
			if(prog != Progress::BoughtPotion)
				OnUpdate(game->txQuest[180]);
			const Item* item = Item::Get("q_magowie_potion");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 0u);
			DialogContext::current->pc->unit->ModGold(-150);
		}
		break;
	case Progress::MageDrinkPotion:
		// wypi³ miksturkê
		{
			const Item* mikstura = Item::Get("q_magowie_potion");
			DialogContext::current->pc->unit->RemoveItem(mikstura, 1);
			DialogContext::current->talker->action = A_NONE;
			DialogContext::current->talker->ConsumeItem(mikstura->ToConsumable());
			DialogContext::current->dialog_wait = 3.f;
			DialogContext::current->can_skip = false;
			mages_state = State::MageCured;
			OnUpdate(game->txQuest[181]);
			GetTargetLocation().active_quest = nullptr;
			Location& loc = *world->CreateLocation(L_DUNGEON, Vec2(0, 0), -64.f, MAGE_TOWER, UnitGroup::Get("mages_and_golems"));
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.active_quest = this;
			target_loc = loc.index;
			do
			{
				NameHelper::GenerateHeroName(Class::TryGet("mage"), false, evil_mage_name);
			} while(good_mage_name == evil_mage_name);
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
		// nie zrekrutowa³em maga
		{
			Unit* u = DialogContext::current->talker;
			team->RemoveTeamMember(u);
			mages_state = State::MageLeaving;
			good_mage_name = u->hero->name;
			hd_mage.Get(*u->human_data);

			if(world->GetCurrentLocationIndex() == mage_loc)
			{
				// idŸ do karczmy
				u->ai->goto_inn = true;
				u->ai->timer = 0.f;
			}
			else
			{
				// idŸ do startowej lokacji do karczmy
				u->SetOrder(ORDER_LEAVE);
				u->event_handler = this;
			}

			Location& target = GetTargetLocation();
			target.SetKnown();

			OnUpdate(Format(game->txQuest[182], u->hero->name.c_str(), evil_mage_name.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
		}
		break;
	case Progress::RecruitMage:
		// zrekrutowa³em maga
		{
			Unit* u = DialogContext::current->talker;
			Location& target = GetTargetLocation();

			if(prog == Progress::MageDrinkPotion)
			{
				target.SetKnown();
				OnUpdate(Format(game->txQuest[183], u->hero->name.c_str(), evil_mage_name.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			}
			else
			{
				OnUpdate(Format(game->txQuest[184], u->hero->name.c_str()));
				good_mage_name = u->hero->name;
				u->ai->goto_inn = false;
				team->AddTeamMember(u, true);
			}

			mages_state = State::MageRecruited;
		}
		break;
	case Progress::KilledBoss:
		// zabito maga
		{
			if(mages_state == State::MageRecruited)
				scholar->StartAutoTalk();
			mages_state = State::Completed;
			OnUpdate(game->txQuest[185]);
			world->AddNews(game->txQuest[186]);
			team->AddLearningPoint();
		}
		break;
	case Progress::TalkedWithMage:
		// porozmawiano z magiem po
		{
			OnUpdate(Format(game->txQuest[187], DialogContext::current->talker->hero->name.c_str(), evil_mage_name.c_str()));
			// idŸ sobie
			Unit* u = DialogContext::current->talker;
			team->RemoveTeamMember(u);
			u->SetOrder(ORDER_LEAVE);
			scholar = nullptr;
		}
		break;
	case Progress::Finished:
		// odebrano nagrodê
		{
			GetTargetLocation().active_quest = nullptr;
			state = Quest::Completed;
			if(scholar)
			{
				scholar->temporary = true;
				scholar = nullptr;
			}
			team->AddReward(10000, 25000);
			OnUpdate(game->txQuest[188]);
			quest_mgr->EndUniqueQuest();
			quest_mgr->RemoveQuestRumor(refid);
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
		return GetLocationDirName(GetStartLocation().pos, world->GetLocation(mage_loc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(world->GetCurrentLocation()->pos, GetTargetLocation().pos);
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
		return target_loc == world->GetCurrentLocationIndex();
	else if(strcmp(msg, "q_magowie_u_siebie") == 0)
		return target_loc == world->GetCurrentLocationIndex();
	else if(strcmp(msg, "q_magowie_czas") == 0)
		return timer >= 30.f;
	else if(strcmp(msg, "q_magowie_to_miasto") == 0)
		return mages_state >= State::TalkedWithCaptain && world->GetCurrentLocationIndex() == start_loc;
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
bool Quest_Mages2::Load(GameReader& f)
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
		at_level = GetTargetLocation().GetLastLevel();
		unit_to_spawn = UnitData::Get("q_magowie_boss");
		unit_dont_attack = true;
		unit_to_spawn2 = UnitData::Get("golem_iron");
		spawn_2_guard_1 = true;
	}

	return true;
}
