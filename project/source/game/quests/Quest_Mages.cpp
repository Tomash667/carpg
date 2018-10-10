#include "Pch.h"
#include "GameCore.h"
#include "Quest_Mages.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "AIController.h"
#include "SoundManager.h"
#include "World.h"
#include "Team.h"

//=================================================================================================
void Quest_Mages::Start()
{
	quest_id = Q_MAGES;
	type = QuestType::Unique;
	// start_loc ustawiane w InitQuests
}

//=================================================================================================
GameDialog* Quest_Mages::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(game->current_dialog->talker->data->id == "q_magowie_uczony")
		return FindDialog("q_mages_scholar");
	else
		return FindDialog("q_mages_golem");
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
			target_loc = W.GetClosestLocation(L_CRYPT, sl.pos);
			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.reset = true;
			tl.spawn = SG_UNDEAD;
			tl.st = 8;
			tl.SetKnown();

			at_level = tl.GetLastLevel();
			item_to_give[0] = Item::Get("q_magowie_kula");
			spawn_item = Quest_Event::Item_InTreasure;

			msgs.push_back(Format(game->txQuest[166], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[167], tl.name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			const Item* item = Item::Get("q_magowie_kula");
			game->current_dialog->talker->AddItem(item, 1, true);
			game->current_dialog->pc->unit->RemoveItem(item, 1);
			QM.quest_mages2->scholar = game->current_dialog->talker;
			QM.quest_mages2->mages_state = Quest_Mages2::State::ScholarWaits;

			GetTargetLocation().active_quest = nullptr;

			game->AddReward(1500);
			OnUpdate(game->txQuest[168]);
			quest_manager.RemoveQuestRumor(R_MAGES);
		}
		break;
	case Progress::EncounteredGolem:
		{
			QM.quest_mages2->OnStart(game->txQuest[169]);
			Quest_Mages2* q = QM.quest_mages2;
			q->mages_state = Quest_Mages2::State::EncounteredGolem;
			quest_manager.quest_rumor[R_MAGES2] = false;
			++quest_manager.quest_rumor_counter;
			q->msgs.push_back(Format(game->txQuest[170], W.GetDate()));
			q->msgs.push_back(game->txQuest[171]);
			W.AddNews(game->txQuest[172]);
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
		QM.quest_mages2->paid = true;
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Mages::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_zaplacono") == 0)
		return QM.quest_mages2->paid;
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
	QM.RegisterSpecialIfHandler(this, "q_magowie_to_miasto");
	QM.RegisterSpecialIfHandler(this, "q_magowie_poinformuj");
	QM.RegisterSpecialIfHandler(this, "q_magowie_kup_miksture");
	QM.RegisterSpecialIfHandler(this, "q_magowie_kup");
	QM.RegisterSpecialIfHandler(this, "q_magowie_nie_ukonczono");
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

	if(game->current_dialog->talker->data->id == "q_magowie_stary")
		return FindDialog("q_mages2_mage");
	else if(game->current_dialog->talker->data->id == "q_magowie_boss")
		return FindDialog("q_mages2_boss");
	else
		return FindDialog("q_mages2_captain");
}

//=================================================================================================
void Quest_Mages2::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::Started:
		// porozmawiano ze stra¿nikiem o golemach, wys³a³ do maga
		{
			start_loc = W.GetCurrentLocationIndex();
			mage_loc = W.GetRandomSettlementIndex(start_loc);

			Location& sl = GetStartLocation();
			Location& ml = *W.GetLocation(mage_loc);

			OnUpdate(Format(game->txQuest[173], sl.name.c_str(), ml.name.c_str(), GetLocationDirName(sl.pos, ml.pos)));

			mages_state = State::TalkedWithCaptain;
		}
		break;
	case Progress::MageWantsBeer:
		// mag chce piwa
		{
			OnUpdate(Format(game->txQuest[174], game->current_dialog->talker->hero->name.c_str()));
		}
		break;
	case Progress::MageWantsVodka:
		// daj piwo, chce wódy
		{
			const Item* piwo = Item::Get("beer");
			game->current_dialog->pc->unit->RemoveItem(piwo, 1);
			game->current_dialog->talker->action = A_NONE;
			game->current_dialog->talker->ConsumeItem(piwo->ToConsumable());
			game->current_dialog->dialog_wait = 2.5f;
			game->current_dialog->can_skip = false;
			OnUpdate(game->txQuest[175]);
		}
		break;
	case Progress::GivenVodka:
		// da³eœ wóde
		{
			const Item* woda = Item::Get("vodka");
			game->current_dialog->pc->unit->RemoveItem(woda, 1);
			game->current_dialog->talker->action = A_NONE;
			game->current_dialog->talker->ConsumeItem(woda->ToConsumable());
			game->current_dialog->dialog_wait = 2.5f;
			game->current_dialog->can_skip = false;
			OnUpdate(game->txQuest[176]);
		}
		break;
	case Progress::GotoTower:
		// idzie za tob¹ do pustej wie¿y
		{
			Location& loc = *W.CreateLocation(L_DUNGEON, Vec2(0, 0), -64.f, MAGE_TOWER, SG_NONE, true, 2);
			loc.st = 1;
			loc.SetKnown();
			target_loc = loc.index;
			Team.AddTeamMember(game->current_dialog->talker, true);
			OnUpdate(Format(game->txQuest[177], game->current_dialog->talker->hero->name.c_str(), GetTargetLocationName(),
				GetLocationDirName(W.GetCurrentLocation()->pos, GetTargetLocation().pos), W.GetCurrentLocation()->name.c_str()));
			mages_state = State::OldMageJoined;
			timer = 0.f;
			scholar = game->current_dialog->talker;
		}
		break;
	case Progress::MageTalkedAboutTower:
		// mag sobie przypomnia³ ¿e to jego wie¿a
		{
			game->current_dialog->talker->auto_talk = AutoTalkMode::No;
			mages_state = State::OldMageRemembers;
			OnUpdate(Format(game->txQuest[178], game->current_dialog->talker->hero->name.c_str(), GetStartLocationName()));
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
		// wywo³ywane z DTF_IF_SPECAL q_magowie_kup
		{
			if(prog != Progress::BoughtPotion)
				OnUpdate(game->txQuest[180]);
			const Item* item = Item::Get("q_magowie_potion");
			game->current_dialog->pc->unit->AddItem2(item, 1u, 0u);
			game->current_dialog->pc->unit->ModGold(-150);
		}
		break;
	case Progress::MageDrinkPotion:
		// wypi³ miksturkê
		{
			const Item* mikstura = Item::Get("q_magowie_potion");
			game->current_dialog->pc->unit->RemoveItem(mikstura, 1);
			game->current_dialog->talker->action = A_NONE;
			game->current_dialog->talker->ConsumeItem(mikstura->ToConsumable());
			game->current_dialog->dialog_wait = 3.f;
			game->current_dialog->can_skip = false;
			mages_state = State::MageCured;
			OnUpdate(game->txQuest[181]);
			GetTargetLocation().active_quest = nullptr;
			Location& loc = *W.CreateLocation(L_DUNGEON, Vec2(0, 0), -64.f, MAGE_TOWER, SG_MAGES_AND_GOLEMS);
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.active_quest = this;
			target_loc = loc.index;
			do
			{
				game->GenerateHeroName(Class::MAGE, false, evil_mage_name);
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
			Unit* u = game->current_dialog->talker;
			Team.RemoveTeamMember(u);
			mages_state = State::MageLeaving;
			good_mage_name = u->hero->name;
			hd_mage.Get(*u->human_data);

			if(W.GetCurrentLocationIndex() == mage_loc)
			{
				// idŸ do karczmy
				u->ai->goto_inn = true;
				u->ai->timer = 0.f;
			}
			else
			{
				// idŸ do startowej lokacji do karczmy
				u->hero->mode = HeroData::Leave;
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
			Unit* u = game->current_dialog->talker;
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
				Team.AddTeamMember(u, true);
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
			W.AddNews(game->txQuest[186]);
		}
		break;
	case Progress::TalkedWithMage:
		// porozmawiano z magiem po
		{
			OnUpdate(Format(game->txQuest[187], game->current_dialog->talker->hero->name.c_str(), evil_mage_name.c_str()));
			// idŸ sobie
			Unit* u = game->current_dialog->talker;
			Team.RemoveTeamMember(u);
			u->hero->mode = HeroData::Leave;
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
			game->AddReward(5000);
			OnUpdate(game->txQuest[188]);
			quest_manager.EndUniqueQuest();
			quest_manager.RemoveQuestRumor(R_MAGES2);
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
		return W.GetLocation(mage_loc)->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, W.GetLocation(mage_loc)->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(W.GetCurrentLocation()->pos, GetTargetLocation().pos);
	else if(str == "name")
		return game->current_dialog->talker->hero->name.c_str();
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
		return target_loc == W.GetCurrentLocationIndex();
	else if(strcmp(msg, "q_magowie_u_siebie") == 0)
		return target_loc == W.GetCurrentLocationIndex();
	else if(strcmp(msg, "q_magowie_czas") == 0)
		return timer >= 30.f;
	else if(strcmp(msg, "q_magowie_to_miasto") == 0)
		return mages_state >= State::TalkedWithCaptain && W.GetCurrentLocationIndex() == start_loc;
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

	if(LOAD_VERSION >= V_0_4)
	{
		f >> mages_state;
		f >> days;
		f >> paid;
		f >> timer;
		f >> scholar;
		f >> evil_mage_name;
		f >> good_mage_name;
		f >> hd_mage;
	}

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

//=================================================================================================
void Quest_Mages2::LoadOld(GameReader& f)
{
	int old_refid, old_refid2, city, where;

	f >> mages_state;
	f >> old_refid;
	f >> old_refid2;
	f >> city;
	f >> days;
	f >> where;
	f >> paid;
	f >> timer;
	f >> scholar;
	f >> evil_mage_name;
	f >> good_mage_name;
	f >> hd_mage;
}
