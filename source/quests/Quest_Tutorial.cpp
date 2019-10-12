#include "Pch.h"
#include "GameCore.h"
#include "Quest_Tutorial.h"
#include "HumanData.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "CreateCharacterPanel.h"
#include "LevelGui.h"
#include "Journal.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "WorldMapGui.h"
#include "Language.h"
#include "SingleInsideLocation.h"
#include "World.h"
#include "Level.h"
#include "LocationGeneratorFactory.h"
#include "SoundManager.h"
#include "ParticleSystem.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"

//=================================================================================================
void Quest_Tutorial::LoadLanguage()
{
	LoadArray(txTut, "tut");
	txTutNote = Str("tutNote");
	txTutLoc = Str("tutLoc");
}

//=================================================================================================
void Quest_Tutorial::Start()
{
	game->LoadingStart(1);

	HumanData hd;
	hd.Get(*game_gui->create_character->unit->human_data);
	game->NewGameCommon(game_gui->create_character->clas, game_gui->create_character->player_name.c_str(), hd, game_gui->create_character->cc, true);
	in_tutorial = true;
	state = 0;
	texts.clear();
	quest_mgr->quest_contest->state = Quest_Contest::CONTEST_NOT_DONE;
	game->pc->data.autowalk = false;
	game->pc->shortcuts[2].type = Shortcut::TYPE_NONE; // disable action in tutorial

	// ekwipunek
	game->pc->unit->ClearInventory();
	auto item = Item::Get("al_clothes");
	game_res->PreloadItem(item);
	game->pc->unit->slots[SLOT_ARMOR] = item;
	game->pc->unit->weight += game->pc->unit->slots[SLOT_ARMOR]->weight;
	game->pc->unit->gold = 10;
	game_gui->journal->GetNotes().push_back(txTutNote);

	// startowa lokacja
	SingleInsideLocation* loc = new SingleInsideLocation;
	loc->target = TUTORIAL_FORT;
	loc->name = txTutLoc;
	loc->type = L_DUNGEON;
	loc->image = LI_DUNGEON;
	loc->group = UnitGroup::empty;
	world->StartInLocation(loc);
	game_level->dungeon_level = 0;

	game->loc_gen_factory->Get(game_level->location)->OnEnter();

	// go!
	game->LoadResources("", false);
	game_level->event_handler = nullptr;
	game->SetMusic();
	game_gui->load_screen->visible = false;
	game_gui->main_menu->visible = false;
	game_gui->level_gui->visible = true;
	game_gui->world_map->Hide();
	game->clear_color = game->clear_color_next;
	game_level->camera.Reset();
}

/*
state:
0 - pocz¹tek
1 - pierwszy tekst
2 - tekst przed drzwiami
3 - tekst przed skrzyni¹
4 - otwarto skrzyniê
5 - tekst przed manekinem
6 - uderzono manekin
7 - tekst przed goblinem
8 - zabito goblina
9 - tekst o dzienniku
10 - tekst o skrzyni z ³ukiem
11 - otwarto skrzyniê
12 - tekst o ³uku
13 - trafiono z ³uku
14 - tekst o gadaniu
*/

//=================================================================================================
void Quest_Tutorial::Update()
{
	PlayerController* pc = game->pc;

	// check tutorial texts
	for(Text& text : texts)
	{
		if(text.state != 1 || Vec3::Distance(text.pos, pc->unit->pos) > 3.f)
			continue;

		DialogInfo info;
		info.event = nullptr;
		info.name = "tut";
		info.order = ORDER_TOP;
		info.parent = nullptr;
		info.pause = true;
		info.text = text.text;
		info.type = DIALOG_OK;
		gui->ShowDialog(info);

		text.state = 2;

		int activate = -1,
			unlock = -1;

		switch(text.id)
		{
		case 0:
			activate = 1;
			if(state < 1)
				state = 1;
			break;
		case 1:
			unlock = 0;
			activate = 2;
			if(state < 2)
				state = 2;
			break;
		case 2:
			if(state < 3)
				state = 3;
			break;
		case 3:
			if(state < 5)
				state = 5;
			break;
		case 4:
			if(state < 7)
				state = 7;
			unlock = 2;
			break;
		case 5:
			if(state < 9)
				state = 9;
			unlock = 4;
			activate = 6;
			break;
		case 6:
			if(state < 10)
				state = 10;
			break;
		case 7:
			if(state < 12)
				state = 12;
			break;
		case 8:
			if(state < 14)
				state = 14;
			break;
		}

		HandleEvent(activate, unlock);
		break;
	}
}

//=================================================================================================
void Quest_Tutorial::Finish(int)
{
	gui->GetDialog("tut_end")->visible = false;
	finished_tutorial = true;
	game->ClearGame();
	game->StartNewGame();
}

//=================================================================================================
void Quest_Tutorial::OnEvent(Event event)
{
	int activate = -1,
		unlock = -1;

	switch(event)
	{
	case OpenedChest1:
		if(state < 4)
		{
			unlock = 1;
			activate = 3;
			state = 4;
		}
		break;
	case OpenedChest2:
		if(state < 11)
		{
			unlock = 5;
			activate = 7;
			state = 11;
		}
		break;
	case KilledGoblin:
		if(state < 8)
		{
			unlock = 3;
			activate = 5;
			state = 8;
		}
		break;
	case Exit:
		{
			DialogInfo info;
			info.event = DialogEvent(this, &Quest_Tutorial::Finish);
			info.name = "tut_end";
			info.order = ORDER_TOP;
			info.parent = nullptr;
			info.pause = true;
			info.text = txTut[9];
			info.type = DIALOG_OK;
			gui->ShowDialog(info);
		}
		break;
	default:
		assert(0);
		break;
	}

	HandleEvent(activate, unlock);
}

//=================================================================================================
void Quest_Tutorial::HandleEvent(int activate, int unlock)
{
	if(activate != -1)
	{
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			if(it->id == activate)
			{
				it->state = 1;
				break;
			}
		}
	}

	if(unlock != -1)
	{
		for(Door* door : game_level->local_area->doors)
		{
			if(door->locked == LOCK_TUTORIAL + unlock)
			{
				door->locked = LOCK_NONE;
				break;
			}
		}
	}
}

//=================================================================================================
void Quest_Tutorial::HandleChestEvent(ChestEventHandler::Event event, Chest* chest)
{
	OnEvent(chest == chests[0] ? OpenedChest1 : OpenedChest2);
}

//=================================================================================================
void Quest_Tutorial::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	OnEvent(KilledGoblin);
}

//=================================================================================================
void Quest_Tutorial::HandleMeleeAttackCollision()
{
	if(state != 5)
		return;

	game->pc->Train(true, (int)SkillId::ONE_HANDED_WEAPON, TrainMode::Tutorial);
	state = 6;
	int activate = 4;
	for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
	{
		if(it->id == activate)
		{
			it->state = 1;
			break;
		}
	}
}

//=================================================================================================
void Quest_Tutorial::HandleBulletCollision()
{
	if(state != 12)
		return;

	game->pc->Train(true, (int)SkillId::BOW, TrainMode::Tutorial);
	state = 13;
	int unlock = 6;
	int activate = 8;
	for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
	{
		if(it->id == activate)
		{
			it->state = 1;
			break;
		}
	}
	for(vector<Door*>::iterator it = game_level->local_area->doors.begin(), end = game_level->local_area->doors.end(); it != end; ++it)
	{
		if((*it)->locked == LOCK_TUTORIAL + unlock)
		{
			(*it)->locked = LOCK_NONE;
			break;
		}
	}
}
