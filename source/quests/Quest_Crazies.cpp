#include "Pch.h"
#include "Quest_Crazies.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "World.h"
#include "Level.h"
#include "AIController.h"
#include "Team.h"
#include "GameGui.h"
#include "GameMessages.h"

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
	target_loc = -1;
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
	case Progress::Started: // zaatakowano przez unk
		{
			OnStart(game->txQuest[253]);
			msgs.push_back(Format(game->txQuest[170], world->GetDate()));
			msgs.push_back(game->txQuest[254]);
		}
		break;
	case Progress::KnowLocation: // trener powiedzia³ o labiryncie
		{
			start_loc = world->GetCurrentLocationIndex();
			Location& loc = *world->CreateLocation(L_DUNGEON, Vec2(0, 0), -128.f, LABYRINTH, UnitGroup::Get("unk"), false);
			loc.active_quest = this;
			loc.SetKnown();
			loc.st = 13;
			target_loc = loc.index;

			crazies_state = State::TalkedTrainer;

			OnUpdate(Format(game->txQuest[255], world->GetCurrentLocation()->name.c_str(), loc.name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::Finished: // schowano kamieñ do skrzyni
		{
			state = Quest::Completed;
			GetTargetLocation().active_quest = nullptr;

			crazies_state = State::End;
			team->AddExp(12000);

			OnUpdate(game->txQuest[256]);
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

	return LoadResult::Ok;
}

//=================================================================================================
bool Quest_Crazies::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "crazies_talked") == 0)
	{
		ctx.talker->ai->morale = -100.f;
		crazies_state = State::TalkedWithCrazy;
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
		// usuñ kamieñ z gry o ile to nie encounter bo i tak jest resetowany
		if(game_level->location->type != L_ENCOUNTER)
		{
			if(target_loc == game_level->location_index)
			{
				// jest w dobrym miejscu, sprawdŸ czy w³o¿y³ kamieñ do skrzyni
				Chest* chest;
				int slot;
				if(game_level->local_area->FindItemInChest(stone, &chest, &slot))
				{
					// w³o¿y³ kamieñ, koniec questa
					chest->items.erase(chest->items.begin() + slot);
					SetProgress(Progress::Finished);
					return;
				}
			}

			game_level->RemoveItemFromWorld(stone);
		}

		// dodaj kamieñ przywódcy
		team->leader->AddItem(stone, 1, false);
		game_gui->messages->AddGameMsg3(team->leader->player, GMS_ADDED_CURSED_STONE);
	}

	if(crazies_state == State::TalkedWithCrazy)
	{
		crazies_state = State::PickedStone;
		days = 13;
	}
}
