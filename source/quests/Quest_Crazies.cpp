#include "Pch.h"
#include "Quest_Crazies.h"

#include "AIController.h"
#include "BaseLocation.h"
#include "DialogContext.h"
#include "Encounter.h"
#include "GameMessages.h"
#include "GameGui.h"
#include "Journal.h"
#include "Level.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//=================================================================================================
void Quest_Crazies::Init()
{
	quest_mgr->RegisterSpecialHandler(this, "crazies_talked");
	quest_mgr->RegisterSpecialHandler(this, "crazies_sell_stone");
	quest_mgr->RegisterSpecialIfHandler(this, "crazies_not_asked");
	quest_mgr->RegisterSpecialIfHandler(this, "crazies_need_talk");
}

//=================================================================================================
void Quest_Crazies::Start()
{
	category = QuestCategory::Unique;
	type = Q_CRAZIES;
	crazies_state = State::None;
	days = 0;
	check_stone = false;
	stone = Item::Get("q_szaleni_kamien");
}

//=================================================================================================
GameDialog* Quest_Crazies::GetDialog(int type2)
{
	return GameDialog::TryGet("q_crazies_trainer");
}

//=================================================================================================
void Quest_Crazies::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(quest_mgr->txQuest[253]);
			msgs.push_back(Format(quest_mgr->txQuest[170], world->GetDate()));
			msgs.push_back(quest_mgr->txQuest[254]);
		}
		break;
	case Progress::KnowLocation:
		{
			startLoc = world->GetCurrentLocation();
			Location& loc = *world->CreateLocation(L_DUNGEON, world->GetRandomPlace(), LABYRINTH);
			loc.group = UnitGroup::Get("unk");
			loc.active_quest = this;
			loc.SetKnown();
			loc.st = 13;
			targetLoc = &loc;

			crazies_state = State::TalkedTrainer;

			OnUpdate(Format(quest_mgr->txQuest[255], world->GetCurrentLocation()->name.c_str(), loc.name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			targetLoc->active_quest = nullptr;

			crazies_state = State::End;
			world->RemoveGlobalEncounter(this);
			team->AddExp(12000);

			OnUpdate(quest_mgr->txQuest[256]);
			quest_mgr->EndUniqueQuest();
		}
	}
}

//=================================================================================================
cstring Quest_Crazies::FormatString(const string& str)
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
bool Quest_Crazies::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "szaleni") == 0;
}

//=================================================================================================
void Quest_Crazies::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << crazies_state;
	f << days;
	f << check_stone;
}

//=================================================================================================
Quest::LoadResult Quest_Crazies::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	stone = Item::Get("q_szaleni_kamien");

	f >> crazies_state;
	f >> days;
	f >> check_stone;

	if(crazies_state == State::TalkedWithCrazy)
	{
		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
		globalEnc->chance = 50;
		globalEnc->quest = this;
		globalEnc->text = quest_mgr->txQuest[251];
		world->AddGlobalEncounter(globalEnc);
	}
	else if(crazies_state == State::PickedStone && crazies_state < State::End && days <= 0)
	{
		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
		globalEnc->chance = 33;
		globalEnc->quest = this;
		globalEnc->text = quest_mgr->txQuest[252];
		world->AddGlobalEncounter(globalEnc);
	}

	return LoadResult::Ok;
}

//=================================================================================================
bool Quest_Crazies::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "crazies_talked") == 0)
	{
		ctx.talker->ai->morale = -100.f;
		crazies_state = State::TalkedWithCrazy;

		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
		globalEnc->chance = 50;
		globalEnc->quest = this;
		globalEnc->text = quest_mgr->txQuest[251];
		world->AddGlobalEncounter(globalEnc);
	}
	else if(strcmp(msg, "crazies_sell_stone") == 0)
	{
		ctx.pc->unit->RemoveItem(stone, 1);
		ctx.pc->unit->ModGold(10);
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Crazies::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "crazies_not_asked") == 0)
		return crazies_state == State::None;
	else if(strcmp(msg, "crazies_need_talk") == 0)
		return crazies_state == State::FirstAttack;
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Crazies::CheckStone()
{
	check_stone = false;

	if(!team->FindItemInTeam(stone, -1, nullptr, nullptr, false))
	{
		// remove item from game, unless it is encounter (because level is reset anyway)
		if(game_level->location->type != L_ENCOUNTER)
		{
			if(targetLoc == game_level->location)
			{
				// is in good location, check if inside chest
				int index;
				Chest* chest = game_level->localPart->FindChestWithItem(stone, &index);
				if(chest)
				{
					// put inside chest, end of quest
					chest->items.erase(chest->items.begin() + index);
					SetProgress(Progress::Finished);
					return;
				}
			}

			game_level->RemoveItemFromWorld(stone);
		}

		// add stone to leader
		team->leader->AddItem(stone, 1, false);
		game_gui->messages->AddGameMsg3(team->leader->player, GMS_ADDED_CURSED_STONE);
	}

	if(crazies_state == State::TalkedWithCrazy)
	{
		crazies_state = State::PickedStone;
		days = 13;
		world->RemoveGlobalEncounter(this);
	}
}

//=================================================================================================
void Quest_Crazies::OnProgress(int d)
{
	if(crazies_state == Quest_Crazies::State::PickedStone && days > 0)
	{
		days -= d;
		if(days <= 0)
		{
			GlobalEncounter* globalEnc = new GlobalEncounter;
			globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
			globalEnc->chance = 33;
			globalEnc->quest = this;
			globalEnc->text = quest_mgr->txQuest[252];
			world->AddGlobalEncounter(globalEnc);
		}
	}
}

//=================================================================================================
void Quest_Crazies::OnEncounter(EncounterSpawn& spawn)
{
	if(crazies_state == State::TalkedWithCrazy)
	{
		spawn.group_name = nullptr;
		spawn.essential = UnitData::Get("q_szaleni_szaleniec");
		spawn.level = 13;
		spawn.dont_attack = true;
		spawn.dialog = GameDialog::TryGet("q_crazies");
		spawn.count = 1;

		check_stone = true;
	}
	else
	{
		spawn.group_name = "unk";
		spawn.level = 13;
		spawn.back_attack = true;
		if(crazies_state == State::PickedStone)
		{
			crazies_state = State::FirstAttack;
			spawn.count = 1;
			SetProgress(Progress::Started);
		}
		else
		{
			int pts = team->GetStPoints();
			if(pts >= 80)
				pts = 80;
			pts = int(Random(0.5f, 0.75f) * pts);
			spawn.count = max(1, pts / 13);
		}
	}
}
