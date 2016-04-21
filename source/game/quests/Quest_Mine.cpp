#include "Pch.h"
#include "Base.h"
#include "Quest_Mine.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"

//=================================================================================================
void Quest_Mine::Start()
{
	quest_id = Q_MINE;
	type = Type::Unique;
	dungeon_loc = -2;
	mine_state = State::None;
	mine_state2 = State2::None;
	mine_state3 = State3::None;
	messenger = nullptr;
	days = 0;
	days_required = 0;
	days_gold = 0;
}

//=================================================================================================
GameDialog* Quest_Mine::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(game->current_dialog->talker->data->id == "inwestor")
			return FindDialog("q_mine_investor");
		else if(game->current_dialog->talker->data->id == "poslaniec_kopalnia")
		{
			if(prog == Quest_Mine::Progress::SelectedShares)
				return FindDialog("q_mine_messenger");
			else if(prog == Quest_Mine::Progress::GotFirstGold || prog == Quest_Mine::Progress::SelectedGold)
			{
				if(days >= days_required)
					return FindDialog("q_mine_messenger2");
				else
					return FindDialog("messenger_talked");
			}
			else if(prog == Quest_Mine::Progress::Invested)
			{
				if(days >= days_required)
					return FindDialog("q_mine_messenger3");
				else
					return FindDialog("messenger_talked");
			}
			else if(prog == Quest_Mine::Progress::UpgradedMine)
			{
				if(days >= days_required)
					return FindDialog("q_mine_messenger4");
				else
					return FindDialog("messenger_talked");
			}
			else
				return FindDialog("messenger_talked");
		}
		else
			return FindDialog("q_mine_boss");
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Mine::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			name = game->txQuest[131];

			location_event_handler = this;

			Location& sl = GetStartLocation();
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			else if(tl.state >= LS_ENTERED)
				tl.reset = true;
			tl.st = 10;

			InitSub();

			QM.AcceptQuest(this);

			msgs.push_back(Format(game->txQuest[132], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[133], tl.name.c_str(), GetTargetLocationDir()));
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
	case Progress::ClearedLocation:
		{
			msgs.push_back(game->txQuest[134]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::SelectedShares:
		{
			msgs.push_back(game->txQuest[135]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			mine_state = State::Shares;
			mine_state2 = State2::InBuild;
			days = 0;
			days_required = random(30, 45);
			if(!game->quest_rumor[P_KOPALNIA])
			{
				game->quest_rumor[P_KOPALNIA] = true;
				--game->quest_rumor_counter;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GotFirstGold:
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[136]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(500);
			mine_state2 = State2::Built;
			days -= days_required;
			days_required = random(60, 90);
			if(days >= days_required)
				days = days_required - 1;
			days_gold = 0;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::SelectedGold:
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[137]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(3000);
			mine_state2 = State2::InBuild;
			days = 0;
			days_required = random(30, 45);
			if(!game->quest_rumor[P_KOPALNIA])
			{
				game->quest_rumor[P_KOPALNIA] = true;
				--game->quest_rumor_counter;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::NeedTalk:
		{
			state = Quest::Started;
			msgs.push_back(game->txQuest[138]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			mine_state2 = State2::CanExpand;
			game->AddNews(Format(game->txQuest[139], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Talked:
		{
			msgs.push_back(Format(game->txQuest[140], mine_state == State::Shares ? 10000 : 12000));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::NotInvested:
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[141]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			QM.EndUniqueQuest();

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Invested:
		{
			if(mine_state == State::Shares)
				game->current_dialog->pc->unit->gold -= 10000;
			else
				game->current_dialog->pc->unit->gold -= 12000;
			msgs.push_back(game->txQuest[142]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			mine_state2 = State2::InExpand;
			days = 0;
			days_required = random(30, 45);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
					game->GetPlayerInfo(game->current_dialog->pc).update_flags |= PlayerInfo::UF_GOLD;
			}
		}
		break;
	case Progress::UpgradedMine:
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[143]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddReward(1000);
			mine_state = State::BigShares;
			mine_state2 = State2::Expanded;
			days -= days_required;
			days_required = random(60, 90);
			if(days >= days_required)
				days = days_required - 1;
			days_gold = 0;
			game->AddNews(Format(game->txQuest[144], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::InfoAboutPortal:
		{
			state = Quest::Started;
			msgs.push_back(game->txQuest[145]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			mine_state2 = State2::FoundPortal;
			game->AddNews(Format(game->txQuest[146], GetTargetLocationName()));

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithMiner:
		{
			msgs.push_back(game->txQuest[147]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			const Item* item = FindItem("key_kopalnia");
			game->current_dialog->pc->unit->AddItem(item, 1, true);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				if(!game->current_dialog->is_local)
				{
					game->Net_AddItem(game->current_dialog->pc, item, true);
					game->Net_AddedItemMsg(game->current_dialog->pc);
				}
				else
					game->AddGameMsg3(GMS_ADDED_ITEM);
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			msgs.push_back(game->txQuest[148]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			QM.EndUniqueQuest();
			game->AddNews(game->txQuest[149]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Mine::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "burmistrzem")
		return GetStartLocation().type == L_CITY ? game->txQuest[150] : game->txQuest[151];
	else if(str == "zloto")
		return Format("%d", mine_state == State::Shares ? 10000 : 12000);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Mine::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "kopalnia") == 0;
}

//=================================================================================================
bool Quest_Mine::IfSpecial(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "udzialy_w_kopalni") == 0)
		return mine_state == State::Shares;
	else if(strcmp(msg, "have_10000") == 0)
		return ctx.pc->unit->gold >= 10000;
	else if(strcmp(msg, "have_12000") == 0)
		return ctx.pc->unit->gold >= 12000;
	else
	{
		assert(0);
		return false;
	}
}

//=================================================================================================
void Quest_Mine::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::Started && event == LocationEventHandler::CLEARED)
		SetProgress(Progress::ClearedLocation);
}

//=================================================================================================
void Quest_Mine::HandleChestEvent(ChestEventHandler::Event event)
{
	if(prog == Progress::TalkedWithMiner && event == ChestEventHandler::Opened)
		SetProgress(Progress::Finished);
}

//=================================================================================================
void Quest_Mine::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	GameWriter f(file);

	f << sub.done;
	f << dungeon_loc;
	f << mine_state;
	f << mine_state2;
	f << mine_state3;
	f << days;
	f << days_required;
	f << days_gold;
	f << messenger;
}

//=================================================================================================
void Quest_Mine::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	GameReader f(file);

	f >> sub.done;
	f >> dungeon_loc;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> mine_state;
		f >> mine_state2;
		f >> mine_state3;
		f >> days;
		f >> days_required;
		f >> days_gold;
		f >> messenger;
	}

	location_event_handler = this;
	InitSub();
}

//=================================================================================================
void Quest_Mine::LoadOld(HANDLE file)
{
	GameReader f(file);
	int city, cave;

	f >> mine_state;
	f >> mine_state2;
	f >> mine_state3;
	f >> city;
	f >> cave;
	f >> refid;
	f >> days;
	f >> days_required;
	f >> days_gold;
	f >> messenger;
}

//=================================================================================================
void Quest_Mine::InitSub()
{
	if(sub.done)
		return;

	ItemListResult result = FindItemList("ancient_armory_armors");
	result.lis->Get(3, sub.item_to_give);
	sub.item_to_give[3] = FindItem("al_angelskin");
	sub.spawn_item = Quest_Event::Item_InChest;
	sub.target_loc = dungeon_loc;
	sub.at_level = 0;
	sub.chest_event_handler = this;
	next_event = &sub;
}

//=================================================================================================
int Quest_Mine::GetIncome(int days)
{
	if(mine_state == State::Shares && mine_state2 >= State2::Built)
	{
		days_gold += days;
		int count = days_gold / 30;
		if(count)
		{
			days_gold -= count * 30;
			return count * 500;
		}
	}
	else if(mine_state == State::BigShares && mine_state2 >= State2::Expanded)
	{
		days_gold += days;
		int count = days_gold / 30;
		if(count)
		{
			days_gold -= count * 30;
			return count * 1000;
		}
	}
	return 0;
}
