#include "Pch.h"
#include "Quest_Tutorial.h"

#include "CreateCharacterPanel.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "HumanData.h"
#include "Journal.h"
#include "Language.h"
#include "Level.h"
#include "LevelGui.h"
#include "LoadScreen.h"
#include "LocationGeneratorFactory.h"
#include "MainMenu.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "SingleInsideLocation.h"
#include "World.h"
#include "WorldMapGui.h"

//=================================================================================================
void Quest_Tutorial::LoadLanguage()
{
	StrArray(txTut, "tut");
	txTutNote = Str("tutNote");
	txTutLoc = Str("tutLoc");
}

//=================================================================================================
void Quest_Tutorial::Start()
{
	game->LoadingStart(1);

	HumanData hd;
	hd.Get(*gameGui->createCharacter->unit->humanData);
	game->NewGameCommon(gameGui->createCharacter->clas, gameGui->createCharacter->playerName.c_str(), hd, gameGui->createCharacter->cc, true);
	inTutorial = true;
	state = 0;
	texts.clear();
	questMgr->questContest->state = Quest_Contest::CONTEST_NOT_DONE;
	game->pc->data.autowalk = false;
	game->pc->shortcuts[2].type = Shortcut::TYPE_NONE; // disable action in tutorial

	// inventory
	game->pc->unit->ClearInventory();
	game->pc->unit->EquipItem(Item::Get("al_clothes"));
	game->pc->unit->gold = 10;
	gameGui->journal->GetNotes().push_back(txTutNote);

	// start location
	SingleInsideLocation* loc = new SingleInsideLocation;
	loc->target = TUTORIAL_FORT;
	loc->name = txTutLoc;
	loc->type = L_DUNGEON;
	loc->image = LI_DUNGEON;
	loc->group = UnitGroup::empty;
	world->StartInLocation(loc);
	gameLevel->dungeonLevel = 0;

	game->locGenFactory->Get(gameLevel->location)->OnEnter();

	// go!
	game->LoadResources("", false);
	gameLevel->eventHandler = nullptr;
	game->SetMusic();
	gameGui->loadScreen->visible = false;
	gameGui->mainMenu->visible = false;
	gameGui->levelGui->visible = true;
	gameGui->worldMap->Hide();
	gameLevel->camera.Reset();
	game->gameState = GS_LEVEL;
	gameLevel->ready = true;
}

/*
state:
0 - start
1 - first text
2 - text before door
3 - text before chest
4 - opened chest
5 - text before melee target
6 - hit melee target
7 - text before goblin
8 - killed goblin
9 - text about journal
10 - text about chest with bow
11 - opened chest with bow
12 - text about bow
13 - hit with bow
14 - text about talking
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
		info.order = DialogOrder::Top;
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
	finishedTutorial = true;
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
			info.order = DialogOrder::Top;
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
		for(Door* door : gameLevel->localPart->doors)
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
	for(vector<Door*>::iterator it = gameLevel->localPart->doors.begin(), end = gameLevel->localPart->doors.end(); it != end; ++it)
	{
		if((*it)->locked == LOCK_TUTORIAL + unlock)
		{
			(*it)->locked = LOCK_NONE;
			break;
		}
	}
}
