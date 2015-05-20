#include "Pch.h"
#include "Base.h"
#include "Game.h"

//=================================================================================================
void Game::OnResize()
{
	cursor_pos = VEC2(float(wnd_size.x)/2, float(wnd_size.y)/2);
	LoadGui();
	GUI.OnResize(wnd_size);
	console->Event(GuiEvent_WindowResize);
}

//=================================================================================================
// Aktualizuj gui
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

void Game::ShowGamePanel()
{
	BuildTmpInventory(0);
	gp_cont->Show();
}

bool Game::IsGamePanelOpen()
{
	return gp_cont->visible || main_menu->visible;
}

void Game::CloseGamePanels()
{
	gp_cont->Hide();
	minimap->visible = false;
	journal->visible = false;
}

#include "TextBox.h"

void Game::CreateGamePanels()
{
	GUI.Init(device, sprite);

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
	load_tasks.push_back(LoadTask("edit_gui.png", &GamePanel::tEdit[0]));
	load_tasks.push_back(LoadTask("edit_gui2.png", &GamePanel::tEdit[1]));
	load_tasks.push_back(LoadTask("dialog_down.png", &IGUI::tDown));
}

void Game::SetGamePanels()
{
	inv_trade_mine->i_items = inventory->i_items = &tmp_inventory[0];
	inv_trade_mine->items = inventory->items = &pc->unit->items;
	inv_trade_mine->slots = inventory->slots = pc->unit->slots;
	inv_trade_mine->unit = inventory->unit = pc->unit;
	inv_trade_other->i_items = &tmp_inventory[1];
	stats->pc = pc;
}

