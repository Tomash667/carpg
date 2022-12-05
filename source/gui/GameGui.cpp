#include "Pch.h"
#include "GameGui.h"

#include "AbilityPanel.h"
#include "BookPanel.h"
#include "Console.h"
#include "Controls.h"
#include "CraftPanel.h"
#include "CreateCharacterPanel.h"
#include "CreateServerPanel.h"
#include "Game.h"
#include "GameMenu.h"
#include "GameMessages.h"
#include "InfoBox.h"
#include "Inventory.h"
#include "Journal.h"
#include "Language.h"
#include "LevelGui.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "Minimap.h"
#include "MpBox.h"
#include "MultiplayerPanel.h"
#include "Options.h"
#include "PickServerPanel.h"
#include "PlayerInfo.h"
#include "SaveLoadPanel.h"
#include "ServerPanel.h"
#include "StatsPanel.h"
#include "TeamPanel.h"
#include "WorldMapGui.h"

#include <DialogBox.h>
#include <GetNumberDialog.h>
#include <GetTextDialog.h>
#include <LayoutLoader.h>
#include <Notifications.h>
#include <PickFileDialog.h>
#include <PickItemDialog.h>
#include <Render.h>
#include <ResourceManager.h>

GameGui* gameGui;
FontPtr GameGui::font, GameGui::fontSmall, GameGui::fontBig;
extern string g_system_dir;

//=================================================================================================
GameGui::GameGui() : loadScreen(nullptr), levelGui(nullptr), inventory(nullptr), stats(nullptr), team(nullptr), journal(nullptr), minimap(nullptr),
ability(nullptr), book(nullptr), craft(nullptr), messages(nullptr), mpBox(nullptr), worldMap(nullptr), mainMenu(nullptr), console(nullptr), gameMenu(nullptr),
options(nullptr), saveload(nullptr), createCharacter(nullptr), multiplayer(nullptr), createServer(nullptr), pickServer(nullptr), server(nullptr),
infoBox(nullptr), controls(nullptr), cursorAllowMove(true), notifications(nullptr)
{
}

//=================================================================================================
GameGui::~GameGui()
{
	delete loadScreen;
	delete levelGui;
	delete inventory;
	delete stats;
	delete team;
	delete journal;
	delete minimap;
	delete ability;
	delete book;
	delete craft;
	delete messages;
	delete mpBox;
	delete worldMap;
	delete mainMenu;
	delete console;
	delete gameMenu;
	delete options;
	delete saveload;
	delete createCharacter;
	delete multiplayer;
	delete createServer;
	delete pickServer;
	delete server;
	delete infoBox;
	delete controls;
	delete notifications;
}

//=================================================================================================
void GameGui::PreInit()
{
	loadScreen = new LoadScreen;
	gui->Add(loadScreen);
}

