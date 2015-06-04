#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "TextBox.h"

//=================================================================================================
void Game::OnResize()
{
	cursor_pos = VEC2(float(wnd_size.x)/2, float(wnd_size.y)/2);
	GUI.wnd_size = wnd_size;
	PositionGui();
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
			cursor_pos += VEC2(float(mouse_dif.x), float(mouse_dif.y));
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
// this name is wrong, need refactoring
bool Game::IsGamePanelOpen()
{
	return stats->visible || inventory->visible || team_panel->visible || main_menu->visible;
}

//=================================================================================================
void Game::CloseGamePanels()
{
	if(stats->visible)
		stats->Hide();
	if(inventory->visible)
		inventory->Hide();
	if(team_panel->visible)
		team_panel->Hide();
	if(journal->visible)
		journal->Hide();
	if(minimap->visible)
		minimap->Hide();
	if(gp_trade->visible)
		gp_trade->Hide();
}

//=================================================================================================
void Game::CreateGamePanels()
{
	GUI.Init(device, sprite);

	// eh
	game_gui = new GameGui;

	load_tasks.push_back(LoadTask("game_panel.png", &GamePanel::tBackground));
	load_tasks.push_back(LoadTask("item_bar.png", &Inventory::tItemBar));
	load_tasks.push_back(LoadTask("equipped.png", &Inventory::tEquipped));
	load_tasks.push_back(LoadTask("coins.png", &Inventory::tGold));
	load_tasks.push_back(LoadTask("dialog.png", &Control::tDialog));
	load_tasks.push_back(LoadTask("scrollbar.png", &Scrollbar::tex));
	load_tasks.push_back(LoadTask("scrollbar2.png", &Scrollbar::tex2));
	load_tasks.push_back(LoadTask("cursor.png", &GUI.tCursor[CURSOR_NORMAL]));
	load_tasks.push_back(LoadTask("hand.png", &GUI.tCursor[CURSOR_HAND]));
	load_tasks.push_back(LoadTask("text.png", &GUI.tCursor[CURSOR_TEXT]));
	load_tasks.push_back(LoadTask("czaszka.png", &TeamPanel::tCzaszka));
	load_tasks.push_back(LoadTask("korona.png", &TeamPanel::tKorona));
	load_tasks.push_back(LoadTask("button.png", &Button::tex[Button::NONE]));
	load_tasks.push_back(LoadTask("button_flash.png", &Button::tex[Button::FLASH]));
	load_tasks.push_back(LoadTask("button_pressed.png", &Button::tex[Button::PRESSED]));
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

	load_tasks.push_back(LoadTask("bar.png", &game_gui->tBar));
	load_tasks.push_back(LoadTask("hp_bar.png", &game_gui->tHpBar));
	load_tasks.push_back(LoadTask("poisoned_hp_bar.png", &game_gui->tPoisonedHpBar));
	load_tasks.push_back(LoadTask("mana_bar.png", &game_gui->tManaBar));
	load_tasks.push_back(LoadTask("shortcut.png", &game_gui->tShortcut));
	load_tasks.push_back(LoadTask("shortcut_hover.png", &game_gui->tShortcutHover));
	load_tasks.push_back(LoadTask("shortcut_down.png", &game_gui->tShortcutDown));
	load_tasks.push_back(LoadTask("bt_menu.png", &game_gui->tSideButton[(int)SideButtonId::Menu]));
	load_tasks.push_back(LoadTask("bt_team.png", &game_gui->tSideButton[(int)SideButtonId::Team]));
	load_tasks.push_back(LoadTask("bt_minimap.png", &game_gui->tSideButton[(int)SideButtonId::Minimap]));
	load_tasks.push_back(LoadTask("bt_journal.png", &game_gui->tSideButton[(int)SideButtonId::Journal]));
	load_tasks.push_back(LoadTask("bt_inventory.png", &game_gui->tSideButton[(int)SideButtonId::Inventory]));
	load_tasks.push_back(LoadTask("bt_active.png", &game_gui->tSideButton[(int)SideButtonId::Active]));
	load_tasks.push_back(LoadTask("bt_stats.png", &game_gui->tSideButton[(int)SideButtonId::Stats]));
	load_tasks.push_back(LoadTask("bt_talk.png", &game_gui->tSideButton[(int)SideButtonId::Talk]));
}

//=================================================================================================
void Game::SetGamePanels()
{
	inv_trade_mine->i_items = inventory->i_items = &tmp_inventory[0];
	inv_trade_mine->items = inventory->items = &pc->unit->items;
	inv_trade_mine->slots = inventory->slots = pc->unit->slots;
	inv_trade_mine->unit = inventory->unit = pc->unit;
	inv_trade_other->i_items = &tmp_inventory[1];
	stats->pc = pc;
}

//=================================================================================================
void Game::InitGui2()
{
	GUI.default_font = GUI.CreateFont("Arial", 12, 800, 512, 2);
	GUI.fBig = GUI.CreateFont("Florence Regular", 28, 800, 512);
	GUI.fSmall = GUI.CreateFont("Arial", 10, 500, 512);
	GUI.wnd_size = wnd_size;

	Inventory::game = this;
	Dialog::game = this;
	Journal::game = this;

	/* UK£AD GUI
	GUI.layers
		load_screen
		game_gui_container
			game_gui
			mp_box
			inventory
			stats
			team_panel
			gp_trade
				inv_trade_mine
				inv_trade_other
			journal
			minimap
			game_gui - 2 raz, raz rysuje z przodu, raz po panelach
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

	// ekran wczytywania
	load_screen = new LoadScreen;
	GUI.Add(load_screen);

	// kontener na gui w grze
	game_gui_container = new Container;
	game_gui_container->visible = false;
	GUI.Add(game_gui_container);

	// gui gry
	game_gui_container->Add(game_gui);

	// mp box
	mp_box = new MpBox;
	game_gui_container->Add(mp_box);
	
	// ekwipunek
	Inventory::LoadText();
	inventory = new Inventory;
	inventory->InitTooltip();
	inventory->title = Inventory::txInventory;
	inventory->mode = Inventory::INVENTORY;
	inventory->visible = false;
	game_gui_container->Add(inventory);

	// statystyki
	stats = new StatsPanel;
	game_gui_container->Add(stats);

	// dru¿yna
	team_panel = new TeamPanel;
	game_gui_container->Add(team_panel);

	// kontener na ekwipunek w handlu
	gp_trade = new GamePanelContainer;
	game_gui_container->Add(gp_trade);

	// ekwipunek gracza w handlu
	inv_trade_mine = new Inventory;
	inv_trade_mine->title = Inventory::txInventory;
	inv_trade_mine->focus = true;
	gp_trade->Add(inv_trade_mine);

	// ekwipunek kogoœ innego w handlu
	inv_trade_other = new Inventory;
	inv_trade_other->title = Inventory::txInventory;
	gp_trade->Add(inv_trade_other);

	gp_trade->Hide();

	// dziennik
	journal = new Journal;
	game_gui_container->Add(journal);

	// minimapa
	minimap = new Minimap;
	game_gui_container->Add(minimap);

	// game gui drugi raz
	game_gui_container->Add(game_gui);

	// main menu
	main_menu = new MainMenu;
	main_menu->event = DialogEvent(this, &Game::MainMenuEvent);
	main_menu->check_updates = check_updates;
	main_menu->skip_version = skip_version;
	main_menu->visible = false;
	GUI.Add(main_menu);

	// mapa œwiata
	world_map = new WorldMapGui;
	world_map->visible = false;
	world_map->mp_box = mp_box;
	world_map->journal = journal;
	GUI.Add(world_map);

	// wiadomoœci
	game_messages = new GameMessagesContainer;
	GUI.Add(game_messages);

	// konsola
	DialogInfo info;
	info.event = NULL;
	info.name = "console";
	info.parent = NULL;
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
	info.event = NULL;
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
void Game::NullGui2()
{
	game_gui_container = NULL;
	main_menu = NULL;
	world_map = NULL;
	gp_trade = NULL;
	inventory = NULL;
	inv_trade_mine = NULL;
	inv_trade_other = NULL;
	stats = NULL;
	team_panel = NULL;
	journal = NULL;
	minimap = NULL;
	game_gui = NULL;
	console = NULL;
	game_menu = NULL;
	options = NULL;
	saveload = NULL;
	create_character = NULL;
	multiplayer_panel = NULL;
	create_server_panel = NULL;
	pick_server_panel = NULL;
	server_panel = NULL;
	info_box = NULL;
	mp_box = NULL;
	load_screen = NULL;
	controls = NULL;
	game_messages = NULL;
}

//=================================================================================================
void Game::RemoveGui2()
{
	delete game_gui_container;
	delete main_menu;
	delete world_map;
	delete gp_trade;
	delete inventory;
	delete inv_trade_mine;
	delete inv_trade_other;
	delete stats;
	delete team_panel;
	delete journal;
	delete minimap;
	delete game_gui;
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
	delete mp_box;
	delete GetTextDialog::self;
	delete GetNumberDialog::self;
	delete load_screen;
	delete controls;
	delete game_messages;
}

//=================================================================================================
// Only for back compability, ignored now
void Game::LoadGui(File& f)
{
	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);
	INT2 prev_wnd_size, _pos, _size;
	f >> prev_wnd_size;
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		f >> _pos;
		f >> _size;
	}
}

//=================================================================================================
void Game::GetGamePanels(vector<GamePanel*>& panels)
{
	panels.push_back(inventory);
	panels.push_back(stats);
	panels.push_back(team_panel);
	panels.push_back(journal);
	panels.push_back(minimap);
	panels.push_back(inv_trade_mine);
	panels.push_back(inv_trade_other);
	panels.push_back(mp_box);
}

//=================================================================================================
// Clear gui state after new game/loading/entering new location
void Game::ClearGui()
{
	if(game_gui)
	{
		game_gui->Reset();
		game_messages->Reset();
	}
	if(mp_box)
		mp_box->visible = false;
}

//=================================================================================================
void Game::ShowPanel(OpenPanel to_open, OpenPanel open)
{
	if(open == OpenPanel::Unknown)
		open = GetOpenPanel();

	// close current panel
	switch(open)
	{
	case OpenPanel::None:
		break;
	case OpenPanel::Stats:
		stats->Hide();
		break;
	case OpenPanel::Inventory:
		inventory->Hide();
		break;
	case OpenPanel::Team:
		team_panel->Hide();
		break;
	case OpenPanel::Journal:
		journal->Hide();
		break;
	case OpenPanel::Minimap:
		minimap->Hide();
		break;
	case OpenPanel::Trade:
		OnCloseInventory();
		gp_trade->Hide();
		break;
	}

	// open new panel
	if(open != to_open)
	{
		switch(to_open)
		{
		case OpenPanel::Stats:
			stats->Show();
			break;
		case OpenPanel::Inventory:
			inventory->Show();
			break;
		case OpenPanel::Team:
			team_panel->Show();
			break;
		case OpenPanel::Journal:
			journal->Show();
			break;
		case OpenPanel::Minimap:
			minimap->Show();
			break;
		}
		open = to_open;
	}
	else
	{
		open = OpenPanel::None;
		game_gui->use_cursor = false;
	}
}

//=================================================================================================
OpenPanel Game::GetOpenPanel()
{
	if(stats->visible)
		return OpenPanel::Stats;
	else if(inventory->visible)
		return OpenPanel::Inventory;
	else if(team_panel->visible)
		return OpenPanel::Team;
	else if(journal->visible)
		return OpenPanel::Journal;
	else if(minimap->visible)
		return OpenPanel::Minimap;
	else if(gp_trade->visible)
		return OpenPanel::Trade;
	else
		return OpenPanel::None;
}

//=================================================================================================
void Game::PositionGui()
{
	float scale = float(GUI.wnd_size.x) / 1024;
	INT2 pos = INT2(int(scale*48), int(scale*32));
	INT2 size = INT2(GUI.wnd_size.x - pos.x*2, GUI.wnd_size.y - pos.x*2);

	stats->global_pos = stats->pos = pos;
	stats->size = size;
	team_panel->global_pos = team_panel->pos = pos;
	team_panel->size = size;
	inventory->global_pos = inventory->pos = pos;
	inventory->size = size;
	inv_trade_other->global_pos = inv_trade_other->pos = pos;
	inv_trade_other->size = INT2(size.x, (size.y-32)/2);
	inv_trade_mine->global_pos = inv_trade_mine->pos = INT2(pos.x, inv_trade_other->pos.y+inv_trade_other->size.y+16);
	inv_trade_mine->size = inv_trade_other->size;
	minimap->size = INT2(size.y, size.y);
	minimap->global_pos = minimap->pos = INT2((wnd_size.x-minimap->size.x)/2, (wnd_size.y-minimap->size.y)/2);
	journal->size = minimap->size;
	journal->global_pos = journal->pos = minimap->pos;
	mp_box->size = INT2((wnd_size.x-32)/2, (wnd_size.y-64)/4);
	mp_box->global_pos = mp_box->pos = INT2(wnd_size.x-pos.x-mp_box->size.x, wnd_size.y-pos.x-mp_box->size.y);

	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		(*it)->Event(GuiEvent_Moved);
		(*it)->Event(GuiEvent_Resize);
	}
}
