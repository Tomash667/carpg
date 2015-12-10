#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "TextBox.h"
#include "PickItemDialog.h"
#include "GameGui.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "StatsPanel.h"
#include "Minimap.h"

//=================================================================================================
void Game::OnResize()
{
	cursor_pos = VEC2(float(wnd_size.x)/2, float(wnd_size.y)/2);
	GUI.wnd_size = wnd_size;
	if(game_gui)
		game_gui->PositionPanels();
	GUI.OnResize(wnd_size);
	console->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void Game::OnFocus(bool focus)
{
	if(!focus && game_gui)
		game_gui->use_cursor = false;
}

//=================================================================================================
void Game::UpdateGui(float dt)
{
	// myszka
	if(GUI.NeedCursor())
	{
		if(cursor_allow_move)
		{
			cursor_pos += VEC2(float(mouse_dif.x), float(mouse_dif.y)) * mouse_sensitivity_f;
			if(cursor_pos.x < 0)
				cursor_pos.x = 0;
			if(cursor_pos.y < 0)
				cursor_pos.y = 0;
			if(cursor_pos.x >= wnd_size.x)
				cursor_pos.x = float(wnd_size.x-1);
			if(cursor_pos.y >= wnd_size.y)
				cursor_pos.y = float(wnd_size.y-1);
			unlock_point = INT2(cursor_pos);
		}
	}
	else
		unlock_point = real_size/2;

	GUI.cursor_pos = INT2(cursor_pos);
	GUI.mouse_wheel = float(mouse_wheel)/WHEEL_DELTA;
	GUI.Update(dt);

	exit_to_menu = false;
}

//=================================================================================================
void Game::CloseGamePanels()
{
	game_gui->ClosePanels();
}

//=================================================================================================
void Game::SetGamePanels()
{
	game_gui->inv_trade_mine->i_items = game_gui->inventory->i_items = &tmp_inventory[0];
	game_gui->inv_trade_mine->items = game_gui->inventory->items = &pc->unit->items;
	game_gui->inv_trade_mine->slots = game_gui->inventory->slots = pc->unit->slots;
	game_gui->inv_trade_mine->unit = game_gui->inventory->unit = pc->unit;
	game_gui->inv_trade_other->i_items = &tmp_inventory[1];
	game_gui->stats->pc = pc;
}

//=================================================================================================
void Game::InitGui()
{
	GUI.Init(device, sprite);
	GUI.wnd_size = wnd_size;

	GUI.default_font = GUI.CreateFont("Arial", 12, 800, 512, 2);
	GUI.fBig = GUI.CreateFont("Florence Regular", 28, 800, 512);
	GUI.fSmall = GUI.CreateFont("Arial", 10, 500, 512);

	/* UK£AD GUI
	GUI.layers
		load_screen
		game_gui
			game_gui front
			mp_box
			inventory
			stats
			team_panel
			gp_trade
				inv_trade_mine
				inv_trade_other
			journal
			minimap
			game_gui base
			game_messages
		main_menu
		world_map
			mp_box
			journal
			game_messages
	GUI.dialog_layers
		console
		game_menu
		options
		saveload
		create_character
		multiplayer_panel
		create_server_panel
		pick_server_panel
		server_panel
		info_box
	*/	

	Dialog::game = this;
	
	// load screen
	load_screen = new LoadScreen;
	GUI.Add(load_screen);

	// game gui
	game_gui = new GameGui;
	GUI.Add(game_gui);

	// main menu
	main_menu = new MainMenu;
	main_menu->event = DialogEvent(this, &Game::MainMenuEvent);
	main_menu->check_updates = check_updates;
	main_menu->skip_version = skip_version;
	GUI.Add(main_menu);

	// worldmap
	world_map = new WorldMapGui;
	GUI.Add(world_map);

	// konsola
	DialogInfo info;
	info.event = nullptr;
	info.name = "console";
	info.parent = nullptr;
	info.pause = true;
	info.text = "";
	info.order = ORDER_TOPMOST;
	info.type = DIALOG_CUSTOM;
	console = new Console(info);

	// menu
	info.name = "game_menu";
	info.event = DialogEvent(this, &Game::MenuEvent);
	info.order = ORDER_TOP;
	game_menu = new GameMenu(info);

	// opcje
	info.name = "options";
	info.event = DialogEvent(this, &Game::OptionsEvent);
	options = new Options(info);

	// zapisywanie / wczytywanie
	info.name = "saveload";
	info.event = DialogEvent(this, &Game::SaveLoadEvent);
	saveload = new SaveLoad(info);

	// tworzenie postaci
	info.name = "create_character";
	info.event = DialogEvent(this, &Game::OnCreateCharacter);
	create_character = new CreateCharacterPanel(info);

	// gra wieloosobowa
	info.name = "multiplayer";
	info.event = DialogEvent(this, &Game::MultiplayerPanelEvent);
	multiplayer_panel = new MultiplayerPanel(info);

	// tworzenie serwera
	info.name = "create_server";
	info.event = DialogEvent(this, &Game::CreateServerEvent);
	create_server_panel = new CreateServerPanel(info);

	// wybór serwera
	info.name = "pick_server";
	info.event = DialogEvent(this, &Game::OnPickServer);
	pick_server_panel = new PickServerPanel(info);

	// server panel
	info.name = "server_panel";
	info.event = nullptr;
	server_panel = new ServerPanel(info);

	// info box
	info.name = "info_box";
	info_box = new InfoBox(info);

	// sterowanie
	info.name = "controls";
	info.parent = options;
	controls = new Controls(info);
}

//=================================================================================================
void Game::LoadGuiData()
{
	load_tasks.push_back(LoadTask("game_panel.png", &GamePanel::tBackground));
	load_tasks.push_back(LoadTask("dialog.png", &Control::tDialog));
	load_tasks.push_back(LoadTask("scrollbar.png", &Scrollbar::tex));
	load_tasks.push_back(LoadTask("scrollbar2.png", &Scrollbar::tex2));
	load_tasks.push_back(LoadTask("cursor.png", &GUI.tCursor[CURSOR_NORMAL]));
	load_tasks.push_back(LoadTask("hand.png", &GUI.tCursor[CURSOR_HAND]));
	load_tasks.push_back(LoadTask("text.png", &GUI.tCursor[CURSOR_TEXT]));
	load_tasks.push_back(LoadTask("czaszka.png", &TeamPanel::tCzaszka));
	load_tasks.push_back(LoadTask("korona.png", &TeamPanel::tKorona));
	load_tasks.push_back(LoadTask("button.png", &Button::tex[Button::NONE]));
	load_tasks.push_back(LoadTask("button_hover.png", &Button::tex[Button::HOVER]));
	load_tasks.push_back(LoadTask("button_down.png", &Button::tex[Button::DOWN]));
	load_tasks.push_back(LoadTask("button_disabled.png", &Button::tex[Button::DISABLED]));
	load_tasks.push_back(LoadTask("background.bmp", &Dialog::tBackground));
	load_tasks.push_back(LoadTask("scrollbar.png", &TextBox::tBox));
	load_tasks.push_back(LoadTask("tlo_konsoli.jpg", &Console::tBackground));
	load_tasks.push_back(LoadTask("logo_small.png", &GameMenu::tLogo));
	load_tasks.push_back(LoadTask("ticked.png", &CheckBox::tTick));
	load_tasks.push_back(LoadTask("menu_bg.jpg", &MainMenu::tBackground));
	load_tasks.push_back(LoadTask("logo.png", &MainMenu::tLogo));
	load_tasks.push_back(LoadTask("book.png", &Journal::tBook));
	load_tasks.push_back(LoadTask("dziennik_przyciski.png", &Journal::tPage[0]));
	load_tasks.push_back(LoadTask("dziennik_przyciski2.png", &Journal::tPage[1]));
	load_tasks.push_back(LoadTask("dziennik_przyciski3.png", &Journal::tPage[2]));
	load_tasks.push_back(LoadTask("strzalka_l.png", &Journal::tArrowL));
	load_tasks.push_back(LoadTask("strzalka_p.png", &Journal::tArrowR));
	load_tasks.push_back(LoadTask("minihp.png", &GUI.tMinihp[0]));
	load_tasks.push_back(LoadTask("minihp2.png", &GUI.tMinihp[1]));
	load_tasks.push_back(LoadTask("box.png", &IGUI::tBox));
	load_tasks.push_back(LoadTask("box2.png", &IGUI::tBox2));
	load_tasks.push_back(LoadTask("pix.png", &IGUI::tPix));
	load_tasks.push_back(LoadTask("dialog_down.png", &IGUI::tDown));

	load_tasks.push_back(LoadTask("close.png", &PickItemDialog::custom_x.tex[Button::NONE]));
	load_tasks.push_back(LoadTask("close_hover.png", &PickItemDialog::custom_x.tex[Button::HOVER]));
	load_tasks.push_back(LoadTask("close_down.png", &PickItemDialog::custom_x.tex[Button::DOWN]));
	load_tasks.push_back(LoadTask("close_disabled.png", &PickItemDialog::custom_x.tex[Button::DISABLED]));

	create_character->LoadData(load_tasks);
	game_gui->LoadData(load_tasks);
	Inventory::LoadData(load_tasks);
	world_map->LoadData(load_tasks);
}

//=================================================================================================
void Game::PostInitGui()
{
}

//=================================================================================================
void Game::NullGui()
{
	main_menu = nullptr;
	world_map = nullptr;
	game_gui = nullptr;
	console = nullptr;
	game_menu = nullptr;
	options = nullptr;
	saveload = nullptr;
	create_character = nullptr;
	multiplayer_panel = nullptr;
	create_server_panel = nullptr;
	pick_server_panel = nullptr;
	server_panel = nullptr;
	info_box = nullptr;
	load_screen = nullptr;
	controls = nullptr;
}

//=================================================================================================
void Game::RemoveGui()
{
	delete game_gui;
	delete main_menu;
	delete world_map;
	delete console;
	delete game_menu;
	delete options;
	delete saveload;
	delete create_character;
	delete multiplayer_panel;
	delete create_server_panel;
	delete pick_server_panel;
	delete server_panel;
	delete info_box;
	delete GetTextDialog::self;
	delete GetNumberDialog::self;
	delete PickItemDialog::self;
	delete load_screen;
	delete controls;
}

//=================================================================================================
// Only for back compability, ignored now
void Game::LoadGui(FileReader& f)
{
	LocalVector<GamePanel*> panels;
	game_gui->GetGamePanels(panels);
	INT2 prev_wnd_size, _pos, _size;
	f >> prev_wnd_size;
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		f >> _pos;
		f >> _size;
	}
}

//=================================================================================================
// Clear gui state after new game/loading/entering new location
void Game::ClearGui(bool reset_mpbox)
{
	if(game_gui)
	{
		game_gui->Reset();
		game_gui->game_messages->Reset();
		if(game_gui->mp_box && reset_mpbox)
			game_gui->mp_box->visible = false;
	}
}
