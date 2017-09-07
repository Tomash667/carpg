#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "TextBox.h"
#include "PickItemDialog.h"
#include "GameGui.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "StatsPanel.h"
#include "Minimap.h"
#include "Console.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "WorldMapGui.h"
#include "GameMenu.h"
#include "Options.h"
#include "SaveLoadPanel.h"
#include "CreateCharacterPanel.h"
#include "MultiplayerPanel.h"
#include "CreateServerPanel.h"
#include "PickServerPanel.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "Controls.h"
#include "GetTextDialog.h"
#include "GetNumberDialog.h"
#include "GameMessages.h"
#include "MpBox.h"
#include "PickFileDialog.h"

//=================================================================================================
void Game::OnResize()
{
	cursor_pos = Vec2(float(wnd_size.x) / 2, float(wnd_size.y) / 2);
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
	if(GUI.NeedCursor() && cursor_allow_move)
	{
		cursor_pos += Vec2(float(mouse_dif.x), float(mouse_dif.y)) * mouse_sensitivity_f;
		if(cursor_pos.x < 0)
			cursor_pos.x = 0;
		if(cursor_pos.y < 0)
			cursor_pos.y = 0;
		if(cursor_pos.x >= wnd_size.x)
			cursor_pos.x = float(wnd_size.x - 1);
		if(cursor_pos.y >= wnd_size.y)
			cursor_pos.y = float(wnd_size.y - 1);
		unlock_point = Int2(cursor_pos);
	}
	else
		unlock_point = real_size / 2;

	GUI.prev_cursor_pos = GUI.cursor_pos;
	Int2 icursor_pos = Int2(cursor_pos);
	GUI.cursor_pos = icursor_pos;
	GUI.mouse_wheel = float(mouse_wheel) / WHEEL_DELTA;
	GUI.Update(dt);
	if(icursor_pos != GUI.cursor_pos)
		cursor_pos = Vec2(GUI.cursor_pos);
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
void Game::PreinitGui()
{
	GUI.Init(device, sprite);
	GUI.wnd_size = wnd_size;
	GUI.cursor_pos = Int2(cursor_pos);

	// font loading works only from main thread (for now...)
	int result = AddFontResourceExA("data/fonts/Florence-Regular.otf", FR_PRIVATE, nullptr);
	if(result == 0)
		Error("Failed to load font 'Florence-Regular.otf' (%d)!", GetLastError());
	else
		Info("Added font resource: %d.", result);
	GUI.default_font = GUI.CreateFont("Arial", 12, 800, 512, 2);
	GUI.fBig = GUI.CreateFont("Florence Regular", 28, 800, 512);
	GUI.fSmall = GUI.CreateFont("Arial", 10, 500, 512);

	GUI.InitLayout();

	GameDialogBox::game = this;

	// load screen
	load_screen = new LoadScreen;
	GUI.Add(load_screen);
}

//=================================================================================================
void Game::InitGui()
{
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

	// game gui
	game_gui = new GameGui;
	GUI.Add(game_gui);

	// main menu
	main_menu = new MainMenu(this, DialogEvent(this, &Game::MainMenuEvent), check_updates);
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
	auto& tex_mgr = ResourceManager::Get<Texture>();

	tex_mgr.AddLoadTask("game_panel.png", GamePanel::tBackground);
	tex_mgr.AddLoadTask("dialog.png", Control::tDialog);
	tex_mgr.AddLoadTask("scrollbar.png", Scrollbar::tex);
	tex_mgr.AddLoadTask("scrollbar2.png", Scrollbar::tex2);
	tex_mgr.AddLoadTask("cursor.png", GUI.tCursor[CURSOR_NORMAL]);
	tex_mgr.AddLoadTask("hand.png", GUI.tCursor[CURSOR_HAND]);
	tex_mgr.AddLoadTask("text.png", GUI.tCursor[CURSOR_TEXT]);
	tex_mgr.AddLoadTask("czaszka.png", TeamPanel::tCzaszka);
	tex_mgr.AddLoadTask("korona.png", TeamPanel::tKorona);
	tex_mgr.AddLoadTask("button.png", Button::tex[Button::NONE]);
	tex_mgr.AddLoadTask("button_hover.png", Button::tex[Button::HOVER]);
	tex_mgr.AddLoadTask("button_down.png", Button::tex[Button::DOWN]);
	tex_mgr.AddLoadTask("button_disabled.png", Button::tex[Button::DISABLED]);
	tex_mgr.AddLoadTask("background.bmp", DialogBox::tBackground);
	tex_mgr.AddLoadTask("scrollbar.png", TextBox::tBox);
	tex_mgr.AddLoadTask("tlo_konsoli.jpg", Console::tBackground);
	tex_mgr.AddLoadTask("logo_small.png", GameMenu::tLogo);
	tex_mgr.AddLoadTask("ticked.png", CheckBox::tTick);
	tex_mgr.AddLoadTask("book.png", Journal::tBook);
	tex_mgr.AddLoadTask("dziennik_przyciski.png", Journal::tPage[0]);
	tex_mgr.AddLoadTask("dziennik_przyciski2.png", Journal::tPage[1]);
	tex_mgr.AddLoadTask("dziennik_przyciski3.png", Journal::tPage[2]);
	tex_mgr.AddLoadTask("strzalka_l.png", Journal::tArrowL);
	tex_mgr.AddLoadTask("strzalka_p.png", Journal::tArrowR);
	tex_mgr.AddLoadTask("box.png", IGUI::tBox);
	tex_mgr.AddLoadTask("box2.png", IGUI::tBox2);
	tex_mgr.AddLoadTask("pix.png", IGUI::tPix);
	tex_mgr.AddLoadTask("dialog_down.png", IGUI::tDown);

	tex_mgr.AddLoadTask("close.png", PickItemDialog::custom_x.tex[Button::NONE]);
	tex_mgr.AddLoadTask("close_hover.png", PickItemDialog::custom_x.tex[Button::HOVER]);
	tex_mgr.AddLoadTask("close_down.png", PickItemDialog::custom_x.tex[Button::DOWN]);
	tex_mgr.AddLoadTask("close_disabled.png", PickItemDialog::custom_x.tex[Button::DISABLED]);

	main_menu->LoadData();
	create_character->LoadData();
	game_gui->LoadData();
	Inventory::LoadData();
	world_map->LoadData();
	server_panel->LoadData();
	pick_server_panel->LoadData();
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
	gui::PickFileDialog::Destroy();
}

//=================================================================================================
// Only for back compability, ignored now
void Game::LoadGui(FileReader& f)
{
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
