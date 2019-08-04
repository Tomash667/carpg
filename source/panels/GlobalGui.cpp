#include "Pch.h"
#include "GameCore.h"
#include "GlobalGui.h"
#include "LoadScreen.h"
#include "GameDialogBox.h"
#include "GameGui.h"
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

GlobalGui* global::gui;
FontPtr GlobalGui::font, GlobalGui::font_small, GlobalGui::font_big;

//=================================================================================================
GlobalGui::GlobalGui() : load_screen(nullptr), game_gui(nullptr), inventory(nullptr), stats(nullptr), team(nullptr),
journal(nullptr), minimap(nullptr), actions(nullptr), book(nullptr), messages(nullptr), mp_box(nullptr), world_map(nullptr), main_menu(nullptr),
console(nullptr), game_menu(nullptr), options(nullptr), saveload(nullptr), create_character(nullptr), multiplayer(nullptr), create_server(nullptr),
pick_server(nullptr), server(nullptr), info_box(nullptr), controls(nullptr), cursor_allow_move(true), notifications(nullptr)
{
}

//=================================================================================================
GlobalGui::~GlobalGui()
{
	delete load_screen;
	delete game_gui;
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
void GlobalGui::Prepare()
{
	Game& game = Game::Get();
	GameDialogBox::game = &game;

	// load screen
	load_screen = new LoadScreen;
	gui->Add(load_screen);
}

//=================================================================================================
void GlobalGui::InitOnce()
{
	Game& game = Game::Get();

	// layout
	LayoutLoader* loader = new LayoutLoader(gui);
	Layout* layout = loader->LoadFromFile("../system/layout.txt");
	font = loader->GetFont("normal");
	font_small = loader->GetFont("small");
	font_big = loader->GetFont("big");
	gui->SetLayout(layout);
	delete loader;

	// game gui & panels
	game_gui = new GameGui;
	gui->Add(game_gui);

	mp_box = new MpBox;
	game_gui->Add(mp_box);

	inventory = new Inventory;
	inventory->InitOnce();
	game_gui->Add(inventory->inv_mine);
	game_gui->Add(inventory->gp_trade);

	stats = new StatsPanel;
	game_gui->Add(stats);

	team = new TeamPanel;
	game_gui->Add(team);

	journal = new Journal;
	game_gui->Add(journal);

	minimap = new Minimap;
	game_gui->Add(minimap);

	actions = new ActionPanel;
	game_gui->Add(actions);

	book = new BookPanel;
	game_gui->Add(book);

	messages = new GameMessages;

	// worldmap
	world_map = new WorldMapGui;
	gui->Add(world_map);

	// main menu
	main_menu = new MainMenu(&game);
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
	info.event = DialogEvent(&game, &Game::OnCreateCharacter);
	create_character = new CreateCharacterPanel(info);

	info.name = "multiplayer";
	info.event = DialogEvent(&game, &Game::MultiplayerPanelEvent);
	multiplayer = new MultiplayerPanel(info);

	info.name = "create_server";
	info.event = DialogEvent(&game, &Game::CreateServerEvent);
	create_server = new CreateServerPanel(info);

	info.name = "pick_server";
	info.event = DialogEvent(&game, &Game::OnPickServer);
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
void GlobalGui::LoadLanguage()
{
	gui->SetText(Str("ok"), Str("yes"), Str("no"), Str("cancel"));

	txReallyQuit = Str("reallyQuit");

	actions->LoadLanguage();
	controls->LoadLanguage();
	create_character->LoadLanguage();
	create_server->LoadLanguage();
	game_gui->LoadLanguage();
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
void GlobalGui::LoadData()
{
	ResourceManager& res_mgr = ResourceManager::Get();
	GamePanel::tBackground = res_mgr.Load<Texture>("game_panel.png");
	GamePanel::tDialog = res_mgr.Load<Texture>("dialog.png");

	actions->LoadData();
	book->LoadData();
	console->LoadData();
	create_character->LoadData();
	game_gui->LoadData();
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
void GlobalGui::PostInit()
{
	create_character->Init();
	saveload->LoadSaveSlots();
}

//=================================================================================================
void GlobalGui::Cleanup()
{
	gui->OnClean();
	delete this;
}

//=================================================================================================
void GlobalGui::Draw(ControlDrawData*)
{
	Container::Draw();
}

//=================================================================================================
void GlobalGui::Draw(const Matrix& mat_view_proj, bool draw_gui, bool draw_dialogs)
{
	gui->mViewProj = mat_view_proj;
	gui->Draw(draw_gui, draw_dialogs);
}

//=================================================================================================
void GlobalGui::UpdateGui(float dt)
{
	gui->Update(dt, cursor_allow_move ? Game::Get().settings.mouse_sensitivity_f : -1.f);
}

//=================================================================================================
void GlobalGui::Save(FileWriter& f)
{
	messages->Save(f);
	game_gui->Save(f);
	journal->Save(f);
	world_map->Save(f);
}

//=================================================================================================
void GlobalGui::Load(FileReader& f)
{
	messages->Load(f);
	game_gui->Load(f);
	journal->Load(f);
	world_map->Load(f);
}

//=================================================================================================
// Clear gui state after new game/loading/entering new location
void GlobalGui::Clear(bool reset_mpbox, bool on_enter)
{
	if(game_gui)
	{
		game_gui->Reset();
		if(!on_enter)
			messages->Reset();
		if(mp_box && reset_mpbox && !N.mp_quickload)
			mp_box->visible = false;
	}
}

//=================================================================================================
void GlobalGui::Setup(PlayerController* pc)
{
	inventory->Setup(pc);
	stats->pc = pc;
}

//=================================================================================================
void GlobalGui::OnResize()
{
	gui->OnResize();
	if(game_gui)
		game_gui->PositionPanels();
	console->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void GlobalGui::OnFocus(bool focus, const Int2& activation_point)
{
	if(!focus && game_gui)
		game_gui->use_cursor = false;
	if(focus && activation_point.x != -1)
		gui->cursor_pos = activation_point;
}

//=================================================================================================
void GlobalGui::ShowMultiplayer()
{
	N.mp_load = false;
	multiplayer->Show();
}

//=================================================================================================
void GlobalGui::ShowQuitDialog()
{
	DialogInfo di;
	di.text = txReallyQuit;
	di.event = [](int id)
	{
		if(id == BUTTON_YES)
		{
			gui->GetDialog("dialog_alt_f4")->visible = false;
			Game::Get().Quit();
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
void GlobalGui::ShowCreateCharacterPanel(bool require_name, bool redo)
{
	if(redo)
	{
		PlayerInfo& info = N.GetMe();
		create_character->ShowRedo(info.clas, info.hd, info.cc);
	}
	else
		create_character->Show(require_name);
}

//=================================================================================================
void GlobalGui::CloseAllPanels(bool close_mp_box)
{
	if(game_gui)
		game_gui->ClosePanels(close_mp_box);
}