void Game::InitGui2()
{
	GUI.default_font = GUI.CreateFont("Arial", 12, 800, 512, 2);
	GUI.fBig = GUI.CreateFont("Florence Regular", 28, 800, 512);
	GUI.fSmall = GUI.CreateFont("Arial", 10, 500, 512);
	GUI.wnd_size = wnd_size;

	Inventory::game = this;
	Dialog::game = this;
	Journal::game = this;
	GamePanel::allow_move = false;

	/* UK£AD GUI
	GUI.layers
		load_screen
		game_gui_container
			game_gui
			mp_box
			gp_cont
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
	game_gui = new GameGui;
	game_gui_container->Add(game_gui);

	// mp box
	mp_box = new MpBox;
	mp_box->id = "mp_box";
	game_gui_container->Add(mp_box);

	// kontener na panele w TAB
	gp_cont = new GamePanelContainer;
	game_gui_container->Add(gp_cont);

	// ekwipunek
	Inventory::LoadText();
	inventory = new Inventory(INT2(396,301), INT2(627,433));
	inventory->id = "inventory";
	inventory->title = Inventory::txInventory;
	inventory->mode = Inventory::INVENTORY;
	gp_cont->Add(inventory);

	// statystyki
	stats = new StatsPanel(INT2(12,31), INT2(386, 704));
	stats->id = "stats";
	stats->min_size = INT2(211, 142);
	gp_cont->Add(stats);

	// dru¿yna
	team_panel = new TeamPanel(INT2(397,32), INT2(625,270));
	team_panel->id = "team";
	team_panel->game = this;
	team_panel->min_size = INT2(308, 193);
	gp_cont->Add(team_panel);

	gp_cont->Hide();

	// kontener na ekwipunek w handlu
	gp_trade = new GamePanelContainer;
	game_gui_container->Add(gp_trade);

	// ekwipunek gracza w handlu
	inv_trade_mine = new Inventory(INT2(16,332), INT2(600,300));
	inv_trade_mine->id = "inv_trade_mine";
	inv_trade_mine->title = Inventory::txInventory;
	inv_trade_mine->min_size = INT2(328, 224);
	inv_trade_mine->focus = true;
	gp_trade->Add(inv_trade_mine);

	// ekwipunek kogoœ innego w handlu
	inv_trade_other = new Inventory(INT2(16,16), INT2(600,300));
	inv_trade_other->id = "inv_trade_other";
	inv_trade_other->title = Inventory::txInventory;
	inv_trade_mine->min_size = INT2(328, 224);
	gp_trade->Add(inv_trade_other);

	gp_trade->Hide();

	// dziennik
	journal = new Journal;
	journal->id = "journal";
	journal->visible = false;
	journal->min_size = INT2(512,512);
	game_gui_container->Add(journal);

	// minimapa
	minimap = new Minimap;
	minimap->id = "minimap";
	minimap->visible = false;
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

	GamePanel::Init(inventory);
}

void Game::NullGui2()
{
	game_gui_container = NULL;
	main_menu = NULL;
	world_map = NULL;
	gp_cont = NULL;
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

void Game::RemoveGui2()
{
	delete game_gui_container;
	delete main_menu;
	delete world_map;
	delete gp_cont;
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

cstring Game::GetGuiSettingsPath(bool def, int w, int h)
{
	return Format("settings/%s/gui_%d_%d.txt", def ? "default" : "user", w, h);
}

void Game::SaveGui(bool save_def)
{
	cstring path = GetGuiSettingsPath(save_def, wnd_size.x, wnd_size.y);

	CreateDirectory("settings", NULL);

	if(save_def)
		CreateDirectory("settings/default", NULL);
	else
		CreateDirectory("settings/user", NULL);

	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);

	std::ofstream out(path);

	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		out << Format("panel \"%s\" {\n\tpos: %d %d\n\tsize: %d %d\n}\n\n", (*it)->id.c_str(), (*it)->pos.x, (*it)->pos.y, (*it)->size.x, (*it)->size.y);
	}
}

bool Game::LoadGui(bool def)
{
	cstring path = GetGuiSettingsPath(def, wnd_size.x, wnd_size.y);

	LocalString str;

	if(!LoadFileToString(path, str.get_ref()))
		return false;

	Tokenizer t;
	t.FromFile(path);
	t.AddKeyword("panel", 0);
	t.AddKeyword("pos", 1);
	t.AddKeyword("size", 2);

	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);

	try
	{
		while(1)
		{
			// panel
			t.Next();
			if(t.IsEof())
				break;
			t.AssertKeyword(0);

			// nazwa
			t.Next();
			t.AssertString();
			const string& s = t.GetString();
			GamePanel* gp = NULL;
			for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
			{
				if((*it)->id == s)
				{
					gp = *it;
					break;
				}
			}
			if(!gp)
				throw Format("Nieznany panel GUI '%s'.", s.c_str());

			// {
			t.Next();
			t.AssertSymbol('{');

			// pos
			t.Next();
			t.AssertKeyword(1);

			// :
			t.Next();
			t.AssertSymbol(':');

			// x
			t.Next();
			t.AssertInt();
			gp->pos.x = gp->global_pos.x = t.GetInt();

			// y
			t.Next();
			t.AssertInt();
			gp->pos.y = gp->global_pos.y = t.GetInt();

			// size
			t.Next();
			t.AssertKeyword(2);

			// :
			t.Next();
			t.AssertSymbol(':');

			// w
			t.Next();
			t.AssertInt();
			gp->size.x = t.GetInt();

			// h
			t.Next();
			t.AssertInt();
			gp->size.y = t.GetInt();

			// }
			t.Next();
			t.AssertSymbol('}');

			gp->Event(GuiEvent_Moved);
			gp->Event(GuiEvent_Resize);
		}
	}
	catch(cstring err)
	{
		ERROR(Format("B³¹d parsowania pliku '%s': %s", path, err));
		return false;
	}

	return true;
}

void Game::SaveGui(File& f)
{
	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);

	f << wnd_size;
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		f << (*it)->pos;
		f << (*it)->size;
	}
}

bool Game::LoadGui(File& f)
{
	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);
	INT2 prev_wnd_size;
	f >> prev_wnd_size;
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		if((f >> (*it)->pos) && (f >> (*it)->size))
		{
			(*it)->global_pos = (*it)->pos;
			(*it)->Event(GuiEvent_Moved);
			(*it)->Event(GuiEvent_Resize);
		}
		else
			return false;
	}

	if(prev_wnd_size != wnd_size)
		LoadGui();

	return true;
}

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

void Game::LoadGui()
{
	cstring path = GetGuiSettingsPath(false, wnd_size.x, wnd_size.y);
	if(!LoadGui(!FileExists(path)))
	{
		WARN(Format("GUI settings for resolution %dx%d don't exists. Generating default.", wnd_size.x, wnd_size.y));
		int part_x = (wnd_size.x-32)/5,
			part_y = (wnd_size.y-64)/5;
		stats->global_pos = stats->pos = INT2(16,32);
		stats->size = INT2(part_x*2,wnd_size.y-64);
		team_panel->global_pos = team_panel->pos = INT2(stats->pos.x+stats->size.x,32);
		team_panel->size = INT2(part_x*3,part_y*2);
		inventory->global_pos = inventory->pos = INT2(team_panel->pos.x,team_panel->pos.y+team_panel->size.y);
		inventory->size = INT2(part_x*3,part_y*3);
		inv_trade_other->global_pos = inv_trade_other->pos = INT2(16,32);
		inv_trade_other->size = INT2(wnd_size.x-32,(wnd_size.y-64)/2);
		inv_trade_mine->global_pos = inv_trade_mine->pos = INT2(16,32+inv_trade_other->size.y);
		inv_trade_mine->size = inv_trade_other->size;
		minimap->size = INT2(min(wnd_size.x-32,wnd_size.y-64));
		minimap->global_pos = minimap->pos = INT2((wnd_size.x-minimap->size.x)/2,(wnd_size.y-minimap->size.y)/2);
		journal->size = minimap->size;
		journal->global_pos = journal->pos = minimap->pos;
		mp_box->size = INT2((wnd_size.x-32)/2,(wnd_size.y-64)/4);
		mp_box->global_pos = mp_box->pos = INT2(wnd_size.x-16-mp_box->size.x, wnd_size.y-32-mp_box->size.y);

		LocalVector<GamePanel*> panels;
		GetGamePanels(panels);
		for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
		{
			(*it)->Event(GuiEvent_Moved);
			(*it)->Event(GuiEvent_Resize);
		}

		SaveGui(true);
	}
}
