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

//=================================================================================================
GlobalGui::GlobalGui() : load_screen(nullptr), game_gui(nullptr), inventory(nullptr), stats(nullptr), team(nullptr),
journal(nullptr), minimap(nullptr), actions(nullptr), book(nullptr), messages(nullptr), mp_box(nullptr), world_map(nullptr), main_menu(nullptr),
console(nullptr), game_menu(nullptr), options(nullptr), saveload(nullptr), create_character(nullptr), multiplayer(nullptr), create_server(nullptr),
pick_server(nullptr), server(nullptr), info_box(nullptr), controls(nullptr), cursor_allow_move(true)
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
	delete GetTextDialog::self;
	delete GetNumberDialog::self;
	delete PickItemDialog::self;
	gui::PickFileDialog::Destroy();
}

//=================================================================================================
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

//=================================================================================================
void GlobalGui::InitOnce()
{
	Game& game = Game::Get();

	// game gui & panels
	game_gui = new GameGui;
	GUI.Add(game_gui);

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
	GUI.Add(world_map);

	// main menu
	main_menu = new MainMenu(&game);
	GUI.Add(main_menu);

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

	GUI.Add(this);
}

//=================================================================================================
void GlobalGui::LoadLanguage()
{
	GUI.SetText();

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
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("game_panel.png", GamePanel::tBackground);
	tex_mgr.AddLoadTask("dialog.png", Control::tDialog);
	tex_mgr.AddLoadTask("scrollbar.png", Scrollbar::tex);
	tex_mgr.AddLoadTask("scrollbar2.png", Scrollbar::tex2);
	tex_mgr.AddLoadTask("cursor.png", GUI.tCursor[CURSOR_NORMAL]);
	tex_mgr.AddLoadTask("hand.png", GUI.tCursor[CURSOR_HAND]);
	tex_mgr.AddLoadTask("text.png", GUI.tCursor[CURSOR_TEXT]);
	tex_mgr.AddLoadTask("button.png", Button::tex[Button::NONE]);
	tex_mgr.AddLoadTask("button_hover.png", Button::tex[Button::HOVER]);
	tex_mgr.AddLoadTask("button_down.png", Button::tex[Button::DOWN]);
	tex_mgr.AddLoadTask("button_disabled.png", Button::tex[Button::DISABLED]);
	tex_mgr.AddLoadTask("background.bmp", DialogBox::tBackground);
	tex_mgr.AddLoadTask("scrollbar.png", TextBox::tBox);
	tex_mgr.AddLoadTask("ticked.png", CheckBox::tTick);
	tex_mgr.AddLoadTask("box.png", IGUI::tBox);
	tex_mgr.AddLoadTask("box2.png", IGUI::tBox2);
	tex_mgr.AddLoadTask("pix.png", IGUI::tPix);
	tex_mgr.AddLoadTask("dialog_down.png", IGUI::tDown);
	tex_mgr.AddLoadTask("close.png", PickItemDialog::custom_x.tex[Button::NONE]);
	tex_mgr.AddLoadTask("close_hover.png", PickItemDialog::custom_x.tex[Button::HOVER]);
	tex_mgr.AddLoadTask("close_down.png", PickItemDialog::custom_x.tex[Button::DOWN]);
	tex_mgr.AddLoadTask("close_disabled.png", PickItemDialog::custom_x.tex[Button::DISABLED]);

	actions->LoadData();
	book->LoadData();
	console->LoadData();
	create_character->LoadData();
	game_gui->LoadData();
	game_menu->LoadData();
	inventory->LoadData();
	journal->LoadData();
	main_menu->LoadData();
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

	// load gui textures that require instant loading
	GUI.GetLayout()->LoadDefault();
}

//=================================================================================================
void GlobalGui::Cleanup()
{
	GUI.OnClean();
	delete this;
}

//=================================================================================================
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
		0, Color::Black, Rect(5, 5, 500, 200));
	assert(W.GetCurrentLocation() == L.location);
	assert(W.GetCurrentLocationIndex() == L.location_index);
}

//=================================================================================================
void GlobalGui::UpdateGui(float dt)
{
	GUI.Update(dt, cursor_allow_move ? Game::Get().settings.mouse_sensitivity_f : -1.f);
}

//=================================================================================================
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
		messages->Reset();
		if(mp_box && reset_mpbox)
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
	GUI.OnResize();
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
		GUI.cursor_pos = activation_point;
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
			GUI.GetDialog("dialog_alt_f4")->visible = false;
			Game::Get().Quit();
		}
	};
	di.type = DIALOG_YESNO;
	di.name = "dialog_alt_f4";
	di.parent = nullptr;
	di.order = ORDER_TOPMOST;
	di.pause = true;

	GUI.ShowDialog(di);
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
