#include "Pch.h"
#include "Base.h"
#include "Quest_Mages.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "GameGui.h"
#include "AIController.h"

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
			name = game->txQuest[165];
			start_time = game->worldtime;
			state = Quest::Started;

			Location& sl = GetStartLocation();
			target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.reset = true;
			tl.spawn = SG_NIEUMARLI;
			tl.st = 8;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			
			at_level = tl.GetLastLevel();
			item_to_give[0] = FindItem("q_magowie_kula");
			spawn_item = Quest_Event::Item_InTreasure;

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[166], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[167], tl.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			const Item* item = FindItem("q_magowie_kula");
			game->current_dialog->talker->AddItem(item, 1, true);
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->quest_mages2->scholar = game->current_dialog->talker;
			game->quest_mages2->mages_state = Quest_Mages2::State::ScholarWaits;

			GetTargetLocation().active_quest = nullptr;

			game->AddReward(1500);
			msgs.push_back(game->txQuest[168]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			quest_manager.RemoveQuestRumor(P_MAGOWIE);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::EncounteredGolem:
		{
			Quest_Mages2* q = game->quest_mages2;
			q->name = game->txQuest[169];
			q->start_time = game->worldtime;
			q->state = Quest::Started;
			q->mages_state = Quest_Mages2::State::EncounteredGolem;
			q->quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(q);
			RemoveElementTry(quest_manager.unaccepted_quests, (Quest*)q);
			quest_manager.quest_rumor[P_MAGOWIE2] = false;
			++quest_manager.quest_rumor_counter;
			q->msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			q->msgs.push_back(game->txQuest[171]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, q->quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[172]);

			if(game->IsOnline())
				game->Net_AddQuest(q->refid);
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
void Quest_Mages::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_zaplac") == 0)
	{
		if(ctx.pc->unit->gold)
		{
			ctx.talker->gold += ctx.pc->unit->gold;
			ctx.pc->unit->gold = 0;
			if(game->sound_volume)
				game->PlaySound2d(game->sCoins);
			if(!ctx.is_local)
				game->GetPlayerInfo(ctx.pc->id).UpdateGold();
		}
		game->quest_mages2->paid = true;
	}
	else
	{
		assert(0);
	}
}

//=================================================================================================
bool Quest_Mages::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_zaplacono") == 0)
		return game->quest_mages2->paid;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_Mages::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(!done)
	{
		item_to_give[0] = FindItem("q_magowie_kula");
		spawn_item = Quest_Event::Item_InTreasure;
	}
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
			start_loc = game->current_location;
			mage_loc = game->GetRandomSettlement(start_loc);

			Location& sl = GetStartLocation();
			Location& ml = *game->locations[mage_loc];

			msgs.push_back(Format(game->txQuest[173], sl.name.c_str(), ml.name.c_str(), GetLocationDirName(sl.pos, ml.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			mages_state = State::TalkedWithCaptain;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::MageWantsBeer:
		// mag chce piwa
		{
			msgs.push_back(Format(game->txQuest[174], game->current_dialog->talker->hero->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::MageWantsVodka:
		// daj piwo, chce wódy
		{
			const Item* piwo = FindItem("beer");
			game->RemoveItem(*game->current_dialog->pc->unit, piwo, 1);
			game->current_dialog->talker->action = A_NONE;
			game->current_dialog->talker->ConsumeItem(piwo->ToConsumable());
			game->current_dialog->dialog_wait = 2.5f;
			game->current_dialog->can_skip = false;
			msgs.push_back(game->txQuest[175]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GivenVodka:
		// da³eœ wóde
		{
			const Item* woda = FindItem("vodka");
			game->RemoveItem(*game->current_dialog->pc->unit, woda, 1);
			game->current_dialog->talker->action = A_NONE;
			game->current_dialog->talker->ConsumeItem(woda->ToConsumable());
			game->current_dialog->dialog_wait = 2.5f;
			game->current_dialog->can_skip = false;
			msgs.push_back(game->txQuest[176]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GotoTower:
		// idzie za tob¹ do pustej wie¿y
		{
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -64.f, MAGE_TOWER, SG_BRAK, true, 2);
			Location& loc = *game->locations[target_loc];
			loc.st = 1;
			loc.state = LS_KNOWN;
			game->AddTeamMember(game->current_dialog->talker, true);
			msgs.push_back(Format(game->txQuest[177], game->current_dialog->talker->hero->name.c_str(), GetTargetLocationName(), GetLocationDirName(game->location->pos, GetTargetLocation().pos),
				game->location->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			mages_state = State::OldMageJoined;
			timer = 0.f;
			scholar = game->current_dialog->talker;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::MageTalkedAboutTower:
		// mag sobie przypomnia³ ¿e to jego wie¿a
		{
			game->current_dialog->talker->auto_talk = AutoTalkMode::No;
			mages_state = State::OldMageRemembers;
			msgs.push_back(Format(game->txQuest[178], game->current_dialog->talker->hero->name.c_str(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithCaptain:
		// cpt kaza³ pogadaæ z alchemikiem
		{
			mages_state = State::BuyPotion;
			msgs.push_back(game->txQuest[179]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::BoughtPotion:
		// kupno miksturki
		// wywo³ywane z DT_IF_SPECAL q_magowie_kup
		{
			if(prog != Progress::BoughtPotion)
			{
				msgs.push_back(game->txQuest[180]);
				game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
				game->AddGameMsg3(GMS_JOURNAL_UPDATED);

				if(game->IsOnline())
					game->Net_UpdateQuest(refid);
			}
			const Item* item = FindItem("q_magowie_potion");
			game->current_dialog->pc->unit->AddItem(item, 1, false);
			game->current_dialog->pc->unit->gold -= 150;

			if(game->IsOnline() && !game->current_dialog->is_local)
			{
				game->Net_AddItem(game->current_dialog->pc, item, false);
				game->Net_AddedItemMsg(game->current_dialog->pc);
				game->GetPlayerInfo(game->current_dialog->pc).update_flags |= PlayerInfo::UF_GOLD;
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::MageDrinkPotion:
		// wypi³ miksturkê
		{
			const Item* mikstura = FindItem("q_magowie_potion");
			game->RemoveItem(*game->current_dialog->pc->unit, mikstura, 1);
			game->current_dialog->talker->action = A_NONE;
			game->current_dialog->talker->ConsumeItem(mikstura->ToConsumable());
			game->current_dialog->dialog_wait = 3.f;
			game->current_dialog->can_skip = false;
			mages_state = State::MageCured;
			msgs.push_back(game->txQuest[181]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			GetTargetLocation().active_quest = nullptr;
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -64.f, MAGE_TOWER, SG_MAGOWIE_I_GOLEMY);
			Location& loc = GetTargetLocation();
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.active_quest = this;
			do
			{
				game->GenerateHeroName(Class::MAGE, false, evil_mage_name);
			}
			while(good_mage_name == evil_mage_name);
			done = false;
			unit_event_handler = this;
			unit_auto_talk = true;
			at_level = loc.GetLastLevel();
			unit_to_spawn = FindUnitData("q_magowie_boss");
			unit_dont_attack = true;
			unit_to_spawn2 = FindUnitData("golem_iron");
			spawn_2_guard_1 = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::NotRecruitMage:
		// nie zrekrutowa³em maga
		{
			Unit* u = game->current_dialog->talker;
			game->RemoveTeamMember(u);
			mages_state = State::MageLeaving;
			good_mage_name = u->hero->name;
			hd_mage.Get(*u->human_data);

			if(game->current_location == mage_loc)
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
			target.state = LS_KNOWN;

			msgs.push_back(Format(game->txQuest[182], u->hero->name.c_str(), evil_mage_name.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::RecruitMage:
		// zrekrutowa³em maga
		{
			Unit* u = game->current_dialog->talker;
			Location& target = GetTargetLocation();

			if(prog == Progress::MageDrinkPotion)
			{
				target.state = LS_KNOWN;
				msgs.push_back(Format(game->txQuest[183], u->hero->name.c_str(), evil_mage_name.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			}
			else
			{
				msgs.push_back(Format(game->txQuest[184], u->hero->name.c_str()));
				good_mage_name = u->hero->name;
				u->ai->goto_inn = false;
				game->AddTeamMember(u, true);
			}

			if(game->IsOnline())
			{
				if(prog == Progress::MageDrinkPotion)
					game->Net_ChangeLocationState(target_loc, false);
				game->Net_UpdateQuest(refid);
			}

			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			mages_state = State::MageRecruited;
		}
		break;
	case Progress::KilledBoss:
		// zabito maga
		{
			if(mages_state == State::MageRecruited)
				scholar->StartAutoTalk();
			mages_state = State::Completed;
			msgs.push_back(game->txQuest[185]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[186]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithMage:
		// porozmawiano z magiem po
		{
			msgs.push_back(Format(game->txQuest[187], game->current_dialog->talker->hero->name.c_str(), evil_mage_name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// idŸ sobie
			Unit* u = game->current_dialog->talker;
			game->RemoveTeamMember(u);
			u->hero->mode = HeroData::Leave;
			scholar = nullptr;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
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
			msgs.push_back(game->txQuest[188]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			quest_manager.EndUniqueQuest();
			quest_manager.RemoveQuestRumor(P_MAGOWIE2);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
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
		return game->locations[mage_loc]->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[mage_loc]->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(game->location->pos, GetTargetLocation().pos);
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
bool Quest_Mages2::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "q_magowie_u_bossa") == 0)
		return target_loc == game->current_location;
	else if(strcmp(msg, "q_magowie_u_siebie") == 0)
		return game->current_location == target_loc;
	else if(strcmp(msg, "q_magowie_czas") == 0)
		return timer >= 30.f;
	else
	{
		assert(0);
		return false;
	}
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
void Quest_Mages2::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

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
void Quest_Mages2::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

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
		unit_to_spawn = FindUnitData("q_magowie_boss");
		unit_dont_attack = true;
		unit_to_spawn2 = FindUnitData("golem_iron");
		spawn_2_guard_1 = true;
	}
}

//=================================================================================================
void Quest_Mages2::LoadOld(HANDLE file)
{
	int old_refid, old_refid2, city, where;
	GameReader f(file);

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