//=================================================================================================
void GameGui::Init()
{
	// layout
	LayoutLoader* loader = new LayoutLoader(gui);
	Layout* layout = loader->LoadFromFile(Format("%s/layout.txt", g_system_dir.c_str()));
	font = loader->GetFont("normal");
	fontSmall = loader->GetFont("small");
	fontBig = loader->GetFont("big");
	gui->SetLayout(layout);
	delete loader;

	// level gui & panels
	levelGui = new LevelGui;
	gui->Add(levelGui);

	mpBox = new MpBox;
	levelGui->Add(mpBox);

	inventory = new Inventory;
	inventory->InitOnce();
	levelGui->Add(inventory->invMine);
	levelGui->Add(inventory->gpTrade);

	stats = new StatsPanel;
	levelGui->Add(stats);

	team = new TeamPanel;
	levelGui->Add(team);

	journal = new Journal;
	levelGui->Add(journal);

	minimap = new Minimap;
	levelGui->Add(minimap);

	ability = new AbilityPanel;
	levelGui->Add(ability);

	book = new BookPanel;
	levelGui->Add(book);

	craft = new CraftPanel;
	levelGui->Add(craft);

	messages = new GameMessages;

	// worldmap
	worldMap = new WorldMapGui;
	gui->Add(worldMap);

	// main menu
	mainMenu = new MainMenu;
	gui->Add(mainMenu);

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
	info.order = DialogOrder::TopMost;
	info.type = DIALOG_CUSTOM;
	console = new Console(info);

	info.name = "gameMenu";
	info.order = DialogOrder::Top;
	gameMenu = new GameMenu(info);

	info.name = "options";
	options = new Options(info);

	info.name = "saveload";
	saveload = new SaveLoad(info);

	info.name = "createCharacter";
	info.event = DialogEvent(game, &Game::OnCreateCharacter);
	createCharacter = new CreateCharacterPanel(info);

	info.name = "multiplayer";
	info.event = DialogEvent(game, &Game::MultiplayerPanelEvent);
	multiplayer = new MultiplayerPanel(info);

	info.name = "createServer";
	info.event = DialogEvent(game, &Game::CreateServerEvent);
	createServer = new CreateServerPanel(info);

	info.name = "pickServer";
	info.event = DialogEvent(game, &Game::OnPickServer);
	pickServer = new PickServerPanel(info);

	info.name = "server";
	info.event = nullptr;
	server = new ServerPanel(info);

	info.name = "infoBox";
	infoBox = new InfoBox(info);

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
	txReallyQuitHardcore = Str("reallyQuitHardcore");

	ability->LoadLanguage();
	controls->LoadLanguage();
	craft->LoadLanguage();
	createCharacter->LoadLanguage();
	createServer->LoadLanguage();
	levelGui->LoadLanguage();
	gameMenu->LoadLanguage();
	inventory->LoadLanguage();
	journal->LoadLanguage();
	mainMenu->LoadLanguage();
	messages->LoadLanguage();
	multiplayer->LoadLanguage();
	options->LoadLanguage();
	pickServer->LoadLanguage();
	saveload->LoadLanguage();
	stats->LoadLanguage();
	server->LoadLanguage();
	team->LoadLanguage();
	worldMap->LoadLanguage();
}

//=================================================================================================
void GameGui::LoadData()
{
	GamePanel::tBackground = resMgr->Load<Texture>("game_panel.png");
	GamePanel::tDialog = resMgr->Load<Texture>("dialog.png");

	ability->LoadData();
	book->LoadData();
	console->LoadData();
	craft->LoadData();
	createCharacter->LoadData();
	levelGui->LoadData();
	gameMenu->LoadData();
	inventory->LoadData();
	journal->LoadData();
	mainMenu->LoadData();
	messages->LoadData();
	minimap->LoadData();
	pickServer->LoadData();
	server->LoadData();
	team->LoadData();
	worldMap->LoadData();
}

//=================================================================================================
void GameGui::PostInit()
{
	createCharacter->Init();
	saveload->LoadSaveSlots();
	ChangeControls();
}

//=================================================================================================
void GameGui::Draw()
{
	Container::Draw();
}

//=================================================================================================
void GameGui::Draw(const Matrix& matViewProj, bool drawGui, bool drawDialogs)
{
	gui->mViewProj = matViewProj;
	gui->Draw(drawGui, drawDialogs);
}

