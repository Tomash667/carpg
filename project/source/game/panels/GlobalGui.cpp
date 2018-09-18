#include "Pch.h"
#include "GameCore.h"
#include "GlobalGui.h"
#include "World.h"
#include "Level.h"
#include "LoadScreen.h"
#include "GameDialogBox.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "MpBox.h"
#include "Inventory.h"
#include "StatsPanel.h"
#include "Game.h"

GlobalGui::GlobalGui() : load_screen(nullptr), game_gui(nullptr)
{
}

GlobalGui::~GlobalGui()
{
	delete load_screen;
	delete game_gui;
}

void GlobalGui::Prepare()
{
	Game& game = Game::Get();

	GUI.Init(game.device, game.sprite);
	GUI.AddFont("Florence-Regular.otf");
	GUI.default_font = GUI.CreateFont("Arial", 12, 800, 512, 2);
	GUI.fBig = GUI.CreateFont("Florence Regular", 28, 800, 512);
	GUI.fSmall = GUI.CreateFont("Arial", 10, 500, 512);
	GUI.InitLayout();

	GameDialogBox::game = &game;

	// load screen
	load_screen = new LoadScreen;
	GUI.Add(load_screen);
}

void GlobalGui::InitOnce()
{
	// game gui
	game_gui = new GameGui;
	game_gui->LoadData();
	GUI.Add(game_gui);

	// inventory
	inventory = new Inventory;
	inventory->InitOnce();
	game_gui->Add(inventory->inv_mine);
	game_gui->Add(inventory->gp_trade);

	GUI.Add(this);
}

void GlobalGui::LoadLanguage()
{
	game_gui->LoadLanguage();
	inventory->LoadLanguage();
}

void GlobalGui::Draw(ControlDrawData*)
{
	Container::Draw();

	FIXME;
	if(!Any(Game::Get().game_state, GS_LEVEL, GS_WORLDMAP))
		return;

	cstring state;
	switch(W.GetState())
	{
	default:
	case World::State::ON_MAP:
		state = "ON_MAP";
		break;
	case World::State::ENCOUNTER:
		state = "ENCOUNTER";
		break;
	case World::State::INSIDE_LOCATION:
		state = "INSIDE_LOCATION";
		break;
	case World::State::INSIDE_ENCOUNTER:
		state = "INSIDE_ENCOUNTER";
		break;
	case World::State::TRAVEL:
		state = "TRAVEL";
		break;
	}
	GUI.DrawText(GUI.default_font, Format("state:%s\nlocation: %p %s\nindex: %d\ntravel index:%d", state, W.GetCurrentLocation(),
		W.GetCurrentLocation() ? W.GetCurrentLocation()->name.c_str() : "", W.GetCurrentLocationIndex(), W.GetTravelLocationIndex()),
		0, Color::Black, Rect(0, 0, 500, 200));
	assert(W.GetCurrentLocation() == L.location);
	assert(W.GetCurrentLocationIndex() == L.location_index);
}

void GlobalGui::LoadOldGui(FileReader& f)
{
	// old gui settings, now removed
	LocalVector<GamePanel*> panels;
	game_gui->GetGamePanels(panels);
	Int2 prev_wnd_size, _pos, _size;
	f >> prev_wnd_size;
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		f >> _pos;
		f >> _size;
	}
}

//=================================================================================================
// Clear gui state after new game/loading/entering new location
void GlobalGui::Clear(bool reset_mpbox)
{
	if(game_gui)
	{
		game_gui->Reset();
		game_gui->game_messages->Reset();
		if(game_gui->mp_box && reset_mpbox)
			game_gui->mp_box->visible = false;
	}
}

void GlobalGui::Setup(PlayerController* pc)
{
	inventory->Setup(pc);
	game_gui->stats->pc = pc;
}
