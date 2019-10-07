#include "Pch.h"
#include "GameCore.h"
#include "GameGui.h"
#include "LoadScreen.h"
#include "DialogBox.h"
#include "LevelGui.h"
#include "GameMessages.h"
#include "MpBox.h"
#include "Inventory.h"
#include "StatsPanel.h"
#include "TeamPanel.h"
#include "Journal.h"
#include "Minimap.h"
#include "ActionPanel.h"
#include "BookPanel.h"
#include "WorldMapGui.h"
#include "MainMenu.h"
#include "Console.h"
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
#include "PickItemDialog.h"
#include "PickFileDialog.h"
#include "ResourceManager.h"
#include "Game.h"
#include "Language.h"
#include "PlayerInfo.h"
#include "Render.h"
#include "Notifications.h"
#include "LayoutLoader.h"

GameGui* global::game_gui;
FontPtr GameGui::font, GameGui::font_small, GameGui::font_big;
extern string g_system_dir;

//=================================================================================================
GameGui::GameGui() : load_screen(nullptr), level_gui(nullptr), inventory(nullptr), stats(nullptr), team(nullptr),
journal(nullptr), minimap(nullptr), actions(nullptr), book(nullptr), messages(nullptr), mp_box(nullptr), world_map(nullptr), main_menu(nullptr),
console(nullptr), game_menu(nullptr), options(nullptr), saveload(nullptr), create_character(nullptr), multiplayer(nullptr), create_server(nullptr),
pick_server(nullptr), server(nullptr), info_box(nullptr), controls(nullptr), cursor_allow_move(true), notifications(nullptr)
{
}

//=================================================================================================
GameGui::~GameGui()
{
	delete load_screen;
	delete level_gui;
	delete inventory;
	delete stats;
	delete team;
	delete journal;
	delete minimap;
	delete actions;
	delete book;
	delete messages;
	delete mp_box;
	delete world_map;
	delete main_menu;
	delete console;
	delete game_menu;
	delete options;
	delete saveload;
	delete create_character;
	delete multiplayer;
	delete create_server;
	delete pick_server;
	delete server;
	delete info_box;
	delete controls;
	delete notifications;
	delete GetTextDialog::self;
	delete GetNumberDialog::self;
	delete PickItemDialog::self;
	PickFileDialog::Destroy();
}

//=================================================================================================
void GameGui::PreInit()
{
	load_screen = new LoadScreen;
	gui->Add(load_screen);
}

//=================================================================================================
void GameGui::Init()
{
	// layout
	LayoutLoader* loader = new LayoutLoader(gui);
	Layout* layout = loader->LoadFromFile(Format("%s/layout.txt", g_system_dir.c_str()));
	font = loader->GetFont("normal");
	font_small = loader->GetFont("small");
	font_big = loader->GetFont("big");
	gui->SetLayout(layout);
	delete loader;

	// level gui & panels
	level_gui = new LevelGui;
	gui->Add(level_gui);

	mp_box = new MpBox;
	level_gui->Add(mp_box);

	inventory = new Inventory;
	inventory->InitOnce();
	level_gui->Add(inventory->inv_mine);
	level_gui->Add(inventory->gp_trade);

	stats = new StatsPanel;
	level_gui->Add(stats);

	team = new TeamPanel;
	level_gui->Add(team);

	journal = new Journal;
	level_gui->Add(journal);

	minimap = new Minimap;
	level_gui->Add(minimap);

	actions = new ActionPanel;
	level_gui->Add(actions);

	book = new BookPanel;
	level_gui->Add(book);

	messages = new GameMessages;

	// worldmap
	world_map = new WorldMapGui;
	gui->Add(world_map);

	// main menu
	main_menu = new MainMenu;
	gui->Add(main_menu);

	// notifications
	notifications = new Notifications;
	gui->Add(notifications);

	// dialogs
	DialogInfo info;
	info.event = nullptr;
	info.name = "console";
	info.parent = nullptr;
	info.pause = true;
	info.text = "";
	info.order = ORDER_TOPMOST;
	info.type = DIALOG_CUSTOM;
	console = new Console(info);

	info.name = "game_menu";
	info.order = ORDER_TOP;
	game_menu = new GameMenu(info);

	info.name = "options";
	options = new Options(info);

	info.name = "saveload";
	saveload = new SaveLoad(info);

	info.name = "create_character";
	info.event = DialogEvent(game, &Game::OnCreateCharacter);
	create_character = new CreateCharacterPanel(info);

	info.name = "multiplayer";
	info.event = DialogEvent(game, &Game::MultiplayerPanelEvent);
	multiplayer = new MultiplayerPanel(info);

	info.name = "create_server";
	info.event = DialogEvent(game, &Game::CreateServerEvent);
	create_server = new CreateServerPanel(info);

	info.name = "pick_server";
	info.event = DialogEvent(game, &Game::OnPickServer);
	pick_server = new PickServerPanel(info);

	info.name = "server";
	info.event = nullptr;
	server = new ServerPanel(info);

	info.name = "info_box";
	info_box = new InfoBox(info);

	info.name = "controls";
	info.parent = options;
	controls = new Controls(info);

	gui->Add(this);
}