//=================================================================================================
void GameGui::UpdateGui(float dt)
{
	// handle panels
	if(gui->HaveDialog() || (mpBox->visible && mpBox->itb.focus))
		GKey.allowInput = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game->gameState == GS_LEVEL && game->deathScreen == 0 && !game->dialogContext.dialogMode && !game->cutscene)
	{
		OpenPanel open = levelGui->GetOpenPanel(),
			to_open = OpenPanel::None;

		if(GKey.PressedRelease(GK_STATS))
			to_open = OpenPanel::Stats;
		else if(GKey.PressedRelease(GK_INVENTORY))
			to_open = OpenPanel::Inventory;
		else if(GKey.PressedRelease(GK_TEAM_PANEL))
			to_open = OpenPanel::Team;
		else if(GKey.PressedRelease(GK_JOURNAL))
			to_open = OpenPanel::Journal;
		else if(GKey.PressedRelease(GK_MINIMAP))
			to_open = OpenPanel::Minimap;
		else if(GKey.PressedRelease(GK_ABILITY_PANEL))
			to_open = OpenPanel::Ability;
		else if(open == OpenPanel::Trade && input->PressedRelease(Key::Escape))
			to_open = OpenPanel::Trade; // ShowPanel will hide when already opened

		if(to_open != OpenPanel::None)
			levelGui->ShowPanel(to_open, open);

		switch(open)
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(levelGui->useCursor)
				GKey.allowInput = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::Inventory:
		case OpenPanel::Team:
		case OpenPanel::Trade:
		case OpenPanel::Ability:
		case OpenPanel::Journal:
		case OpenPanel::Book:
		case OpenPanel::Craft:
			GKey.allowInput = GameKeys::ALLOW_KEYBOARD;
			break;
		}
	}

	const float mouseSpeed = cursorAllowMove ? Lerp(0.5f, 1.5f, float(game->settings.mouseSensitivity) / 100) : -1.f;
	gui->Update(dt, mouseSpeed);

	// handle blocking input by gui
	if(gui->HaveDialog() || (mpBox->visible && mpBox->itb.focus))
		GKey.allowInput = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game->gameState == GS_LEVEL && game->deathScreen == 0 && !game->dialogContext.dialogMode)
	{
		switch(levelGui->GetOpenPanel())
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(levelGui->useCursor)
				GKey.allowInput = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::Inventory:
		case OpenPanel::Team:
		case OpenPanel::Trade:
		case OpenPanel::Ability:
			GKey.allowInput = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Journal:
		case OpenPanel::Craft:
			GKey.allowInput = GameKeys::ALLOW_NONE;
			break;
		}
	}
	else
		GKey.allowInput = GameKeys::ALLOW_INPUT;

	// mp box
	if(game->gameState == GS_LEVEL)
	{
		if(GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
			mpBox->visible = !mpBox->visible;

		if(GKey.AllowKeyboard() && mpBox->visible && !mpBox->itb.focus && input->PressedRelease(Key::Enter))
		{
			mpBox->itb.focus = true;
			mpBox->Event(GuiEvent_GainFocus);
			mpBox->itb.Event(GuiEvent_GainFocus);
		}
	}
}

//=================================================================================================
void GameGui::Save(GameWriter& f)
{
	messages->Save(f);
	levelGui->Save(f);
	journal->Save(f);
	worldMap->Save(f);
}

//=================================================================================================
void GameGui::Load(GameReader& f)
{
	messages->Load(f);
	levelGui->Load(f);
	journal->Load(f);
	worldMap->Load(f);
}

//=================================================================================================
// Clear gui state after new game/loading/entering new location
void GameGui::Clear(bool resetMpBox, bool onEnter)
{
	if(levelGui)
	{
		levelGui->Reset();
		if(!onEnter)
			messages->Reset();
		if(mpBox && resetMpBox && !net->mpQuickload)
			mpBox->visible = false;
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
	if(levelGui)
		levelGui->PositionPanels();
	console->Event(GuiEvent_WindowResize);
	craft->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void GameGui::OnFocus(bool focus, const Int2& activationPoint)
{
	if(!focus && levelGui)
		levelGui->useCursor = false;
	if(focus && activationPoint.x != -1)
		gui->cursorPos = activationPoint;
}

//=================================================================================================
void GameGui::ShowMultiplayer()
{
	net->mpLoad = false;
	multiplayer->Show();
}

//=================================================================================================
void GameGui::ShowQuitDialog()
{
	DialogInfo di;
	di.text = game->hardcoreMode ? txReallyQuitHardcore : txReallyQuit;
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
	di.order = DialogOrder::TopMost;
	di.pause = true;

	gui->ShowDialog(di);
}

//=================================================================================================
void GameGui::ShowCreateCharacterPanel(bool enterName, bool redo)
{
	if(redo)
	{
		PlayerInfo& info = net->GetMe();
		createCharacter->ShowRedo(info.clas, info.hd, info.cc);
	}
	else
		createCharacter->Show(enterName);
}

//=================================================================================================
void GameGui::CloseAllPanels(bool closeMpBox)
{
	if(levelGui)
		levelGui->ClosePanels(closeMpBox);
}

//=================================================================================================
void GameGui::AddMsg(cstring msg)
{
	assert(msg);
	if(server->visible)
		server->AddMsg(msg);
	else
		mpBox->itb.Add(msg);
}

//=================================================================================================
void GameGui::ChangeControls()
{
	GameKey& accept = GKey[GK_ACCEPT_NOTIFICATION];
	GameKey& decline = GKey[GK_DECLINE_NOTIFICATION];
	notifications->SetShortcuts(accept.key, decline.key, controls->GetKeyText(accept.GetFirstKey()), controls->GetKeyText(decline.GetFirstKey()));
}
