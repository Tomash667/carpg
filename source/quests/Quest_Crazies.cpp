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
	questMgr->RegisterSpecialHandler(this, "crazies_talked");
	questMgr->RegisterSpecialHandler(this, "crazies_sell_stone");
	questMgr->RegisterSpecialIfHandler(this, "crazies_not_asked");
	questMgr->RegisterSpecialIfHandler(this, "crazies_need_talk");
}

//=================================================================================================
void Quest_Crazies::Start()
{
	category = QuestCategory::Unique;
	type = Q_CRAZIES;
	craziesState = State::None;
	days = 0;
	checkStone = false;
	stone = Item::Get("qCraziesStone");
}

//=================================================================================================
GameDialog* Quest_Crazies::GetDialog(int type2)
{
	return GameDialog::TryGet("qCraziesTrainer");
}

//=================================================================================================
void Quest_Crazies::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(questMgr->txQuest[253]);
			msgs.push_back(Format(questMgr->txQuest[170], world->GetDate()));
			msgs.push_back(questMgr->txQuest[254]);
		}
		break;
	case Progress::KnowLocation:
		{
			startLoc = world->GetCurrentLocation();
			Location& loc = *world->CreateLocation(L_DUNGEON, world->GetRandomPlace(), LABYRINTH);
			loc.group = UnitGroup::Get("unk");
			loc.activeQuest = this;
			loc.SetKnown();
			loc.st = 13;
			targetLoc = &loc;

			craziesState = State::TalkedTrainer;

			OnUpdate(Format(questMgr->txQuest[255], world->GetCurrentLocation()->name.c_str(), loc.name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			targetLoc->activeQuest = nullptr;

			craziesState = State::End;
			world->RemoveGlobalEncounter(this);
			team->AddExp(12000);

			OnUpdate(questMgr->txQuest[256]);
			questMgr->EndUniqueQuest();
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

	f << craziesState;
	f << days;
	f << checkStone;
}

//=================================================================================================
Quest::LoadResult Quest_Crazies::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	stone = Item::Get("qCraziesStone");

	f >> craziesState;
	f >> days;
	f >> checkStone;

	if(craziesState == State::TalkedWithCrazy)
	{
		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
		globalEnc->chance = 50;
		globalEnc->quest = this;
		globalEnc->text = questMgr->txQuest[251];
		world->AddGlobalEncounter(globalEnc);
	}
	else if(craziesState == State::PickedStone && craziesState < State::End && days <= 0)
	{
		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
		globalEnc->chance = 33;
		globalEnc->quest = this;
		globalEnc->text = questMgr->txQuest[252];
		world->AddGlobalEncounter(globalEnc);
	}

	return LoadResult::Ok;
}

//=================================================================================================
bool Quest_Crazies::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "crazies_talked") == 0)
	{
		craziesState = State::TalkedWithCrazy;

		GlobalEncounter* globalEnc = new GlobalEncounter;
		globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
		globalEnc->chance = 50;
		globalEnc->quest = this;
		globalEnc->text = questMgr->txQuest[251];
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
		return craziesState == State::None;
	else if(strcmp(msg, "crazies_need_talk") == 0)
		return craziesState == State::FirstAttack;
	assert(0);
	return false;
}

//=================================================================================================
void Quest_Crazies::CheckStone()
{
	checkStone = false;

	if(!team->FindItemInTeam(stone, -1, nullptr, nullptr, false))
	{
		// remove item from game, unless it is encounter (because level is reset anyway)
		if(gameLevel->location->type != L_ENCOUNTER)
		{
			if(targetLoc == gameLevel->location)
			{
				// is in good location, check if inside chest
				int index;
				Chest* chest = gameLevel->localPart->FindChestWithItem(stone, &index);
				if(chest)
				{
					// put inside chest, end of quest
					chest->items.erase(chest->items.begin() + index);
					SetProgress(Progress::Finished);
					return;
				}
			}

			gameLevel->RemoveItemFromWorld(stone);
		}

		// add stone to leader
		team->leader->AddItem(stone, 1, false);
		gameGui->messages->AddGameMsg3(team->leader->player, GMS_ADDED_CURSED_STONE);
	}

	if(craziesState == State::TalkedWithCrazy)
	{
		craziesState = State::PickedStone;
		days = 13;
		world->RemoveGlobalEncounter(this);
	}
}

//=================================================================================================
void Quest_Crazies::OnProgress(int d)
{
	if(craziesState == Quest_Crazies::State::PickedStone && days > 0)
	{
		days -= d;
		if(days <= 0)
		{
			GlobalEncounter* globalEnc = new GlobalEncounter;
			globalEnc->callback = GlobalEncounter::Callback(this, &Quest_Crazies::OnEncounter);
			globalEnc->chance = 33;
			globalEnc->quest = this;
			globalEnc->text = questMgr->txQuest[252];
			world->AddGlobalEncounter(globalEnc);
		}
	}
}

//=================================================================================================
void Quest_Crazies::OnEncounter(EncounterSpawn& spawn)
{
	if(craziesState == State::TalkedWithCrazy)
	{
		spawn.groupName = nullptr;
		spawn.essential = UnitData::Get("q_szaleni_szaleniec");
		spawn.level = 13;
		spawn.dontAttack = true;
		spawn.dialog = GameDialog::TryGet("qCrazies");
		spawn.count = 1;

		checkStone = true;
	}
	else
	{
		spawn.groupName = "unk";
		spawn.level = 13;
		spawn.backAttack = true;
		if(craziesState == State::PickedStone)
		{
			craziesState = State::FirstAttack;
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