//=================================================================================================
void GameGui::LoadLanguage()
{
	gui->SetText(Str("ok"), Str("yes"), Str("no"), Str("cancel"));

	txReallyQuit = Str("reallyQuit");

	actions->LoadLanguage();
	controls->LoadLanguage();
	create_character->LoadLanguage();
	create_server->LoadLanguage();
	level_gui->LoadLanguage();
	game_menu->LoadLanguage();
	inventory->LoadLanguage();
	journal->LoadLanguage();
	main_menu->LoadLanguage();
	messages->LoadLanguage();
	multiplayer->LoadLanguage();
	options->LoadLanguage();
	pick_server->LoadLanguage();
	saveload->LoadLanguage();
	stats->LoadLanguage();
	server->LoadLanguage();
	team->LoadLanguage();
	world_map->LoadLanguage();
}

//=================================================================================================
void GameGui::LoadData()
{
	GamePanel::tBackground = res_mgr->Load<Texture>("game_panel.png");
	GamePanel::tDialog = res_mgr->Load<Texture>("dialog.png");

	actions->LoadData();
	book->LoadData();
	console->LoadData();
	create_character->LoadData();
	level_gui->LoadData();
	game_menu->LoadData();
	inventory->LoadData();
	journal->LoadData();
	main_menu->LoadData();
	messages->LoadData();
	minimap->LoadData();
	pick_server->LoadData();
	server->LoadData();
	team->LoadData();
	world_map->LoadData();
}

//=================================================================================================
void GameGui::PostInit()
{
	create_character->Init();
	saveload->LoadSaveSlots();
}

//=================================================================================================
void GameGui::UpdateGui(float dt)
{
	gui->Update(dt, cursor_allow_move ? game->settings.mouse_sensitivity_f : -1.f);
}

//=================================================================================================
void GameGui::Save(FileWriter& f)
{
	messages->Save(f);
	level_gui->Save(f);
	journal->Save(f);
	world_map->Save(f);
}

//=================================================================================================
void GameGui::Load(FileReader& f)
{
	messages->Load(f);
	level_gui->Load(f);
	journal->Load(f);
	world_map->Load(f);
}

//=================================================================================================
// Clear gui state after new game/loading/entering new location
void GameGui::Clear(bool reset_mpbox, bool on_enter)
{
	if(level_gui)
	{
		level_gui->Reset();
		if(!on_enter)
			messages->Reset();
		if(mp_box && reset_mpbox && !net->mp_quickload)
			mp_box->visible = false;
	}
}

//=================================================================================================
void GameGui::Setup(PlayerController* pc)
{
	inventory->Setup(pc);
	stats->pc = pc;
}

//=================================================================================================
void GameGui::OnResize()
{
	gui->OnResize();
	if(level_gui)
		level_gui->PositionPanels();
	console->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void GameGui::OnFocus(bool focus, const Int2& activation_point)
{
	if(!focus && level_gui)
		level_gui->use_cursor = false;
	if(focus && activation_point.x != -1)
		gui->cursor_pos = activation_point;
}

//=================================================================================================
void GameGui::ShowMultiplayer()
{
	net->mp_load = false;
	multiplayer->Show();
}

//=================================================================================================
void GameGui::ShowQuitDialog()
{
	DialogInfo di;
	di.text = txReallyQuit;
	di.event = [](int id)
	{
		if(id == BUTTON_YES)
		{
			gui->GetDialog("dialog_alt_f4")->visible = false;
			game->Quit();
		}
	};
	di.type = DIALOG_YESNO;
	di.name = "dialog_alt_f4";
	di.parent = nullptr;
	di.order = ORDER_TOPMOST;
	di.pause = true;

	gui->ShowDialog(di);
}

//=================================================================================================
void GameGui::ShowCreateCharacterPanel(bool require_name, bool redo)
{
	if(redo)
	{
		PlayerInfo& info = net->GetMe();
		create_character->ShowRedo(info.clas, info.hd, info.cc);
	}
	else
		create_character->Show(require_name);
}

//=================================================================================================
void GameGui::CloseAllPanels(bool close_mp_box)
{
	if(level_gui)
		level_gui->ClosePanels(close_mp_box);
}

//=================================================================================================
void GameGui::AddMsg(cstring msg)
{
	assert(msg);
	if(server->visible)
		server->AddMsg(msg);
	else
		mp_box->itb.Add(msg);
}
