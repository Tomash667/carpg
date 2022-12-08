#include "Pch.h"
#include "Game.h"

#include "Ability.h"
#include "AIController.h"
#include "AIManager.h"
#include "AITeam.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "BookPanel.h"
#include "CombatHelper.h"
#include "CommandParser.h"
#include "Console.h"
#include "Controls.h"
#include "CraftPanel.h"
#include "CreateServerPanel.h"
#include "DungeonGenerator.h"
#include "DungeonMeshBuilder.h"
#include "Electro.h"
#include "Encounter.h"
#include "EntityInterpolator.h"
#include "FOV.h"
#include "GameGui.h"
#include "GameMenu.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "InfoBox.h"
#include "Inventory.h"
#include "ItemContainer.h"
#include "ItemHelper.h"
#include "ItemScript.h"
#include "Journal.h"
#include "Language.h"
#include "Level.h"
#include "LevelGui.h"
#include "LevelPart.h"
#include "LoadScreen.h"
#include "LobbyApi.h"
#include "LocationGeneratorFactory.h"
#include "LocationHelper.h"
#include "MainMenu.h"
#include "Messenger.h"
#include "Minimap.h"
#include "MpBox.h"
#include "MultiInsideLocation.h"
#include "NameHelper.h"
#include "News.h"
#include "Pathfinding.h"
#include "PhysicCallbacks.h"
#include "PickServerPanel.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Crazies.h"
#include "Quest_Evil.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Scripted.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "RoomType.h"
#include "SaveSlot.h"
#include "ScriptManager.h"
#include "ServerPanel.h"
#include "SingleInsideLocation.h"
#include "Stock.h"
#include "Team.h"
#include "TeamPanel.h"
#include "UnitGroup.h"
#include "Var.h"
#include "Version.h"
#include "World.h"
#include "WorldMapGui.h"

#include <BasicShader.h>
#include <DirectX.h>
#include <Engine.h>
#include <GlowShader.h>
#include <GrassShader.h>
#include <Notifications.h>
#include <ParticleShader.h>
#include <ParticleSystem.h>
#include <PostfxShader.h>
#include <Render.h>
#include <RenderTarget.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneManager.h>
#include <SkyboxShader.h>
#include <SuperShader.h>
#include <Terrain.h>
#include <TerrainShader.h>
#include <Texture.h>
#include <SoundManager.h>

const float LIMIT_DT = 0.3f;
Game* game;
CustomCollisionWorld* phyWorld;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;
void HumanPredraw(void* ptr, Matrix* mat, int n);

//=================================================================================================
Game::Game() : quickstart(QUICKSTART_NONE), inactiveUpdate(false), lastScreenshot(0), drawParticleSphere(false), drawUnitRadius(false),
drawHitbox(false), noai(false), testing(false), gameSpeed(1.f), nextSeed(0), dontWander(false), checkUpdates(true), skipTutorial(false), portalAnim(0),
musicType(MusicType::Max), endOfGame(false), preparedStream(64 * 1024), paused(false), drawFlags(0xFFFFFFFF), prevGameState(GS_LOAD), rtSave(nullptr),
rtItemRot(nullptr), usePostfx(true), mpTimeout(10.f), screenshotFormat(ImageFormat::JPG), gameState(GS_LOAD), quickstartSlot(SaveSlot::MAX_SLOTS),
inLoad(false), tMinimap(nullptr)
{
	dialogContext.isLocal = true;
	uvMod = Terrain::DEFAULT_UV_MOD;

	aiMgr = new AIManager;
	arena = new Arena;
	cmdp = new CommandParser;
	dungeonMeshBuilder = new DungeonMeshBuilder;
	gameGui = new GameGui;
	gameLevel = new Level;
	gameRes = new GameResources;
	gameStats = new GameStats;
	locGenFactory = new LocationGeneratorFactory;
	messenger = new Messenger;
	net = new Net;
	pathfinding = new Pathfinding;
	questMgr = new QuestManager;
	scriptMgr = new ScriptManager;
	team = new Team;
	world = new World;
}

//=================================================================================================
Game::~Game()
{
}

//=================================================================================================
// Initialize game and show loadscreen.
//=================================================================================================
bool Game::OnInit()
{
	Info("Game: Initializing game.");
	gameState = GS_LOAD_MENU;

	try
	{
		// set everything needed to show loadscreen
		PreconfigureGame();

		// STEP 1 - load game system (units/items/etc)
		LoadSystem();

		// STEP 2 - load game content (meshes/textures/music/etc)
		LoadData();

		// configure game after loading and enter menu state
		PostconfigureGame();

		Info("Game: Game initialized.");
		return true;
	}
	catch(cstring err)
	{
		engine->ShowError(Format("Game: Failed to initialize game: %s", err), Logger::L_FATAL);
		return false;
	}
}

//=================================================================================================
// Preconfigure game vars.
//=================================================================================================
void Game::PreconfigureGame()
{
	Info("Game: Preconfiguring game.");

	phyWorld = engine->GetPhysicsWorld();
	engine->UnlockCursor(false);

	// set animesh callback
	MeshInstance::Predraw = HumanPredraw;

	gameGui->PreInit();

	PreloadLanguage();
	PreloadData();
	resMgr->SetProgressCallback(ProgressCallback(this, &Game::OnLoadProgress));
}

//=================================================================================================
// Load language strings showed on load screen.
//=================================================================================================
void Game::PreloadLanguage()
{
	Language::LoadFile("preload.txt");

	txPreloadAssets = Str("preloadAssets");
	txCreatingListOfFiles = Str("creatingListOfFiles");
	txConfiguringGame = Str("configuringGame");
	txLoadingAbilities = Str("loadingAbilities");
	txLoadingBuildings = Str("loadingBuildings");
	txLoadingClasses = Str("loadingClasses");
	txLoadingDialogs = Str("loadingDialogs");
	txLoadingItems = Str("loadingItems");
	txLoadingLocations = Str("loadingLocations");
	txLoadingMusics = Str("loadingMusics");
	txLoadingObjects = Str("loadingObjects");
	txLoadingPerks = Str("loadingPerks");
	txLoadingQuests = Str("loadingQuests");
	txLoadingRequired = Str("loadingRequired");
	txLoadingUnits = Str("loadingUnits");
	txLoadingShaders = Str("loadingShaders");
	txLoadingLanguageFiles = Str("loadingLanguageFiles");
}

//=================================================================================================
// Load data used by loadscreen.
//=================================================================================================
void Game::PreloadData()
{
	resMgr->AddDir("data/preload");

	GameGui::font = gameGui->gui->GetFont("Arial", 12, 8, 2);

	// loadscreen textures
	gameGui->loadScreen->LoadData();

	// intro music
	if(!soundMgr->IsDisabled())
	{
		MusicList* list = new MusicList;
		list->musics.push_back(resMgr->Load<Music>("Intro.ogg"));
		musicLists[(int)MusicType::Intro] = list;
		SetMusic(MusicType::Intro);
	}
}

//=================================================================================================
// Load system.
//=================================================================================================
void Game::LoadSystem()
{
	Info("Game: Loading system.");
	gameGui->loadScreen->Setup(0.f, 0.33f, 16, txCreatingListOfFiles);

	AddFilesystem();
	arena->Init();
	gameGui->Init();
	gameRes->Init();
	net->Init();
	questMgr->Init();
	scriptMgr->Init();
	gameGui->mainMenu->UpdateCheckVersion();
	LoadDatafiles();
	LoadLanguageFiles();
	gameGui->server->Init();
	gameGui->loadScreen->Tick(txLoadingShaders);
	ConfigureGame();
}

//=================================================================================================
// Add filesystem.
//=================================================================================================
void Game::AddFilesystem()
{
	Info("Game: Creating list of files.");
	resMgr->AddDir("data");
	resMgr->AddPak("data/data.pak", "KrystaliceFire");
}

//=================================================================================================
// Load datafiles (items/units/etc).
//=================================================================================================
void Game::LoadDatafiles()
{
	Info("Game: Loading system.");
	loadErrors = 0;
	loadWarnings = 0;

	// content
	content.systemDir = g_system_dir;
	content.LoadContent([this](Content::Id id)
	{
		switch(id)
		{
		case Content::Id::Abilities:
			gameGui->loadScreen->Tick(txLoadingAbilities);
			break;
		case Content::Id::Buildings:
			gameGui->loadScreen->Tick(txLoadingBuildings);
			break;
		case Content::Id::Classes:
			gameGui->loadScreen->Tick(txLoadingClasses);
			break;
		case Content::Id::Dialogs:
			gameGui->loadScreen->Tick(txLoadingDialogs);
			break;
		case Content::Id::Items:
			gameGui->loadScreen->Tick(txLoadingItems);
			break;
		case Content::Id::Locations:
			gameGui->loadScreen->Tick(txLoadingLocations);
			break;
		case Content::Id::Musics:
			gameGui->loadScreen->Tick(txLoadingMusics);
			break;
		case Content::Id::Objects:
			gameGui->loadScreen->Tick(txLoadingObjects);
			break;
		case Content::Id::Perks:
			gameGui->loadScreen->Tick(txLoadingPerks);
			break;
		case Content::Id::Quests:
			gameGui->loadScreen->Tick(txLoadingQuests);
			break;
		case Content::Id::Required:
			gameGui->loadScreen->Tick(txLoadingRequired);
			break;
		case Content::Id::Units:
			gameGui->loadScreen->Tick(txLoadingUnits);
			break;
		default:
			assert(0);
			break;
		}
	});
}

//=================================================================================================
// Load language files.
//=================================================================================================
void Game::LoadLanguageFiles()
{
	Info("Game: Loading language files.");
	gameGui->loadScreen->Tick(txLoadingLanguageFiles);

	Language::LoadLanguageFiles();

	txHaveErrors = Str("haveErrors");
	SetGameCommonText();
	SetItemStatsText();
	NameHelper::SetHeroNames();
	SetGameText();
	SetStatsText();
	arena->LoadLanguage();
	gameGui->LoadLanguage();
	gameLevel->LoadLanguage();
	gameRes->LoadLanguage();
	net->LoadLanguage();
	questMgr->LoadLanguage();
	world->LoadLanguage();

	uint language_errors = Language::GetErrors();
	if(language_errors != 0)
	{
		Error("Game: %u language errors.", language_errors);
		loadWarnings += language_errors;
	}
}

//=================================================================================================
// Initialize everything needed by game before loading content.
//=================================================================================================
void Game::ConfigureGame()
{
	Info("Game: Configuring game.");
	gameGui->loadScreen->Tick(txConfiguringGame);

	cmdp->AddCommands();
	settings.ResetGameKeys();
	settings.LoadGameKeys(cfg);
	loadErrors += BaseLocation::SetRoomPointers();

	// shaders
	render->RegisterShader(basicShader = new BasicShader);
	render->RegisterShader(grassShader = new GrassShader);
	render->RegisterShader(particleShader = new ParticleShader);
	render->RegisterShader(postfxShader = new PostfxShader);
	render->RegisterShader(skyboxShader = new SkyboxShader);
	render->RegisterShader(terrainShader = new TerrainShader);
	render->RegisterShader(glowShader = new GlowShader(postfxShader));

	tMinimap = render->CreateDynamicTexture(Int2(128));
	CreateRenderTargets();
}

//=================================================================================================
// Load game data.
//=================================================================================================
void Game::LoadData()
{
	Info("Game: Loading data.");

	resMgr->PrepareLoadScreen(0.33f);
	gameGui->loadScreen->Tick(txPreloadAssets);
	gameRes->LoadData();
	resMgr->StartLoadScreen();
}

//=================================================================================================
// Configure game after loading content.
//=================================================================================================
void Game::PostconfigureGame()
{
	Info("Game: Postconfiguring game.");

	engine->LockCursor();
	gameGui->PostInit();
	gameLevel->Init();
	locGenFactory->Init();
	world->Init();
	questMgr->InitLists();
	dungeonMeshBuilder->Build();

	// create save folders
	io::CreateDirectory("saves");
	io::CreateDirectory("saves/single");
	io::CreateDirectory("saves/multi");

	ItemScript::Init();

	// test & validate game data (in debug always check some things)
	if(testing)
		ValidateGameData(true);
	else if(IsDebug())
		ValidateGameData(false);

	// show errors notification
	bool start_game_mode = true;
	loadErrors += content.errors;
	loadWarnings += content.warnings;
	if(loadErrors > 0 || loadWarnings > 0)
	{
		// show message in release, notification in debug
		if(loadErrors > 0)
			Error("Game: %u loading errors, %u warnings.", loadErrors, loadWarnings);
		else
			Warn("Game: %u loading warnings.", loadWarnings);
		Texture* img = (loadErrors > 0 ? gameRes->tError : gameRes->tWarning);
		cstring text = Format(txHaveErrors, loadErrors, loadWarnings);
		if(IsDebug())
			gameGui->notifications->Add(text, img, 5.f);
		else
		{
			DialogInfo info;
			info.name = "have_errors";
			info.text = text;
			info.type = DIALOG_OK;
			info.img = img;
			info.event = [this](int result) { StartGameMode(); };
			info.parent = gameGui->mainMenu;
			info.order = DialogOrder::TopMost;
			info.pause = false;
			info.autoWrap = true;
			gui->ShowDialog(info);
			start_game_mode = false;
		}
	}

	// save config
	cfg.Add("adapter", render->GetAdapter());
	cfg.Add("resolution", engine->GetWindowSize());
	SaveCfg();

	// end load screen, show menu
	gameState = GS_MAIN_MENU;
	gameGui->loadScreen->visible = false;
	gameGui->mainMenu->Show();
	SetMusic(MusicType::Title);

	// start game mode if selected quickmode
	if(start_game_mode)
		StartGameMode();
}

//=================================================================================================
// Start quickmode if selected in config.
//=================================================================================================
void Game::StartGameMode()
{
	switch(quickstart)
	{
	case QUICKSTART_NONE:
		break;
	case QUICKSTART_SINGLE:
		StartQuickGame();
		break;
	case QUICKSTART_HOST:
	case QUICKSTART_LOAD_MP:
		if(playerName.empty())
		{
			Warn("Quickstart: Can't create server, no player nick.");
			break;
		}
		if(net->serverName.empty())
		{
			Warn("Quickstart: Can't create server, no server name.");
			break;
		}

		if(quickstart == QUICKSTART_LOAD_MP)
		{
			net->mpLoad = true;
			net->mpQuickload = false;
			if(TryLoadGame(quickstartSlot, false, false))
			{
				gameGui->createServer->CloseDialog();
				gameGui->server->autoready = true;
			}
			else
			{
				Error("Multiplayer quickload failed.");
				break;
			}
		}

		try
		{
			net->InitServer();
		}
		catch(cstring err)
		{
			gui->SimpleDialog(err, nullptr);
			break;
		}

		net->OnNewGameServer();
		break;
	case QUICKSTART_JOIN_LAN:
		if(!playerName.empty())
		{
			gameGui->server->autoready = true;
			gameGui->pickServer->Show(true);
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_JOIN_IP:
		if(!playerName.empty())
		{
			if(!serverIp.empty())
			{
				gameGui->server->autoready = true;
				QuickJoinIp();
			}
			else
				Warn("Quickstart: Can't join server, no server ip.");
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_LOAD:
		if(!TryLoadGame(quickstartSlot, false, false))
			Error("Quickload failed.");
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void Game::OnCleanup()
{
	if(gameState != GS_QUIT && gameState != GS_LOAD_MENU)
		ClearGame();

	if(gameGui && gameGui->mainMenu)
		gameGui->mainMenu->ShutdownThread();

	content.CleanupContent();

	ClearQuadtree();

	drawBatch.Clear();
	delete tMinimap;

	Language::Cleanup();

	delete aiMgr;
	delete arena;
	delete cmdp;
	delete dungeonMeshBuilder;
	delete gameGui;
	delete gameLevel;
	delete gameRes;
	delete gameStats;
	delete locGenFactory;
	delete messenger;
	delete net;
	delete pathfinding;
	delete questMgr;
	delete scriptMgr;
	delete team;
	delete world;
}

//=================================================================================================
void Game::OnDraw()
{
	DrawGame();
	render->Present();
}

//=================================================================================================
void Game::DrawGame()
{
	if(gameState == GS_LEVEL)
	{
		LocationPart& locPart = *pc->unit->locPart;
		gameLevel->camera.zfar = locPart.lvlPart->drawRange;
		ListDrawObjects(locPart, gameLevel->camera.frustum);

		vector<PostEffect> postEffects;
		GetPostEffects(postEffects);

		const bool usePostfx = !postEffects.empty();
		const bool useGlow = !drawBatch.glowNodes.empty();

		if(usePostfx || useGlow)
			postfxShader->SetTarget();

		Scene* scene = gameLevel->localPart->lvlPart->scene;
		render->Clear(scene->clearColor);

		// draw level
		DrawScene();

		// draw glow
		if(useGlow)
			glowShader->Draw(gameLevel->camera, drawBatch.glowNodes, usePostfx);

		if(usePostfx)
		{
			if(!useGlow)
				postfxShader->Prepare();
			postfxShader->Draw(postEffects);
		}

		if(drawBatch.tmpGlow)
			drawBatch.tmpGlow->tint = Vec4::One;
	}
	else
		render->Clear(Color::Black);

	// draw gui
	gameGui->Draw(gameLevel->camera.matViewProj, IsSet(drawFlags, DF_GUI), IsSet(drawFlags, DF_MENU));
}

//=================================================================================================
void HumanPredraw(void* ptr, Matrix* mat, int n)
{
	if(n != 0)
		return;

	Unit* u = static_cast<Unit*>(ptr);

	if(u->data->type == UNIT_TYPE::HUMAN)
	{
		int bone = u->meshInst->mesh->GetBone("usta")->id;
		static Matrix mat2;
		float val = u->talking ? sin(u->talkTimer * 6) : 0.f;
		mat[bone] = Matrix::RotationX(val / 5) * mat[bone];
	}
}

//=================================================================================================
void Game::OnUpdate(float dt)
{
	// check for memory corruption
	assert(_CrtCheckMemory());

	if(dt > LIMIT_DT)
		dt = LIMIT_DT;

	api->Update();
	scriptMgr->UpdateScripts(dt);

	if(Net::IsSingleplayer() || !paused)
	{
		// update time spent in game
		if(gameState != GS_MAIN_MENU && gameState != GS_LOAD)
			gameStats->Update(dt);
	}

	GKey.allowInput = GameKeys::ALLOW_INPUT;

	if(!engine->IsActive() || !engine->IsCursorLocked())
	{
		input->SetFocus(false);
		if(Net::IsSingleplayer() && !inactiveUpdate)
			return;
	}
	else
		input->SetFocus(true);

	// fast quit (alt+f4)
	if(input->Focus() && input->Shortcut(KEY_ALT, Key::F4) && !gui->HaveTopDialog("dialog_alt_f4"))
		gameGui->ShowQuitDialog();

	if(endOfGame)
	{
		deathFade += dt;
		GKey.allowInput = GameKeys::ALLOW_NONE;
	}

	// global keys handling
	if(GKey.allowInput == GameKeys::ALLOW_INPUT)
	{
		// handle open/close of console
		if(!gui->HaveTopDialog("dialog_alt_f4") && !gui->HaveDialog("console") && GKey.KeyDownUpAllowed(GK_CONSOLE))
			gui->ShowDialog(gameGui->console);

		// unlock cursor
		if(!engine->IsFullscreen() && engine->IsActive() && engine->IsCursorLocked() && input->Shortcut(KEY_CONTROL, Key::U))
			engine->UnlockCursor();

		// switch window mode
		if(input->Shortcut(KEY_ALT, Key::Enter))
			engine->SetFullscreen(!engine->IsFullscreen());

		// screenshot
		if(input->PressedRelease(Key::PrintScreen))
			TakeScreenshot(input->Down(Key::Shift));

		// pause/resume game
		if(Any(gameState, GS_LEVEL, GS_WORLDMAP) && GKey.KeyPressedReleaseAllowed(GK_PAUSE) && !Net::IsClient())
			PauseGame();

		if(!cutscene)
		{
			// quicksave, quickload
			bool consoleOpen = gui->HaveTopDialog("console");
			bool specialKeyAllowed = (GKey.allowInput == GameKeys::ALLOW_KEYBOARD || GKey.allowInput == GameKeys::ALLOW_INPUT || (!gui->HaveDialog() || consoleOpen));
			if(GKey.KeyPressedReleaseSpecial(GK_QUICKSAVE, specialKeyAllowed))
				Quicksave();
			if(GKey.KeyPressedReleaseSpecial(GK_QUICKLOAD, specialKeyAllowed))
				Quickload();
		}
	}

	// update game gui
	gameGui->UpdateGui(dt);
	if(gameState == GS_EXIT_TO_MENU)
	{
		ExitToMenu();
		return;
	}
	else if(gameState == GS_QUIT)
	{
		ClearGame();
		engine->Shutdown();
		return;
	}

	// open game menu
	if(GKey.AllowKeyboard() && CanShowMenu() && !cutscene && input->PressedRelease(Key::Escape))
		gameGui->ShowMenu();

	// update game
	if(!endOfGame && !cutscene)
	{
		arena->UpdatePvpRequest(dt);

		// update fallback, can leave level so we need to check if we are still in GS_LEVEL
		if(gameState == GS_LEVEL)
			UpdateFallback(dt);

		bool isPaused = false;

		if(gameState == GS_LEVEL)
		{
			isPaused = (paused || (gui->HavePauseDialog() && Net::IsSingleplayer()));
			if(!isPaused)
				UpdateGame(dt);
		}

		if(gameState == GS_LEVEL)
		{
			if(Net::IsLocal())
			{
				if(Net::IsOnline())
					net->UpdateWarpData(dt);
				gameLevel->ProcessUnitWarps();
			}
			gameLevel->camera.Update(isPaused ? 0.f : dt);
		}
	}
	else if(cutscene)
		UpdateFallback(dt);

	if(Net::IsOnline() && Any(gameState, GS_LEVEL, GS_WORLDMAP))
		UpdateGameNet(dt);

	messenger->Process();

	if(Net::IsSingleplayer() && gameState != GS_MAIN_MENU)
	{
		assert(Net::changes.empty());
	}
}

//=================================================================================================
void Game::SetTitle(cstring mode, bool initial)
{
	if(!initial && !changeTitle)
		return;

	LocalString s = "CaRpg " VERSION_STR;

	if(changeTitle)
	{
		bool none = true;

		if(IsDebug())
		{
			none = false;
			s += " - DEBUG";
		}

		if(mode)
		{
			if(none)
				s += " - ";
			else
				s += ", ";
			s += mode;
		}

		s += Format(" [%d]", GetCurrentProcessId());
	}

	SetConsoleTitle(s->c_str());
	engine->SetTitle(s->c_str());
}

//=================================================================================================
void Game::TakeScreenshot(bool noGui)
{
	if(noGui)
	{
		int old_flags = drawFlags;
		drawFlags = (0xFFFF & ~DF_GUI);
		DrawGame();
		drawFlags = old_flags;
	}
	else
		DrawGame();

	io::CreateDirectory("screenshots");

	time_t t = ::time(0);
	tm lt;
	localtime_s(&lt, &t);

	if(t == lastScreenshot)
		++screenshotCount;
	else
	{
		lastScreenshot = t;
		screenshotCount = 1;
	}

	cstring path = Format("screenshots\\%04d%02d%02d_%02d%02d%02d_%02d.%s", lt.tm_year + 1900, lt.tm_mon + 1,
		lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, screenshotCount, ImageFormatMethods::GetExtension(screenshotFormat));

	render->SaveScreenshot(path, screenshotFormat);

	cstring msg = Format("Screenshot saved to '%s'.", path);
	gameGui->console->AddMsg(msg);
	Info(msg);
}

//=================================================================================================
void Game::ExitToMenu()
{
	if(Net::IsOnline())
		CloseConnection(VoidF(this, &Game::DoExitToMenu));
	else
		DoExitToMenu();
}

//=================================================================================================
void Game::DoExitToMenu()
{
	prevGameState = gameState;
	gameState = GS_EXIT_TO_MENU;
	gameLevel->ready = false;

	StopAllSounds();
	ClearGame();

	if(resMgr->IsLoadScreen())
		resMgr->CancelLoadScreen(true);

	gameState = GS_MAIN_MENU;
	paused = false;
	net->mpLoad = false;
	net->wasClient = false;

	SetMusic(MusicType::Title);
	endOfGame = false;

	gameGui->CloseAllPanels();
	string msg;
	DialogBox* box = gui->GetDialog("fatal");
	bool console = gameGui->console->visible;
	if(box)
		msg = box->text;
	gui->CloseDialogs();
	if(!msg.empty())
		gui->SimpleDialog(msg.c_str(), nullptr, "fatal");
	if(console)
		gui->ShowDialog(gameGui->console);
	gameGui->gameMenu->visible = false;
	gameGui->levelGui->visible = false;
	gameGui->worldMap->Hide();
	gameGui->mainMenu->Show();
	unitsMeshLoad.clear();

	SetTitle(nullptr);
}

//=================================================================================================
void Game::LoadCfg()
{
	// last save
	lastSave = cfg.GetInt("lastSave", -1);
	if(lastSave < 1 || lastSave > SaveSlot::MAX_SLOTS)
		lastSave = -1;

	// quickstart
	if(quickstart == QUICKSTART_NONE)
	{
		const string& mode = cfg.GetString("quickstart");
		if(mode == "single")
			quickstart = QUICKSTART_SINGLE;
		else if(mode == "host")
			quickstart = QUICKSTART_HOST;
		else if(mode == "join")
			quickstart = QUICKSTART_JOIN_LAN;
		else if(mode == "joinip")
			quickstart = QUICKSTART_JOIN_IP;
		else if(mode == "load")
			quickstart = QUICKSTART_LOAD;
		else if(mode == "loadmp")
			quickstart = QUICKSTART_LOAD_MP;
	}
	int slot = cfg.GetInt("loadslot", -1);
	if(slot >= 1 && slot <= SaveSlot::MAX_SLOTS)
		quickstartSlot = slot;

	// multiplayer mode
	playerName = Truncate(cfg.GetString("nick"), 16);
	net->serverName = Truncate(cfg.GetString("serverName"), 16);
	net->password = Truncate(cfg.GetString("serverPswd"), 16);
	net->maxPlayers = Clamp(cfg.GetUint("serverPlayers", DEFAULT_PLAYERS), MIN_PLAYERS, MAX_PLAYERS);
	serverIp = cfg.GetString("serverIp");
	mpTimeout = Clamp(cfg.GetFloat("timeout", 10.f), 1.f, 3600.f);
	net->serverLan = cfg.GetBool("serverLan");
	net->joinLan = cfg.GetBool("joinLan");
	net->port = Clamp(cfg.GetInt("port", PORT), 0, 0xFFFF);

	// miscellaneous
	changeTitle = cfg.GetBool("changeTitle");
	checkUpdates = cfg.GetBool("checkUpdates", true);
	defaultDevmode = cfg.GetBool("devmode", IsDebug());
	devmode = defaultDevmode;
	defaultPlayerDevmode = cfg.GetBool("playersDevmode", IsDebug());
	hardcoreOption = cfg.GetBool("hardcoreOption");
	inactiveUpdate = cfg.GetBool("inactiveUpdate");
	skipTutorial = cfg.GetBool("skipTutorial");
	testing = cfg.GetBool("test");
	useGlow = cfg.GetBool("useGlow", true);
	usePostfx = cfg.GetBool("usePostfx", true);
	settings.grassRange = max(cfg.GetFloat("grassRange", 40.f), 0.f);
	settings.mouseSensitivity = Clamp(cfg.GetInt("mouseSensitivity", 50), 0, 100);
	screenshotFormat = ImageFormatMethods::FromString(cfg.GetString("screenshotFormat", "jpg"));
	if(screenshotFormat == ImageFormat::Invalid)
	{
		Warn("Settings: Unknown screenshot format '%s'. Defaulting to jpg.", cfg.GetString("screenshotFormat").c_str());
		screenshotFormat = ImageFormat::JPG;
	}

	SetTitle(nullptr, true);
}

//=================================================================================================
void Game::SaveCfg()
{
	if(!cfg.Save())
		Error("Failed to save configuration file '%s'!", cfg.GetFileName().c_str());
}

//=================================================================================================
void Game::CreateRenderTargets()
{
	rtSave = render->CreateRenderTarget(Int2(256, 256));
	rtItemRot = render->CreateRenderTarget(Int2(128, 128));
}

//=================================================================================================
void Game::RestartGame()
{
	// create mutex
	HANDLE mutex = CreateMutex(nullptr, TRUE, RESTART_MUTEX_NAME);
	DWORD lastError = GetLastError();
	bool alreadyRunning = (lastError == ERROR_ALREADY_EXISTS || lastError == ERROR_ACCESS_DENIED);
	if(alreadyRunning)
	{
		WaitForSingleObject(mutex, INFINITE);
		CloseHandle(mutex);
		return;
	}

	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi = { 0 };

	// append -restart to cmdline, hopefuly noone will use it 100 times in a row to overflow Format
	string cmdLine = GetCommandLine();
	if(!EndsWith(cmdLine, "-restart"))
		cmdLine += " -restart";
	CreateProcess(nullptr, (char*)cmdLine.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

	Quit();
}

//=================================================================================================
void Game::SetStatsText()
{
	Language::Section s = Language::GetSection("WeaponTypes");
	WeaponTypeInfo::info[WT_SHORT_BLADE].name = s.Get("shortBlade");
	WeaponTypeInfo::info[WT_LONG_BLADE].name = s.Get("longBlade");
	WeaponTypeInfo::info[WT_BLUNT].name = s.Get("blunt");
	WeaponTypeInfo::info[WT_AXE].name = s.Get("axe");
}

//=================================================================================================
void Game::SetGameText()
{
	// ai
	StrArray(txAiNoHpPot, "aiNoHpPot");
	StrArray(txAiNoMpPot, "aiNoMpPot");
	StrArray(txAiCity, "aiCity");
	StrArray(txAiVillage, "aiVillage");
	txAiForest = Str("aiForest");
	txAiMoonwell = Str("aiMoonwell");
	txAiAcademy = Str("aiAcademy");
	txAiCampEmpty = Str("aiCampEmpty");
	txAiCampFull = Str("aiCampFull");
	txAiFort = Str("aiFort");
	txAiDwarfFort = Str("aiDwarfFort");
	txAiTower = Str("aiTower");
	txAiArmory = Str("aiArmory");
	txAiHideout = Str("aiHideout");
	txAiVault = Str("aiVault");
	txAiCrypt = Str("aiCrypt");
	txAiTemple = Str("aiTemple");
	txAiNecromancerBase = Str("aiNecromancerBase");
	txAiLabyrinth = Str("aiLabyrinth");
	txAiNoEnemies = Str("aiNoEnemies");
	txAiNearEnemies = Str("aiNearEnemies");
	txAiCave = Str("aiCave");

	// world
	txEnteringLocation = Str("enteringLocation");
	txGeneratingMap = Str("generatingMap");
	txGeneratingBuildings = Str("generatingBuildings");
	txGeneratingObjects = Str("generatingObjects");
	txGeneratingUnits = Str("generatingUnits");
	txGeneratingItems = Str("generatingItems");
	txGeneratingPhysics = Str("generatingPhysics");
	txRecreatingObjects = Str("recreatingObjects");
	txGeneratingMinimap = Str("generatingMinimap");
	txLoadingComplete = Str("loadingComplete");
	txWaitingForPlayers = Str("waitingForPlayers");
	txLoadingResources = Str("loadingResources");

	txTutPlay = Str("tutPlay");
	txTutTick = Str("tutTick");

	txCantSaveGame = Str("cantSaveGame");
	txSaveFailed = Str("saveFailed");
	txLoadFailed = Str("loadFailed");
	txQuickSave = Str("quickSave");
	txGameSaved = Str("gameSaved");
	txLoadingData = Str("loadingData");
	txEndOfLoading = Str("endOfLoading");
	txCantSaveNow = Str("cantSaveNow");
	txOnlyServerCanSave = Str("onlyServerCanSave");
	txCantLoadGame = Str("cantLoadGame");
	txOnlyServerCanLoad = Str("onlyServerCanLoad");
	txLoadSignature = Str("loadSignature");
	txLoadVersion = Str("loadVersion");
	txLoadSaveVersionOld = Str("loadSaveVersionOld");
	txLoadMP = Str("loadMP");
	txLoadSP = Str("loadSP");
	txLoadOpenError = Str("loadOpenError");
	txCantLoadMultiplayer = Str("cantLoadMultiplayer");
	txTooOldVersion = Str("tooOldVersion");
	txMissingPlayerInSave = Str("missingPlayerInSave");
	txGameLoaded = Str("gameLoaded");
	txLoadError = Str("loadError");
	txLoadErrorGeneric = Str("loadErrorGeneric");
	txMissingQuicksave = Str("missingQuicksave");

	txPvpRefuse = Str("pvpRefuse");
	txWin = Str("win");
	txWinHardcore = Str("winHardcore");
	txWinMp = Str("winMp");
	txLevelUp = Str("levelUp");
	txLevelDown = Str("levelDown");
	txRegeneratingLevel = Str("regeneratingLevel");
	txNeedItem = Str("needItem");

	// rumors
	StrArray(txRumor, "rumor_");
	StrArray(txRumorD, "rumor_d_");

	// dialogs 1
	StrArray(txQuestAlreadyGiven, "questAlreadyGiven");
	StrArray(txMayorNoQ, "mayorNoQ");
	StrArray(txCaptainNoQ, "captainNoQ");
	StrArray(txLocationDiscovered, "locationDiscovered");
	StrArray(txAllDiscovered, "allDiscovered");
	StrArray(txCampDiscovered, "campDiscovered");
	StrArray(txAllCampDiscovered, "allCampDiscovered");
	StrArray(txNoQRumors, "noQRumors");
	txNeedMoreGold = Str("needMoreGold");
	txNoNearLoc = Str("noNearLoc");
	txNearLoc = Str("nearLoc");
	StrArray(txNearLocEmpty, "nearLocEmpty");
	txNearLocCleared = Str("nearLocCleared");
	StrArray(txNearLocEnemy, "nearLocEnemy");
	StrArray(txNoNews, "noNews");
	StrArray(txAllNews, "allNews");
	txAllNearLoc = Str("allNearLoc");
	txLearningPoint = Str("learningPoint");
	txLearningPoints = Str("learningPoints");
	txNeedLearningPoints = Str("needLearningPoints");
	txTeamTooBig = Str("teamTooBig");
	txHeroJoined = Str("heroJoined");
	txCantLearnAbility = Str("cantLearnAbility");
	txSpell = Str("spell");
	txAbility = Str("ability");
	txCantLearnSkill = Str("cantLearnSkill");

	// location distance/strength
	txNear = Str("near");
	txFar = Str("far");
	txVeryFar = Str("veryFar");
	StrArray(txELvlVeryWeak, "eLvlVeryWeak");
	StrArray(txELvlWeak, "eLvlWeak");
	StrArray(txELvlAverage, "eLvlAverage");
	StrArray(txELvlQuiteStrong, "eLvlQuiteStrong");
	StrArray(txELvlStrong, "eLvlStrong");

	// quests
	txMineBuilt = Str("mineBuilt");
	txAncientArmory = Str("ancientArmory");
	txPortalClosed = Str("portalClosed");
	txPortalClosedNews = Str("portalClosedNews");
	txHiddenPlace = Str("hiddenPlace");
	txOrcCamp = Str("orcCamp");
	txPortalClose = Str("portalClose");
	txPortalCloseLevel = Str("portalCloseLevel");
	txXarDanger = Str("xarDanger");
	txGorushDanger = Str("gorushDanger");
	txGorushCombat = Str("gorushCombat");
	txMageHere = Str("mageHere");
	txMageEnter = Str("mageEnter");
	txMageFinal = Str("mageFinal");

	// menu net
	txEnterIp = Str("enterIp");
	txConnecting = Str("connecting");
	txInvalidIp = Str("invalidIp");
	txWaitingForPswd = Str("waitingForPswd");
	txEnterPswd = Str("enterPswd");
	txConnectingTo = Str("connectingTo");
	txConnectingProxy = Str("connectingProxy");
	txConnectTimeout = Str("connectTimeout");
	txConnectInvalid = Str("connectInvalid");
	txConnectVersion = Str("connectVersion");
	txConnectSLikeNet = Str("connectSLikeNet");
	txCantJoin = Str("cantJoin");
	txLostConnection = Str("lostConnection");
	txInvalidPswd = Str("invalidPswd");
	txCantJoin2 = Str("cantJoin2");
	txServerFull = Str("serverFull");
	txInvalidData = Str("invalidData");
	txNickUsed = Str("nickUsed");
	txInvalidVersion = Str("invalidVersion");
	txInvalidVersion2 = Str("invalidVersion2");
	txInvalidNick = Str("invalidNick");
	txGeneratingWorld = Str("generatingWorld");
	txLoadedWorld = Str("loadedWorld");
	txWorldDataError = Str("worldDataError");
	txLoadedPlayer = Str("loadedPlayer");
	txPlayerDataError = Str("playerDataError");
	txGeneratingLocation = Str("generatingLocation");
	txLoadingLocation = Str("loadingLocation");
	txLoadingLocationError = Str("loadingLocationError");
	txLoadingChars = Str("loadingChars");
	txLoadingCharsError = Str("loadingCharsError");
	txSendingWorld = Str("sendingWorld");
	txMpNPCLeft = Str("mpNPCLeft");
	txLoadingLevel = Str("loadingLevel");
	txDisconnecting = Str("disconnecting");
	txPreparingWorld = Str("preparingWorld");
	txInvalidCrc = Str("invalidCrc");
	txConnectionFailed = Str("connectionFailed");
	txLoadingSaveByServer = Str("loadingSaveByServer");
	txServerFailedToLoadSave = Str("serverFailedToLoadSave");

	// net
	txYouAreLeader = Str("youAreLeader");
	txRolledNumber = Str("rolledNumber");
	txPcIsLeader = Str("pcIsLeader");
	txReceivedGold = Str("receivedGold");
	txYouDisconnected = Str("youDisconnected");
	txYouKicked = Str("youKicked");
	txGamePaused = Str("gamePaused");
	txGameResumed = Str("gameResumed");
	txDevmodeOn = Str("devmodeOn");
	txDevmodeOff = Str("devmodeOff");
	txPlayerDisconnected = Str("playerDisconnected");
	txPlayerQuit = Str("playerQuit");
	txPlayerKicked = Str("playerKicked");
	txServerClosed = Str("serverClosed");

	// yell text
	StrArray(txYell, "yell");
}

//=================================================================================================
void Game::GetPostEffects(vector<PostEffect>& postEffects)
{
	postEffects.clear();
	if(!usePostfx)
		return;

	// vignette
	PostEffect effect;
	effect.id = POSTFX_VIGNETTE;
	effect.power = 1.f;
	effect.skill = Vec4(0.9f, 0.9f, 0.9f, 1.f);
	effect.tex = gameRes->tVignette->tex;
	postEffects.push_back(effect);

	// gray effect
	if(pc->data.grayout > 0.f)
	{
		PostEffect effect;
		effect.id = POSTFX_MONOCHROME;
		effect.power = pc->data.grayout;
		effect.skill = Vec4::Zero;
		postEffects.push_back(effect);
	}

	// drunk effect
	float drunk = pc->unit->alcohol / pc->unit->hpmax;
	if(drunk > 0.1f)
	{
		PostEffect effect;
		effect.id = POSTFX_BLUR_X;
		// 0.1-0.5 - 1
		// 1 - 2
		float mod;
		if(drunk < 0.5f)
			mod = 1.f;
		else
			mod = 1.f + (drunk - 0.5f) * 2;
		effect.skill = Vec4(1.f / engine->GetClientSize().x * mod, 1.f / engine->GetClientSize().y * mod, 0, 0);
		// 0.1-0
		// 1-1
		effect.power = (drunk - 0.1f) / 0.9f;
		postEffects.push_back(effect);

		effect.id = POSTFX_BLUR_Y;
		postEffects.push_back(effect);
	}
}

//=================================================================================================
uint Game::ValidateGameData(bool major)
{
	Info("Test: Validating game data...");

	uint err = TestGameData(major);

	UnitData::Validate(err);
	Attribute::Validate(err);
	Skill::Validate(err);
	Item::Validate(err);
	Perk::Validate(err);

	if(major)
		err += resMgr->VerifyResources();

	if(err == 0)
		Info("Test: Validation succeeded.");
	else
		Error("Test: Validation failed, %u errors found.", err);

	content.warnings += err;
	return err;
}

cstring Game::GetShortcutText(GAME_KEYS key, cstring action)
{
	auto& k = GKey[key];
	bool first_key = (k[0] != Key::None),
		second_key = (k[1] != Key::None);
	if(!action)
		action = k.text;

	if(first_key && second_key)
		return Format("%s (%s, %s)", action, gameGui->controls->GetKeyText(k[0]), gameGui->controls->GetKeyText(k[1]));
	else if(first_key || second_key)
	{
		Key used = k[first_key ? 0 : 1];
		return Format("%s (%s)", action, gameGui->controls->GetKeyText(used));
	}
	else
		return action;
}

void Game::PauseGame()
{
	paused = !paused;
	if(Net::IsOnline())
	{
		gameGui->mpBox->Add(paused ? txGamePaused : txGameResumed);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PAUSED;
		c.id = (paused ? 1 : 0);
		if(paused && gameState == GS_WORLDMAP && world->GetState() == World::State::TRAVEL)
			Net::PushChange(NetChange::UPDATE_MAP_POS);
	}
}

void Game::GenerateWorld()
{
	if(nextSeed != 0)
	{
		Srand(nextSeed);
		nextSeed = 0;
	}

	Info("Generating world, seed %u.", RandVal());

	world->GenerateWorld();

	Info("Randomness integrity: %d", RandVal());
}

void Game::EnterLocation(int level, int fromPortal, bool closePortal)
{
	Location& l = *gameLevel->location;
	gameLevel->lvl = nullptr;

	if(level == -2)
		level = (l.outside ? -1 : 0);

	gameGui->worldMap->Hide();
	gameGui->levelGui->Reset();
	gameGui->levelGui->visible = true;

	gameLevel->isOpen = true;
	if(world->GetState() != World::State::INSIDE_ENCOUNTER)
		world->SetState(World::State::INSIDE_LOCATION);
	if(fromPortal != -1)
		gameLevel->enterFrom = ENTER_FROM_PORTAL + fromPortal;
	else
		gameLevel->enterFrom = ENTER_FROM_OUTSIDE;
	gameLevel->lightAngle = Random(PI * 2);

	gameLevel->dungeonLevel = level;
	gameLevel->eventHandler = nullptr;
	pc->data.beforePlayer = BP_NONE;
	arena->Reset();
	gameGui->inventory->lock = nullptr;

	bool first = false;

	if(l.lastVisit == -1)
		first = true;

	InitQuadTree();

	if(Net::IsOnline() && net->activePlayers > 1)
	{
		BitStreamWriter f;
		f << ID_CHANGE_LEVEL;
		f << (byte)gameLevel->locationIndex;
		f << (byte)0;
		f << (world->GetState() == World::State::INSIDE_ENCOUNTER);
		int ack = net->SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
		for(PlayerInfo& info : net->players)
		{
			if(info.id == team->myId)
				info.state = PlayerInfo::IN_GAME;
			else
			{
				info.state = PlayerInfo::WAITING_FOR_RESPONSE;
				info.ack = ack;
				info.timer = 5.f;
			}
		}
		net->FilterServerChanges();
	}

	// calculate number of loading steps for drawing progress bar
	LocationGenerator* locGen = locGenFactory->Get(&l, first);
	int steps = locGen->GetNumberOfSteps();
	LoadingStart(steps);
	LoadingStep(txEnteringLocation);

	// generate map on first visit
	if(first)
	{
		if(nextSeed != 0)
		{
			Srand(nextSeed);
			nextSeed = 0;
		}

		// log what is required to generate location for testing
		if(l.type == L_CITY)
		{
			City* city = (City*)&l;
			Info("Generating location '%s', seed %u [%d].", l.name.c_str(), RandVal(), city->citizens);
		}
		else
			Info("Generating location '%s', seed %u.", l.name.c_str(), RandVal());
		l.seed = RandVal();
		if(!l.outside)
		{
			InsideLocation* inside = static_cast<InsideLocation*>(gameLevel->location);
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
				multi->infos[gameLevel->dungeonLevel].seed = l.seed;
			}
		}

		// mark as visited
		if(l.state != LS_HIDDEN)
			l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		locGen->Generate();
	}
	else if(!Any(l.type, L_DUNGEON, L_CAVE))
		Info("Entering location '%s'.", l.name.c_str());

	if(gameLevel->location->outside)
	{
		locGen->OnEnter();
		SetTerrainTextures();
		CalculateQuadtree();
	}
	else
		EnterLevel(locGen);

	bool loaded_resources = gameLevel->location->RequireLoadingResources(nullptr);
	LoadResources(txLoadingComplete, false);

	l.lastVisit = world->GetWorldtime();
	gameLevel->CheckIfLocationCleared();
	gameLevel->camera.Reset();
	pc->data.rotBuf = 0.f;
	SetMusic();

	if(closePortal)
	{
		delete gameLevel->location->portal;
		gameLevel->location->portal = nullptr;
	}

	if(gameLevel->location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	if(Net::IsOnline())
	{
		netMode = NM_SERVER_SEND;
		netState = NetState::Server_Send;
		if(net->activePlayers > 1)
		{
			preparedStream.Reset();
			BitStreamWriter f(preparedStream);
			net->WriteLevelData(f, loaded_resources);
			Info("Generated location packet: %d.", preparedStream.GetNumberOfBytesUsed());
		}
		else
			net->GetMe().state = PlayerInfo::IN_GAME;

		gameGui->infoBox->Show(txWaitingForPlayers);
	}
	else
	{
		gameState = GS_LEVEL;
		gameGui->loadScreen->visible = false;
		gameGui->mainMenu->visible = false;
		gameGui->levelGui->visible = true;
		gameLevel->ready = true;
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");
}

void Game::LeaveLocation(bool clear, bool takesTime)
{
	if(!gameLevel->isOpen)
		return;

	if(Net::IsLocal() && !net->wasClient && !clear)
	{
		if(questMgr->questTournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			questMgr->questTournament->Clean();
		if(!arena->free)
			arena->Clean();
	}

	if(clear)
	{
		LeaveLevel(true);
		return;
	}

	Info("Leaving location.");

	if(Net::IsLocal() && (questMgr->questCrazies->checkStone
		|| (questMgr->questCrazies->craziesState >= Quest_Crazies::State::PickedStone && questMgr->questCrazies->craziesState < Quest_Crazies::State::End)))
		questMgr->questCrazies->CheckStone();

	// drinking contest
	Quest_Contest* contest = questMgr->questContest;
	if(contest->state >= Quest_Contest::CONTEST_STARTING)
		contest->Cleanup();

	// clear blood & bodies from orc base
	if(Net::IsLocal() && questMgr->questOrcs2->orcsState == Quest_Orcs2::State::ClearDungeon && gameLevel->location == questMgr->questOrcs2->targetLoc)
	{
		questMgr->questOrcs2->orcsState = Quest_Orcs2::State::End;
		gameLevel->UpdateLocation(31, 100, false);
	}

	if(gameLevel->cityCtx && gameState != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// ai buy iteams when leaving city
		team->BuyTeamItems();
	}

	LeaveLevel();

	if(gameLevel->isOpen)
	{
		if(Net::IsLocal())
			questMgr->RemoveQuestUnits(true);

		gameLevel->ProcessRemoveUnits(true);

		if(gameLevel->location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = static_cast<OutsideLocation*>(gameLevel->location);
			outside->Clear();
			outside->lastVisit = -1;
		}
	}

	if(team->craziesAttack)
	{
		team->craziesAttack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	// end temporay effects & rest cooldown
	if(Net::IsLocal() && takesTime)
		team->ShortRest();

	gameLevel->isOpen = false;
	gameLevel->cityCtx = nullptr;
	gameLevel->lvl = nullptr;
}

void Game::Event_RandomEncounter(int)
{
	gameGui->worldMap->dialog_enc = nullptr;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CLOSE_ENCOUNTER);
	world->StartEncounter();
	EnterLocation();
}

//=================================================================================================
void Game::OnResize()
{
	gameGui->OnResize();
}

//=================================================================================================
void Game::OnFocus(bool focus, const Int2& activationPoint)
{
	gameGui->OnFocus(focus, activationPoint);
}

//=================================================================================================
void Game::ReportError(int id, cstring text, bool once)
{
	if(once)
	{
		for(int error : reportedErrors)
		{
			if(id == error)
				return;
		}
		reportedErrors.push_back(id);
	}

	cstring mode;
	if(Net::IsSingleplayer())
		mode = "SP";
	else if(Net::IsServer())
		mode = "SV";
	else
		mode = "CL";
	cstring str = Format("[Report %d]: %s", id, text);
	Warn(str);
	if(IsDebug())
		gameGui->messages->AddGameMsg(str, 5.f);
	api->Report(id, Format("[%s] %s", mode, text));
}

//=================================================================================================
void Game::CutsceneStart(bool instant)
{
	cutscene = true;
	gameGui->CloseAllPanels();
	gameGui->levelGui->ResetCutscene();
	fallbackType = FALLBACK::CUTSCENE;
	fallbackTimer = instant ? 0.f : -1.f;

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CUTSCENE_START;
		c.id = (instant ? 1 : 0);
	}
}

//=================================================================================================
void Game::CutsceneImage(const string& image, float time)
{
	assert(cutscene && time > 0);
	Texture* tex;
	if(image.empty())
		tex = nullptr;
	else
	{
		tex = resMgr->TryLoadInstant<Texture>(image);
		if(!tex)
			Warn("CutsceneImage: missing texture '%s'.", image.c_str());
	}
	gameGui->levelGui->SetCutsceneImage(tex, time);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CUTSCENE_IMAGE;
		c.str = StringPool.Get();
		*c.str = image;
		c.f[0] = time;
		net->netStrs.push_back(c.str);
	}
}

//=================================================================================================
void Game::CutsceneText(const string& text, float time)
{
	assert(cutscene && time > 0);
	gameGui->levelGui->SetCutsceneText(text, time);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CUTSCENE_TEXT;
		c.str = StringPool.Get();
		*c.str = text;
		c.f[0] = time;
		net->netStrs.push_back(c.str);
	}
}

//=================================================================================================
void Game::CutsceneEnd()
{
	assert(cutscene);
	cutsceneScript = scriptMgr->SuspendScript();
}

//=================================================================================================
void Game::CutsceneEnded(bool cancel)
{
	if(cancel)
	{
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CUTSCENE_SKIP;
		}

		if(Net::IsClient())
			return;
	}

	cutscene = false;
	fallbackType = FALLBACK::CUTSCENE_END;
	fallbackTimer = -fallbackTimer;

	if(Net::IsLocal())
		scriptMgr->ResumeScript(cutsceneScript);
}

//=================================================================================================
bool Game::CutsceneShouldSkip()
{
	return cfg.GetBool("skipcutscene");
}

//=================================================================================================
void Game::UpdateGame(float dt)
{
	dt *= gameSpeed;
	if(dt == 0)
		return;

	// sanity checks
	if(IsDebug())
	{
		if(Net::IsLocal())
		{
			assert(pc->isLocal);
			if(Net::IsServer())
			{
				for(PlayerInfo& info : net->players)
				{
					if(info.left == PlayerInfo::LEFT_NO)
					{
						assert(info.pc == info.pc->playerInfo->pc);
						if(info.id != 0)
							assert(!info.pc->isLocal);
					}
				}
			}
		}
		else
			assert(pc->isLocal && pc == pc->playerInfo->pc);
	}

	gameLevel->minimapOpenedDoors = false;

	if(questMgr->questTutorial->inTutorial && !Net::IsOnline())
		questMgr->questTutorial->Update();

	portalAnim += dt;
	if(portalAnim >= 1.f)
		portalAnim -= 1.f;
	gameLevel->lightAngle = Clip(gameLevel->lightAngle + dt / 100);
	gameLevel->localPart->lvlPart->scene->lightDir = Vec3(sin(gameLevel->lightAngle), 2.f, cos(gameLevel->lightAngle)).Normalize();

	if(Net::IsLocal() && !questMgr->questTutorial->inTutorial)
	{
		// arena
		if(arena->mode != Arena::NONE)
			arena->Update(dt);

		// sharing of team items between team members
		team->UpdateTeamItemShares();

		// quests
		questMgr->UpdateQuestsLocal(dt);
	}

	// show info about completing all unique quests
	if(CanShowEndScreen())
	{
		if(Net::IsLocal())
			questMgr->uniqueCompletedShow = true;
		else
			questMgr->uniqueCompletedShow = false;

		cstring text;

		if(Net::IsOnline())
		{
			text = txWinMp;
			if(Net::IsServer())
			{
				Net::PushChange(NetChange::GAME_STATS);
				Net::PushChange(NetChange::ALL_QUESTS_COMPLETED);
			}
		}
		else
			text = hardcoreMode ? txWinHardcore : txWin;

		gui->SimpleDialog(Format(text, pc->kills, gameStats->totalKills - pc->kills), nullptr);
	}

	if(devmode && GKey.AllowKeyboard())
	{
		if(!gameLevel->location->outside)
		{
			InsideLocation* inside = static_cast<InsideLocation*>(gameLevel->location);
			InsideLocationLevel& lvl = inside->GetLevelData();

			// key [<,] - warp to stairs up or upper level
			if(input->Down(Key::Shift) && input->Pressed(Key::Comma) && inside->HavePrevEntry())
			{
				if(!input->Down(Key::Control))
				{
					// warp player to up stairs
					if(Net::IsLocal())
					{
						Int2 tile = lvl.GetPrevEntryFrontTile();
						pc->unit->rot = DirToRot(lvl.prevEntryDir);
						gameLevel->WarpUnit(*pc->unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_WARP_TO_ENTRY;
						c.id = 0;
					}
				}
				else
				{
					// go to upper level
					if(Net::IsLocal())
					{
						ChangeLevel(-1);
						return;
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 0;
					}
				}
			}

			// key [>.] - warp to down stairs or lower level
			if(input->Down(Key::Shift) && input->Pressed(Key::Period) && inside->HaveNextEntry())
			{
				if(!input->Down(Key::Control))
				{
					// warp player to down stairs
					if(Net::IsLocal())
					{
						Int2 tile = lvl.GetNextEntryFrontTile();
						pc->unit->rot = DirToRot(lvl.nextEntryDir);
						gameLevel->WarpUnit(*pc->unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_WARP_TO_ENTRY;
						c.id = 1;
					}
				}
				else
				{
					// go to lower level
					if(Net::IsLocal())
					{
						ChangeLevel(+1);
						return;
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 1;
					}
				}
			}
		}
		else if(input->Pressed(Key::Comma) && input->Down(Key::Shift) && input->Down(Key::Control))
		{
			if(Net::IsLocal())
			{
				ExitToMap();
				return;
			}
			else
				Net::PushChange(NetChange::CHEAT_GOTO_MAP);
		}
	}

	UpdateCamera(dt);

	// handle dying
	if((Net::IsLocal() && !team->IsAnyoneAlive()) || deathScreen != 0)
	{
		if(deathScreen == 0)
		{
			Info("Game over: all players died.");
			SetMusic(MusicType::Death);
			gameGui->CloseAllPanels();
			++deathScreen;
			deathFade = 0;
			deathSolo = (team->GetTeamSize() == 1u);
			if(Net::IsOnline())
			{
				Net::PushChange(NetChange::GAME_STATS);
				Net::PushChange(NetChange::GAME_OVER);
			}
		}
		else if(deathScreen == 1 || deathScreen == 2)
		{
			deathFade += dt / 2;
			if(deathFade >= 1.f)
			{
				deathFade = 0;
				++deathScreen;
			}
		}
	}

	pc->Update(dt);

	// update ai
	if(Net::IsLocal())
	{
		if(noai)
		{
			for(AIController* ai : ais)
			{
				if(ai->unit->IsStanding() && Any(ai->unit->animation, ANI_WALK, ANI_WALK_BACK, ANI_RUN, ANI_LEFT, ANI_RIGHT))
					ai->unit->animation = ANI_STAND;
			}
		}
		else
			UpdateAi(dt);
	}

	// update level context
	for(LocationPart& locPart : gameLevel->ForEachPart())
		locPart.Update(dt);

	// update minimap
	gameLevel->UpdateDungeonMinimap(true);

	// update dialogs
	if(Net::IsSingleplayer())
	{
		if(dialogContext.dialogMode)
			dialogContext.Update(dt);
	}
	else if(Net::IsServer())
	{
		for(PlayerInfo& info : net->players)
		{
			if(info.left != PlayerInfo::LEFT_NO)
				continue;
			DialogContext& dialogCtx = *info.u->player->dialogCtx;
			if(dialogCtx.dialogMode)
			{
				if(!dialogCtx.talker->IsStanding() || !dialogCtx.talker->IsIdle() || dialogCtx.talker->toRemove || dialogCtx.talker->frozen != FROZEN::NO)
					dialogCtx.EndDialog();
				else
					dialogCtx.Update(dt);
			}
		}
	}
	else
	{
		if(dialogContext.dialogMode)
			dialogContext.UpdateClient();
	}

	UpdateAttachedSounds(dt);

	gameLevel->ProcessRemoveUnits(false);
}

//=================================================================================================
void Game::UpdateFallback(float dt)
{
	if(fallbackType == FALLBACK::NO)
		return;

	if(fallbackTimer <= 0.f)
	{
		fallbackTimer += dt * 2;

		if(fallbackTimer > 0.f)
		{
			switch(fallbackType)
			{
			case FALLBACK::TRAIN:
				if(Net::IsLocal())
				{
					switch(fallbackValue)
					{
					case 0:
						pc->Train(false, fallbackValue2);
						break;
					case 1:
						pc->Train(true, fallbackValue2);
						break;
					case 2:
						questMgr->questTournament->Train(*pc);
						break;
					case 3:
						pc->AddPerk(Perk::Get(fallbackValue2), -1);
						gameGui->messages->AddGameMsg3(GMS_LEARNED_PERK);
						break;
					case 4:
						pc->AddAbility(Ability::Get(fallbackValue2));
						gameGui->messages->AddGameMsg3(GMS_LEARNED_ABILITY);
						break;
					}
					pc->Rest(10, false);
					if(Net::IsOnline())
						pc->UseDays(10);
					else
						world->Update(10, UM_NORMAL);
				}
				else
				{
					fallbackType = FALLBACK::CLIENT;
					fallbackTimer = 0.f;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRAIN;
					c.id = fallbackValue;
					c.count = fallbackValue2;
				}
				break;
			case FALLBACK::REST:
				if(Net::IsLocal())
				{
					pc->Rest(fallbackValue, true);
					if(Net::IsOnline())
						pc->UseDays(fallbackValue);
					else
						world->Update(fallbackValue, UM_NORMAL);
				}
				else
				{
					fallbackType = FALLBACK::CLIENT;
					fallbackTimer = 0.f;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::REST;
					c.id = fallbackValue;
				}
				break;
			case FALLBACK::ENTER: // enter/exit building
				gameLevel->WarpUnit(pc->unit, fallbackValue, fallbackValue2);
				break;
			case FALLBACK::EXIT:
				ExitToMap();
				break;
			case FALLBACK::CHANGE_LEVEL:
				ChangeLevel(fallbackValue);
				break;
			case FALLBACK::USE_PORTAL:
				{
					LeaveLocation(false, false);
					Portal* portal = gameLevel->location->GetPortal(fallbackValue);
					world->ChangeLevel(portal->targetLoc, false);
					// currently it is only allowed to teleport from X level to first, can teleport back because X level is already visited
					int atLevel = 0;
					if(gameLevel->location->portal)
						atLevel = gameLevel->location->portal->atLevel;
					EnterLocation(atLevel, portal->index);
				}
				return;
			case FALLBACK::NONE:
			case FALLBACK::ARENA2:
			case FALLBACK::CLIENT2:
			case FALLBACK::CUTSCENE_END:
				break;
			case FALLBACK::ARENA:
			case FALLBACK::ARENA_EXIT:
			case FALLBACK::WAIT_FOR_WARP:
			case FALLBACK::CLIENT:
			case FALLBACK::CUTSCENE:
				fallbackTimer = 0.f;
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	else
	{
		fallbackTimer += dt * 2;

		if(fallbackTimer >= 1.f)
		{
			if(Net::IsLocal())
			{
				if(fallbackType != FALLBACK::ARENA2)
				{
					if(fallbackType == FALLBACK::CHANGE_LEVEL || fallbackType == FALLBACK::USE_PORTAL || fallbackType == FALLBACK::EXIT)
					{
						for(Unit& unit : team->members)
							unit.frozen = FROZEN::NO;
					}
					pc->unit->frozen = FROZEN::NO;
				}
			}
			else if(fallbackType == FALLBACK::CLIENT2)
			{
				pc->unit->frozen = FROZEN::NO;
				Net::PushChange(NetChange::END_FALLBACK);
			}
			fallbackType = FALLBACK::NO;
		}
	}
}

//=================================================================================================
void Game::UpdateCamera(float dt)
{
	GameCamera& camera = gameLevel->camera;

	// rotate camera up/down
	if(team->IsAnyoneAlive())
	{
		if(dialogContext.dialogMode || gameGui->inventory->mode > I_INVENTORY || gameGui->craft->visible)
		{
			// in dialog/trade look at target
			camera.RotateTo(dt, 4.2875104f);
			Vec3 zoom_pos;
			if(gameGui->inventory->mode == I_LOOT_CHEST)
				zoom_pos = pc->actionChest->GetCenter();
			else if(gameGui->inventory->mode == I_LOOT_CONTAINER)
				zoom_pos = pc->actionUsable->GetCenter();
			else if(gameGui->craft->visible)
				zoom_pos = pc->unit->usable->GetCenter();
			else
			{
				if(pc->actionUnit->IsAlive())
					zoom_pos = pc->actionUnit->GetHeadSoundPos();
				else
					zoom_pos = pc->actionUnit->GetLootCenter();
			}
			camera.SetZoom(&zoom_pos);
		}
		else
		{
			camera.UpdateFreeRot(dt);
			camera.UpdateDistance();
			camera.SetZoom(nullptr);
		}
	}
	else
	{
		// when all died rotate camera to look from UP
		camera.RotateTo(dt, PI + 0.1f);
		camera.SetZoom(nullptr);
		if(camera.dist < 10.f)
			camera.dist += dt;
	}

	camera.Update(dt);
}

//=================================================================================================
uint Game::TestGameData(bool major)
{
	string str;
	uint errors = 0;

	Info("Test: Checking items...");

	// weapons
	for(Weapon* weapon : Weapon::weapons)
	{
		const Weapon& w = *weapon;
		if(!w.mesh)
		{
			Error("Test: Weapon %s: missing mesh.", w.id.c_str());
			++errors;
		}
		else
		{
			resMgr->LoadMeshMetadata(w.mesh);
			Mesh::Point* pt = w.mesh->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				Error("Test: Weapon %s: no hitbox in mesh %s.", w.id.c_str(), w.mesh->filename);
				++errors;
			}
			else if(!pt->size.IsPositive())
			{
				Error("Test: Weapon %s: invalid hitbox %g, %g, %g in mesh %s.", w.id.c_str(), pt->size.x, pt->size.y, pt->size.z, w.mesh->filename);
				++errors;
			}
		}
	}

	// shields
	for(Shield* shield : Shield::shields)
	{
		const Shield& s = *shield;
		if(!s.mesh)
		{
			Error("Test: Shield %s: missing mesh.", s.id.c_str());
			++errors;
		}
		else
		{
			resMgr->LoadMeshMetadata(s.mesh);
			Mesh::Point* pt = s.mesh->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				Error("Test: Shield %s: no hitbox in mesh %s.", s.id.c_str(), s.mesh->filename);
				++errors;
			}
			else if(!pt->size.IsPositive())
			{
				Error("Test: Shield %s: invalid hitbox %g, %g, %g in mesh %s.", s.id.c_str(), pt->size.x, pt->size.y, pt->size.z, s.mesh->filename);
				++errors;
			}
		}
	}

	// units
	Info("Test: Checking units...");
	for(UnitData* ud_ptr : UnitData::units)
	{
		UnitData& ud = *ud_ptr;
		str.clear();

		// unit inventory
		if(ud.itemScript)
			ud.itemScript->Test(str, errors);

		// attacks
		if(ud.frames)
		{
			if(ud.frames->extra)
			{
				if(InRange(ud.frames->attacks, 1, NAMES::maxAttacks))
				{
					for(int i = 0; i < ud.frames->attacks; ++i)
					{
						if(!InRange(0.f, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end, 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i + 1, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end);
							++errors;
						}
					}
				}
				else
				{
					str += Format("\tInvalid attacks count (%d).\n", ud.frames->attacks);
					++errors;
				}
			}
			else
			{
				if(InRange(ud.frames->attacks, 1, 3))
				{
					for(int i = 0; i < ud.frames->attacks; ++i)
					{
						if(!InRange(0.f, ud.frames->t[F_ATTACK1_START + i * 2], ud.frames->t[F_ATTACK1_END + i * 2], 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i + 1, ud.frames->t[F_ATTACK1_START + i * 2],
								ud.frames->t[F_ATTACK1_END + i * 2]);
							++errors;
						}
					}
				}
				else
				{
					str += Format("\tInvalid attacks count (%d).\n", ud.frames->attacks);
					++errors;
				}
			}
			if(!InRange(ud.frames->t[F_BASH], 0.f, 1.f))
			{
				str += Format("\tInvalid attack point '%g' bash.\n", ud.frames->t[F_BASH]);
				++errors;
			}
		}
		else
		{
			str += "\tMissing attacks information.\n";
			++errors;
		}

		// unit model
		if(major)
		{
			Mesh& mesh = *ud.mesh;
			resMgr->Load(&mesh);

			for(uint i = 0; i < NAMES::nAniBase; ++i)
			{
				if(!mesh.GetAnimation(NAMES::aniBase[i]))
				{
					str += Format("\tMissing animation '%s'.\n", NAMES::aniBase[i]);
					++errors;
				}
			}

			if(!IsSet(ud.flags, F_SLOW))
			{
				if(!mesh.GetAnimation(NAMES::aniRun))
				{
					str += Format("\tMissing animation '%s'.\n", NAMES::aniRun);
					++errors;
				}
			}

			if(!IsSet(ud.flags, F_DONT_SUFFER))
			{
				if(!mesh.GetAnimation(NAMES::aniHurt))
				{
					str += Format("\tMissing animation '%s'.\n", NAMES::aniHurt);
					++errors;
				}
			}

			if(IsSet(ud.flags, F_HUMAN) || IsSet(ud.flags, F_HUMANOID))
			{
				for(uint i = 0; i < NAMES::nPoints; ++i)
				{
					if(!mesh.GetPoint(NAMES::points[i]))
					{
						str += Format("\tMissing attachment point '%s'.\n", NAMES::points[i]);
						++errors;
					}
				}

				for(uint i = 0; i < NAMES::nAniHumanoid; ++i)
				{
					if(!mesh.GetAnimation(NAMES::aniHumanoid[i]))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::aniHumanoid[i]);
						++errors;
					}
				}
			}

			// attack animations
			if(ud.frames)
			{
				if(ud.frames->attacks > NAMES::maxAttacks)
				{
					str += Format("\tToo many attacks (%d)!\n", ud.frames->attacks);
					++errors;
				}
				for(int i = 0; i < min(ud.frames->attacks, NAMES::maxAttacks); ++i)
				{
					if(!mesh.GetAnimation(NAMES::aniAttacks[i]))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::aniAttacks[i]);
						++errors;
					}
				}
			}

			// idle animations
			if(ud.idles)
			{
				for(const string& s : ud.idles->anims)
				{
					if(!mesh.GetAnimation(s.c_str()))
					{
						str += Format("\tMissing animation '%s'.\n", s.c_str());
						++errors;
					}
				}
			}

			// cast spell point
			if(ud.abilities && !mesh.GetPoint(NAMES::pointCast))
			{
				str += Format("\tMissing attachment point '%s'.\n", NAMES::pointCast);
				++errors;
			}
		}

		if(!str.empty())
			Error("Test: Unit %s:\n%s", ud.id.c_str(), str.c_str());
	}

	return errors;
}

void Game::ChangeLevel(int where)
{
	assert(where == 1 || where == -1);

	Info(where == 1 ? "Changing level to lower." : "Changing level to upper.");

	gameLevel->eventHandler = nullptr;
	gameLevel->UpdateDungeonMinimap(false);

	if(!questMgr->questTutorial->inTutorial && questMgr->questCrazies->craziesState >= Quest_Crazies::State::PickedStone
		&& questMgr->questCrazies->craziesState < Quest_Crazies::State::End)
		questMgr->questCrazies->CheckStone();

	if(Net::IsOnline() && net->activePlayers > 1)
	{
		int level = gameLevel->dungeonLevel;
		if(where == -1)
			--level;
		else
			++level;
		if(level >= 0)
			net->OnChangeLevel(level);

		net->FilterServerChanges();
	}

	if(where == -1)
	{
		// upper leve
		if(gameLevel->dungeonLevel == 0)
		{
			if(questMgr->questTutorial->inTutorial)
			{
				questMgr->questTutorial->OnEvent(Quest_Tutorial::Exit);
				fallbackType = FALLBACK::CLIENT;
				fallbackTimer = 0.f;
				return;
			}

			// exit to surface
			ExitToMap();
			return;
		}
		else
		{
			LoadingStart(4);
			LoadingStep(txLevelUp);

			MultiInsideLocation* inside = static_cast<MultiInsideLocation*>(gameLevel->location);
			LeaveLevel();
			--gameLevel->dungeonLevel;
			LocationGenerator* locGen = locGenFactory->Get(inside);
			gameLevel->enterFrom = ENTER_FROM_NEXT_LEVEL;
			EnterLevel(locGen);
		}
	}
	else
	{
		MultiInsideLocation* inside = (MultiInsideLocation*)gameLevel->location;

		int steps = 3; // txLevelDown, txGeneratingMinimap, txLoadingComplete
		if(gameLevel->dungeonLevel + 1 >= inside->generated)
			steps += 3; // txGeneratingMap, txGeneratingObjects, txGeneratingUnits
		else
			++steps; // txRegeneratingLevel

		LoadingStart(steps);
		LoadingStep(txLevelDown);

		// lower level
		LeaveLevel();
		++gameLevel->dungeonLevel;

		LocationGenerator* locGen = locGenFactory->Get(inside);

		// is this first visit?
		if(gameLevel->dungeonLevel >= inside->generated)
		{
			if(nextSeed != 0)
			{
				Srand(nextSeed);
				nextSeed = 0;
			}

			inside->generated = gameLevel->dungeonLevel + 1;
			inside->infos[gameLevel->dungeonLevel].seed = RandVal();

			Info("Generating location '%s', seed %u.", gameLevel->location->name.c_str(), RandVal());
			Info("Generating dungeon, level %d, target %d.", gameLevel->dungeonLevel + 1, inside->target);

			LoadingStep(txGeneratingMap);

			locGen->first = true;
			locGen->Generate();
		}

		gameLevel->enterFrom = ENTER_FROM_PREV_LEVEL;
		EnterLevel(locGen);
	}

	gameLevel->location->lastVisit = world->GetWorldtime();
	gameLevel->CheckIfLocationCleared();
	bool loaded_resources = gameLevel->location->RequireLoadingResources(nullptr);
	LoadResources(txLoadingComplete, false);

	SetMusic();

	if(Net::IsOnline() && net->activePlayers > 1)
	{
		netMode = NM_SERVER_SEND;
		netState = NetState::Server_Send;
		preparedStream.Reset();
		BitStreamWriter f(preparedStream);
		net->WriteLevelData(f, loaded_resources);
		Info("Generated location packet: %d.", preparedStream.GetNumberOfBytesUsed());
		gameGui->infoBox->Show(txWaitingForPlayers);
	}
	else
	{
		gameState = GS_LEVEL;
		gameGui->loadScreen->visible = false;
		gameGui->mainMenu->visible = false;
		gameGui->levelGui->visible = true;
		gameLevel->ready = true;
	}

	Info("Randomness integrity: %d", RandVal());
}

void Game::ExitToMap()
{
	gameGui->CloseAllPanels();

	gameState = GS_WORLDMAP;
	gameLevel->ready = false;
	if(gameLevel->isOpen)
		LeaveLocation();

	net->ClearFastTravel();
	world->ExitToMap();
	SetMusic(MusicType::Travel);

	if(Net::IsServer())
		Net::PushChange(NetChange::EXIT_TO_MAP);

	gameGui->worldMap->Show();
	gameGui->levelGui->visible = false;
}

void Game::SetMusic(MusicType type)
{
	if(type == MusicType::Default)
		type = gameLevel->boss ? MusicType::Boss : gameLevel->GetLocationMusic();

	if(soundMgr->IsDisabled() || type == musicType)
		return;

	const bool delayed = musicType == MusicType::Intro && type == MusicType::Title;
	musicType = type;
	soundMgr->PlayMusic(musicLists[(int)type], delayed);
}

void Game::PlayAttachedSound(Unit& unit, Sound* sound, float distance)
{
	assert(sound);

	if(!soundMgr->CanPlaySound())
		return;

	Vec3 pos = unit.GetHeadSoundPos();
	FMOD::Channel* channel = soundMgr->CreateChannel(sound, pos, distance);
	AttachedSound& s = Add1(attachedSounds);
	s.channel = channel;
	s.unit = &unit;
}

void Game::StopAllSounds()
{
	soundMgr->StopSounds();
	attachedSounds.clear();
}

void Game::UpdateAttachedSounds(float dt)
{
	LoopAndRemove(attachedSounds, [](AttachedSound& sound)
	{
		Unit* unit = sound.unit;
		if(unit)
		{
			if(!soundMgr->UpdateChannelPosition(sound.channel, unit->GetHeadSoundPos()))
				return false;
		}
		else
		{
			if(!soundMgr->IsPlaying(sound.channel))
				return true;
		}
		return false;
	});
}

// clear game vars on new game or load
void Game::ClearGameVars(bool newGame)
{
	gameGui->inventory->mode = I_NONE;
	dialogContext.dialogMode = false;
	dialogContext.isLocal = true;
	deathScreen = 0;
	endOfGame = false;
	cutscene = false;
	gameGui->minimap->city = nullptr;
	team->Clear(newGame);
	drawFlags = 0xFFFFFFFF;
	gameGui->levelGui->Reset();
	gameGui->journal->Reset();
	arena->Reset();
	gameGui->levelGui->visible = false;
	gameGui->inventory->lock = nullptr;
	gameLevel->camera.Reset(newGame);
	pc->data.Reset();
	scriptMgr->Reset();
	gameLevel->Reset();
	aiMgr->Reset();
	pathfinding->SetTarget(nullptr);
	gameGui->worldMap->Clear();
	net->ClearFastTravel();
	ParticleEmitter::ResetEntities();
	TrailParticleEmitter::ResetEntities();
	Unit::ResetEntities();
	GroundItem::ResetEntities();
	Chest::ResetEntities();
	Usable::ResetEntities();
	Trap::ResetEntities();
	Door::ResetEntities();
	Electro::ResetEntities();
	Bullet::ResetEntities();

	// remove black background at begining
	fallbackType = FALLBACK::NONE;
	fallbackTimer = -0.5f;

	if(newGame)
	{
		devmode = defaultDevmode;
		sceneMgr->useLighting = true;
		sceneMgr->useFog = true;
		drawParticleSphere = false;
		drawUnitRadius = false;
		drawHitbox = false;
		noai = false;
		drawPhy = false;
		drawCol = false;
		gameSpeed = 1.f;
		gameLevel->dungeonLevel = -1;
		questMgr->Reset();
		world->OnNewGame();
		gameStats->Reset();
		dontWander = false;
		gameLevel->isOpen = false;
		gameGui->levelGui->PositionPanels();
		gameGui->Clear(true, false);
		if(!net->mpQuickload)
			gameGui->mpBox->visible = Net::IsOnline();
		gameLevel->lightAngle = Random(PI * 2);
		startVersion = VERSION;

		if(IsDebug())
		{
			noai = true;
			dontWander = true;
		}
	}
}

// called before starting new game/loading/at exit
void Game::ClearGame()
{
	Info("Clearing game.");

	EntitySystem::clear = true;
	drawBatch.Clear();
	scriptMgr->StopAllScripts();

	LeaveLocation(true, false);

	// delete units on world map
	if((gameState == GS_WORLDMAP || prevGameState == GS_WORLDMAP || (net->mpLoad && prevGameState == GS_LOAD)) && !gameLevel->isOpen
		&& Net::IsLocal() && !net->wasClient)
	{
		for(Unit& unit : team->members)
		{
			if(unit.interp)
				EntityInterpolator::Pool.Free(unit.interp);

			delete unit.ai;
			delete &unit;
		}
		team->members.clear();
		prevGameState = GS_LOAD;
	}
	if((gameState == GS_WORLDMAP || prevGameState == GS_WORLDMAP || prevGameState == GS_LOAD) && !gameLevel->isOpen && (Net::IsClient() || net->wasClient))
	{
		delete pc;
		pc = nullptr;
	}

	if(!net->netStrs.empty())
		StringPool.Free(net->netStrs);

	gameLevel->isOpen = false;
	gameLevel->cityCtx = nullptr;
	questMgr->Clear();
	world->Reset();
	gameGui->Clear(true, false);
	pc = nullptr;
	cutscene = false;
	EntitySystem::clear = false;
}

void ApplyTextureOverrideToSubmesh(Mesh::Submesh& sub, TexOverride& tex_o)
{
	sub.tex = tex_o.diffuse;
	sub.tex_normal = tex_o.normal;
	sub.tex_specular = tex_o.specular;
}

void ApplyDungeonLightToMesh(Mesh& mesh)
{
	for(int i = 0; i < mesh.head.n_subs; ++i)
	{
		mesh.subs[i].specular_color = Vec3(1, 1, 1);
		mesh.subs[i].specular_intensity = 0.2f;
		mesh.subs[i].specular_hardness = 10;
	}
}

void Game::SetDungeonParamsToMeshes()
{
	// doors/stairs/traps textures
	ApplyTextureOverrideToSubmesh(gameRes->aStairsDown->subs[0], gameRes->tFloor[0]);
	ApplyTextureOverrideToSubmesh(gameRes->aStairsDown->subs[2], gameRes->tWall[0]);
	ApplyTextureOverrideToSubmesh(gameRes->aStairsDown2->subs[0], gameRes->tFloor[0]);
	ApplyTextureOverrideToSubmesh(gameRes->aStairsDown2->subs[2], gameRes->tWall[0]);
	ApplyTextureOverrideToSubmesh(gameRes->aStairsUp->subs[0], gameRes->tFloor[0]);
	ApplyTextureOverrideToSubmesh(gameRes->aStairsUp->subs[2], gameRes->tWall[0]);
	ApplyTextureOverrideToSubmesh(gameRes->aDoorWall->subs[0], gameRes->tWall[0]);
	ApplyDungeonLightToMesh(*gameRes->aStairsDown);
	ApplyDungeonLightToMesh(*gameRes->aStairsDown2);
	ApplyDungeonLightToMesh(*gameRes->aStairsUp);
	ApplyDungeonLightToMesh(*gameRes->aDoorWall);
	ApplyDungeonLightToMesh(*gameRes->aDoorWall2);

	// apply texture/lighting to trap to make it same texture as dungeon
	if(BaseTrap::traps[TRAP_ARROW].mesh->state == ResourceState::Loaded)
	{
		ApplyTextureOverrideToSubmesh(BaseTrap::traps[TRAP_ARROW].mesh->subs[0], gameRes->tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_ARROW].mesh);
	}
	if(BaseTrap::traps[TRAP_POISON].mesh->state == ResourceState::Loaded)
	{
		ApplyTextureOverrideToSubmesh(BaseTrap::traps[TRAP_POISON].mesh->subs[0], gameRes->tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_POISON].mesh);
	}

	// second texture
	ApplyTextureOverrideToSubmesh(gameRes->aDoorWall2->subs[0], gameRes->tWall[1]);
}

void Game::EnterLevel(LocationGenerator* locGen)
{
	if(!locGen->first)
		Info("Entering location '%s' level %d.", gameLevel->location->name.c_str(), gameLevel->dungeonLevel + 1);

	gameGui->inventory->lock = nullptr;

	locGen->OnEnter();

	OnEnterLevelOrLocation();
	OnEnterLevel();

	if(!locGen->first)
		Info("Entered level.");
}

void Game::LeaveLevel(bool clear)
{
	Info("Leaving level.");

	if(gameGui->levelGui)
		gameGui->levelGui->Reset();

	if(gameLevel->isOpen)
	{
		gameLevel->boss = nullptr;
		for(LocationPart& locPart : gameLevel->ForEachPart())
		{
			LeaveLevel(locPart, clear);
			delete locPart.lvlPart;
			locPart.lvlPart = nullptr;
		}
		if(gameLevel->cityCtx && (Net::IsClient() || net->wasClient))
			DeleteElements(gameLevel->cityCtx->insideBuildings);
		if(Net::IsClient() && !gameLevel->location->outside)
		{
			InsideLocation* inside = (InsideLocation*)gameLevel->location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			Room::Free(lvl.rooms);
		}
		if(clear)
			pc = nullptr;
	}

	ais.clear();
	gameLevel->RemoveColliders();
	StopAllSounds();

	ClearQuadtree();

	gameGui->CloseAllPanels();

	gameLevel->camera.Reset();
	PlayerController::data.rotBuf = 0.f;
	dialogContext.dialogMode = false;
	gameGui->inventory->mode = I_NONE;
	PlayerController::data.beforePlayer = BP_NONE;
	if(Net::IsClient())
		gameLevel->isOpen = false;
}

void Game::LeaveLevel(LocationPart& locPart, bool clear)
{
	// cleanup units
	if(Net::IsLocal() && !clear && !net->wasClient)
	{
		LoopAndRemove(locPart.units, [&](Unit* p_unit)
		{
			Unit& unit = *p_unit;

			unit.BreakAction(Unit::BREAK_ACTION_MODE::ON_LEAVE);

			// physics
			if(unit.cobj)
			{
				delete unit.cobj->getCollisionShape();
				unit.cobj = nullptr;
			}

			// speech bubble
			unit.bubble = nullptr;
			unit.talking = false;

			// mesh
			if(unit.IsAI())
			{
				if(unit.IsFollower())
				{
					if(!unit.IsStanding())
						unit.Standup(false, true);
					if(unit.GetOrder() != ORDER_FOLLOW)
						unit.OrderFollow(team->GetLeader());
					unit.ai->Reset();
					return true;
				}
				else
				{
					UnitOrder order = unit.GetOrder();
					if(order == ORDER_LEAVE && unit.IsAlive())
					{
						delete unit.ai;
						delete &unit;
						return true;
					}
					else
					{
						if(unit.liveState == Unit::DYING)
						{
							unit.liveState = Unit::DEAD;
							unit.meshInst->SetToEnd();
							gameLevel->CreateBlood(locPart, unit, true);
						}
						else if(Any(unit.liveState, Unit::FALLING, Unit::FALL))
							unit.Standup(false, true);

						if(unit.IsAlive())
						{
							// warp to inn if unit wanted to go there
							if(order == ORDER_GOTO_INN)
							{
								unit.OrderNext();
								if(gameLevel->cityCtx)
								{
									InsideBuilding* inn = gameLevel->cityCtx->FindInn();
									gameLevel->WarpToRegion(*inn, (Rand() % 5 == 0 ? inn->region2 : inn->region1), unit.GetUnitRadius(), unit.pos, 20);
									unit.visualPos = unit.pos;
									unit.locPart = inn;
									inn->units.push_back(&unit);
									return true;
								}
							}

							// reset units rotation to don't stay back to shop counter
							if(IsSet(unit.data->flags, F_AI_GUARD) || IsSet(unit.data->flags2, F2_LIMITED_ROT))
								unit.rot = unit.ai->startRot;
						}

						delete unit.meshInst;
						unit.meshInst = nullptr;
						delete unit.ai;
						unit.ai = nullptr;
						unit.EndEffects();
						return false;
					}
				}
			}
			else
			{
				unit.talking = false;
				unit.meshInst->needUpdate = true;
				unit.usable = nullptr;
				return true;
			}
		});

		for(Object* obj : locPart.objects)
		{
			if(obj->meshInst)
			{
				delete obj->meshInst;
				obj->meshInst = nullptr;
			}
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
		{
			if(!*it)
				continue;

			Unit& unit = **it;

			if(unit.IsLocalPlayer() && !clear)
				unit.player = nullptr; // don't delete game->pc

			if(unit.cobj)
				delete unit.cobj->getCollisionShape();

			unit.bubble = nullptr;
			unit.talking = false;

			if(unit.interp)
				EntityInterpolator::Pool.Free(unit.interp);

			delete unit.ai;
			delete *it;
		}

		locPart.units.clear();
	}

	if(Net::IsLocal() && !net->wasClient)
	{
		// remove chest meshes
		for(Chest* chest : locPart.chests)
			chest->Cleanup();

		// remove door meshes
		for(Door* door : locPart.doors)
			door->Cleanup();

		// remove player traps & remove mesh instance
		LoopAndRemove(locPart.traps, [](Trap* trap)
		{
			if(trap->owner != nullptr)
			{
				delete trap;
				return true;
			}

			if(trap->meshInst)
			{
				delete trap->meshInst;
				trap->meshInst = nullptr;
			}

			return false;
		});
	}
	else
	{
		// delete entities
		DeleteElements(locPart.objects);
		DeleteElements(locPart.chests);
		DeleteElements(locPart.doors);
		DeleteElements(locPart.traps);
		DeleteElements(locPart.usables);
		DeleteElements(locPart.GetGroundItems());
	}

	if(!clear)
	{
		// make blood splatter full size
		for(vector<Blood>::iterator it = locPart.bloods.begin(), end = locPart.bloods.end(); it != end; ++it)
			it->size = 1.f;
	}
}

void Game::LoadingStart(int steps)
{
	gameGui->loadScreen->Reset();
	loadingTimer.Reset();
	loadingDt = 0.f;
	loadingCap = 0.66f;
	loadingSteps = steps;
	loadingIndex = 0;
	loadingFirstStep = true;
	gameState = GS_LOAD;
	gameLevel->ready = false;
	gameGui->loadScreen->visible = true;
	gameGui->mainMenu->visible = false;
	gameGui->levelGui->visible = false;
	resMgr->PrepareLoadScreen(loadingCap);
}

void Game::LoadingStep(cstring text, int end)
{
	float progress;
	if(end == 0)
		progress = float(loadingIndex) / loadingSteps * loadingCap;
	else if(end == 1)
		progress = loadingCap;
	else
		progress = 1.f;
	gameGui->loadScreen->SetProgressOptional(progress, text);

	if(end != 2)
	{
		++loadingIndex;
		if(end == 1)
			assert(loadingIndex == loadingSteps);
	}
	if(end != 1)
	{
		loadingDt += loadingTimer.Tick();
		if(loadingDt >= 1.f / 30 || end == 2 || loadingFirstStep)
		{
			loadingDt = 0.f;
			engine->DoPseudotick();
			loadingTimer.Tick();
			loadingFirstStep = false;
		}
	}
}

// Preload resources and start load screen if required
void Game::LoadResources(cstring text, bool worldmap, bool postLoad)
{
	LoadingStep(nullptr, 1);

	PreloadResources(worldmap);

	// check if there is anything to load
	if(resMgr->HaveTasks())
	{
		Info("Loading new resources (%d).", resMgr->GetLoadTasksCount());
		loadingResources = true;
		loadingDt = 0;
		loadingTimer.Reset();
		try
		{
			resMgr->StartLoadScreen(txLoadingResources);
		}
		catch(...)
		{
			loadingResources = false;
			throw;
		}

		// apply mesh instance for newly loaded meshes
		for(auto& unit_mesh : unitsMeshLoad)
		{
			Unit::CREATE_MESH mode;
			if(unit_mesh.second)
				mode = Unit::CREATE_MESH::ON_WORLDMAP;
			else if(net->mpLoad && Net::IsClient())
				mode = Unit::CREATE_MESH::AFTER_PRELOAD;
			else
				mode = Unit::CREATE_MESH::NORMAL;
			unit_mesh.first->CreateMesh(mode);
		}
		unitsMeshLoad.clear();
	}
	else
	{
		Info("Nothing new to load.");
		resMgr->CancelLoadScreen();
	}

	if(postLoad && gameLevel->location && gameLevel->isOpen)
	{
		// spawn blood for units that are dead and their mesh just loaded
		gameLevel->SpawnBlood();

		// create mesh instance for objects
		gameLevel->CreateObjectsMeshInstance();

		// finished
		if((Net::IsLocal() || !net->mpLoadWorldmap) && !gameLevel->location->outside)
			SetDungeonParamsToMeshes();
	}

	LoadingStep(text, 2);
}

// When there is something new to load, add task to load it when entering location etc
void Game::PreloadResources(bool worldmap)
{
	if(Net::IsLocal())
		itemsLoad.clear();

	if(!worldmap)
	{
		for(LocationPart& locPart : gameLevel->ForEachPart())
		{
			// load units - units respawn so need to check everytime...
			for(Unit* unit : locPart.units)
				PreloadUnit(unit);

			// some traps respawn
			for(Trap* trap : locPart.traps)
				gameRes->LoadTrap(trap->base);

			// preload items, this info is sent by server so no need to redo this by clients (and it will be less complete)
			if(Net::IsLocal())
			{
				for(GroundItem* groundItem : locPart.GetGroundItems())
					itemsLoad.insert(groundItem->item);
				for(Chest* chest : locPart.chests)
					PreloadItems(chest->items);
				for(Usable* usable : locPart.usables)
				{
					if(usable->container)
						PreloadItems(usable->container->items);
				}
			}
		}

		bool new_value = true;
		if(gameLevel->location->RequireLoadingResources(&new_value) == false)
		{
			// load music
			gameRes->LoadMusic(gameLevel->GetLocationMusic(), false);

			for(LocationPart& locPart : gameLevel->ForEachPart())
			{
				// load objects
				for(Object* obj : locPart.objects)
					resMgr->Load(obj->mesh);
				for(Chest* chest : locPart.chests)
					resMgr->Load(chest->base->mesh);

				// load usables
				for(Usable* use : locPart.usables)
				{
					BaseUsable* base = use->base;
					if(base->state == ResourceState::NotLoaded)
					{
						if(base->variants)
						{
							for(Mesh* mesh : base->variants->meshes)
								resMgr->Load(mesh);
						}
						else
							resMgr->Load(base->mesh);
						if(base->sound)
							resMgr->Load(base->sound);
						base->state = ResourceState::Loaded;
					}
				}
			}

			// load buildings
			if(gameLevel->cityCtx)
			{
				for(CityBuilding& city_building : gameLevel->cityCtx->buildings)
				{
					Building& building = *city_building.building;
					if(building.state == ResourceState::NotLoaded)
					{
						if(building.mesh)
							resMgr->Load(building.mesh);
						if(building.insideMesh)
							resMgr->Load(building.insideMesh);
						building.state = ResourceState::Loaded;
					}
				}
			}
		}
	}

	for(const Item* item : itemsLoad)
		gameRes->PreloadItem(item);
}

void Game::PreloadUnit(Unit* unit)
{
	UnitData& data = *unit->data;

	if(Net::IsLocal())
	{
		array<const Item*, SLOT_MAX>& equipped = unit->GetEquippedItems();
		for(const Item* item : equipped)
		{
			if(item)
				itemsLoad.insert(item);
		}
		PreloadItems(unit->items);
		if(unit->stock)
			PreloadItems(unit->stock->items);
	}

	if(data.state == ResourceState::Loaded)
		return;

	if(data.mesh)
		resMgr->Load(data.mesh);

	if(!soundMgr->IsDisabled())
	{
		for(int i = 0; i < SOUND_MAX; ++i)
		{
			for(SoundPtr sound : data.sounds->sounds[i])
				resMgr->Load(sound);
		}
	}

	if(data.tex)
	{
		for(TexOverride& tex_o : data.tex->textures)
		{
			if(tex_o.diffuse)
				resMgr->Load(tex_o.diffuse);
		}
	}

	data.state = ResourceState::Loaded;
}

void Game::PreloadItems(vector<ItemSlot>& items)
{
	for(auto& slot : items)
	{
		assert(slot.item);
		itemsLoad.insert(slot.item);
	}
}

void Game::VerifyResources()
{
	for(LocationPart& locPart : gameLevel->ForEachPart())
	{
		for(GroundItem* groundItem : locPart.GetGroundItems())
			VerifyItemResources(groundItem->item);
		for([[maybe_unused]] Object* obj : locPart.objects)
			assert(obj->mesh->state == ResourceState::Loaded);
		for(Unit* unit : locPart.units)
			VerifyUnitResources(unit);
		for(Usable* u : locPart.usables)
		{
			BaseUsable* base = u->base;
			assert(base->state == ResourceState::Loaded);
			if(base->sound)
				assert(base->sound->IsLoaded());
		}
		for(Trap* trap : locPart.traps)
		{
			assert(trap->base->state == ResourceState::Loaded);
			if(trap->base->mesh)
				assert(trap->base->mesh->IsLoaded());
			if(trap->base->sound)
				assert(trap->base->sound->IsLoaded());
			if(trap->base->sound2)
				assert(trap->base->sound2->IsLoaded());
			if(trap->base->sound3)
				assert(trap->base->sound3->IsLoaded());
		}
	}
}

void Game::VerifyUnitResources(Unit* unit)
{
	assert(unit->data->state == ResourceState::Loaded);
	if(unit->data->mesh)
		assert(unit->data->mesh->IsLoaded());
	if(unit->data->sounds)
	{
		for(int i = 0; i < SOUND_MAX; ++i)
		{
			for([[maybe_unused]] SoundPtr sound : unit->data->sounds->sounds[i])
				assert(sound->IsLoaded());
		}
	}
	if(unit->data->tex)
	{
		for(TexOverride& tex_o : unit->data->tex->textures)
		{
			if(tex_o.diffuse)
				assert(tex_o.diffuse->IsLoaded());
		}
	}

	array<const Item*, SLOT_MAX>& equipped = unit->GetEquippedItems();
	for(const Item* item : equipped)
	{
		if(item)
			VerifyItemResources(item);
	}
	for(ItemSlot& slot : unit->items)
		VerifyItemResources(slot.item);
}

void Game::VerifyItemResources(const Item* item)
{
	assert(item->state == ResourceState::Loaded);
	if(item->tex)
		assert(item->tex->IsLoaded());
	if(item->mesh)
		assert(item->mesh->IsLoaded());
	assert(item->icon);
}

void Game::DeleteUnit(Unit* unit)
{
	assert(unit);

	if(gameLevel->isOpen)
	{
		RemoveElement(unit->locPart->units, unit);
		gameGui->levelGui->RemoveUnitView(unit);
		if(pc->data.beforePlayer == BP_UNIT && pc->data.beforePlayerPtr.unit == unit)
			pc->data.beforePlayer = BP_NONE;
		if(unit == pc->data.selectedUnit)
			pc->data.selectedUnit = nullptr;
		if(Net::IsClient())
		{
			if(pc->action == PlayerAction::LootUnit && pc->actionUnit == unit)
				pc->unit->BreakAction();
		}
		else
		{
			for(PlayerInfo& player : net->players)
			{
				PlayerController* pc = player.pc;
				if(pc->action == PlayerAction::LootUnit && pc->actionUnit == unit)
					pc->actionUnit = nullptr;
			}
		}

		if(unit->player && Net::IsLocal())
		{
			switch(unit->player->action)
			{
			case PlayerAction::LootChest:
				unit->player->actionChest->OpenClose(nullptr);
				break;
			case PlayerAction::LootUnit:
				unit->player->actionUnit->busy = Unit::Busy_No;
				break;
			case PlayerAction::Trade:
			case PlayerAction::Talk:
			case PlayerAction::GiveItems:
			case PlayerAction::ShareItems:
				unit->player->actionUnit->busy = Unit::Busy_No;
				unit->player->actionUnit->lookTarget = nullptr;
				break;
			case PlayerAction::LootContainer:
				unit->UseUsable(nullptr);
				break;
			}
		}

		if(questMgr->questContest->state >= Quest_Contest::CONTEST_STARTING)
			RemoveElementTry(questMgr->questContest->units, unit);
		if(!arena->free)
		{
			RemoveElementTry(arena->units, unit);
			if(arena->fighter == unit)
				arena->fighter = nullptr;
		}

		if(unit->usable)
			unit->usable->user = nullptr;

		if(unit->bubble)
			unit->bubble->unit = nullptr;
	}

	if(unit->interp)
		EntityInterpolator::Pool.Free(unit->interp);

	if(unit->ai)
	{
		RemoveElement(ais, unit->ai);
		delete unit->ai;
	}

	if(Net::IsLocal() && unit->IsHero() && unit->hero->otherTeam)
		unit->hero->otherTeam->Remove(unit);

	if(unit->cobj)
	{
		delete unit->cobj->getCollisionShape();
		phyWorld->removeCollisionObject(unit->cobj);
		delete unit->cobj;
	}

	if(--unit->refs == 0)
		delete unit;
}

void Game::RemoveUnit(Unit* unit)
{
	assert(unit && !unit->player);

	unit->BreakAction(Unit::BREAK_ACTION_MODE::ON_LEAVE);
	RemoveElement(unit->locPart->units, unit);
	gameGui->levelGui->RemoveUnitView(unit);
	if(pc->data.beforePlayer == BP_UNIT && pc->data.beforePlayerPtr.unit == unit)
		pc->data.beforePlayer = BP_NONE;
	if(unit == pc->data.selectedUnit)
		pc->data.selectedUnit = nullptr;
	if(Net::IsClient())
	{
		if(pc->action == PlayerAction::LootUnit && pc->actionUnit == unit)
			pc->unit->BreakAction();
	}
	else
	{
		for(PlayerInfo& player : net->players)
		{
			PlayerController* pc = player.pc;
			if(pc->action == PlayerAction::LootUnit && pc->actionUnit == unit)
				pc->actionUnit = nullptr;
		}
	}

	if(questMgr->questContest->state >= Quest_Contest::CONTEST_STARTING)
		RemoveElementTry(questMgr->questContest->units, unit);
	if(!arena->free)
	{
		RemoveElementTry(arena->units, unit);
		if(arena->fighter == unit)
			arena->fighter = nullptr;
	}

	if(unit->usable)
	{
		unit->usable->user = nullptr;
		unit->usable = nullptr;
	}

	if(unit->bubble)
	{
		unit->bubble->unit = nullptr;
		unit->bubble = nullptr;
	}
	unit->talking = false;

	if(unit->interp)
	{
		EntityInterpolator::Pool.Free(unit->interp);
		unit->interp = nullptr;
	}

	if(unit->ai)
	{
		RemoveElement(ais, unit->ai);
		delete unit->ai;
		unit->ai = nullptr;
	}

	if(Net::IsLocal() && unit->IsHero() && unit->hero->otherTeam)
		unit->hero->otherTeam->Remove(unit);

	if(unit->cobj)
	{
		delete unit->cobj->getCollisionShape();
		phyWorld->removeCollisionObject(unit->cobj);
		delete unit->cobj;
		unit->cobj = nullptr;
	}

	delete unit->meshInst;
	unit->meshInst = nullptr;

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->id;
	}
}

void Game::OnCloseInventory()
{
	if(gameGui->inventory->mode == I_TRADE)
	{
		if(Net::IsLocal())
		{
			pc->actionUnit->busy = Unit::Busy_No;
			pc->actionUnit->lookTarget = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(gameGui->inventory->mode == I_SHARE || gameGui->inventory->mode == I_GIVE)
	{
		if(Net::IsLocal())
		{
			pc->actionUnit->busy = Unit::Busy_No;
			pc->actionUnit->lookTarget = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(gameGui->inventory->mode == I_LOOT_CHEST && Net::IsLocal())
		pc->actionChest->OpenClose(nullptr);
	else if(gameGui->inventory->mode == I_LOOT_CONTAINER)
	{
		if(Net::IsLocal())
		{
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_USABLE;
				c.unit = pc->unit;
				c.id = pc->unit->usable->id;
				c.count = USE_USABLE_END;
			}
			pc->unit->UseUsable(nullptr);
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}

	if(Net::IsOnline() && (gameGui->inventory->mode == I_LOOT_BODY || gameGui->inventory->mode == I_LOOT_CHEST))
	{
		if(Net::IsClient())
			Net::PushChange(NetChange::STOP_TRADE);
		else if(gameGui->inventory->mode == I_LOOT_BODY)
			pc->actionUnit->busy = Unit::Busy_No;
	}

	if(Any(pc->nextAction, NA_PUT, NA_GIVE, NA_SELL))
		pc->nextAction = NA_NONE;

	pc->action = PlayerAction::None;
	gameGui->inventory->mode = I_NONE;
}

void Game::CloseInventory()
{
	OnCloseInventory();
	gameGui->inventory->mode = I_NONE;
	if(gameGui->levelGui)
	{
		gameGui->inventory->invMine->Hide();
		gameGui->inventory->gpTrade->Hide();
	}
}

bool Game::CanShowEndScreen()
{
	if(Net::IsLocal())
		return !questMgr->uniqueCompletedShow && questMgr->uniqueQuestsCompleted == questMgr->uniqueQuests && gameLevel->cityCtx && !dialogContext.dialogMode && pc->unit->IsStanding();
	else
		return questMgr->uniqueCompletedShow && gameLevel->cityCtx && !dialogContext.dialogMode && pc->unit->IsStanding();
}

void Game::UpdateGameNet(float dt)
{
	if(gameGui->infoBox->visible)
		return;

	if(Net::IsServer())
		net->UpdateServer(dt);
	else
		net->UpdateClient(dt);
}

DialogContext* Game::FindDialogContext(Unit* talker)
{
	assert(talker);
	if(dialogContext.dialogMode && dialogContext.talker == talker)
		return &dialogContext;
	if(Net::IsOnline())
	{
		for(PlayerInfo& info : net->players)
		{
			DialogContext* ctx = info.pc->dialogCtx;
			if(ctx->dialogMode && ctx->talker == talker)
				return ctx;
		}
	}
	return nullptr;
}

void Game::OnEnterLocation()
{
	Unit* talker = nullptr;
	cstring text = nullptr;

	// orc talking after entering location
	if(questMgr->questOrcs2->orcsState == Quest_Orcs2::State::ToldAboutCamp && questMgr->questOrcs2->targetLoc == gameLevel->location
		&& questMgr->questOrcs2->talked == Quest_Orcs2::Talked::No)
	{
		questMgr->questOrcs2->talked = Quest_Orcs2::Talked::AboutCamp;
		talker = questMgr->questOrcs2->orc;
		text = txOrcCamp;
	}

	if(!talker)
	{
		TeamInfo info;
		team->GetTeamInfo(info);

		if(info.saneHeroes > 0)
		{
			bool always_use = false;
			switch(gameLevel->location->type)
			{
			case L_CITY:
				if(LocationHelper::IsCity(gameLevel->location))
					text = RandomString(txAiCity);
				else
					text = RandomString(txAiVillage);
				break;
			case L_OUTSIDE:
				switch(gameLevel->location->target)
				{
				case FOREST:
				default:
					text = txAiForest;
					break;
				case MOONWELL:
					text = txAiMoonwell;
					break;
				case ACADEMY:
					text = txAiAcademy;
					break;
				}
				break;
			case L_CAMP:
				if(gameLevel->location->state != LS_CLEARED && !gameLevel->location->group->IsEmpty())
				{
					if(!gameLevel->location->group->name2.empty())
					{
						always_use = true;
						text = Format(txAiCampFull, gameLevel->location->group->name2.c_str());
					}
				}
				else
					text = txAiCampEmpty;
				break;
			}

			if(text && (always_use || Rand() % 2 == 0))
				talker = team->GetRandomSaneHero();
		}
	}

	if(talker)
		talker->Talk(text);
}

void Game::OnEnterLevel()
{
	Unit* talker = nullptr;
	cstring text = nullptr;

	// cleric talking after entering location
	Quest_Evil* questEvil = questMgr->questEvil;
	if(questEvil->evilState == Quest_Evil::State::ClosingPortals || questEvil->evilState == Quest_Evil::State::KillBoss)
	{
		if(questEvil->evilState == Quest_Evil::State::ClosingPortals)
		{
			int d = questEvil->GetLocId(gameLevel->location);
			if(d != -1)
			{
				Quest_Evil::Loc& loc = questEvil->loc[d];

				if(gameLevel->dungeonLevel == gameLevel->location->GetLastLevel())
				{
					if(loc.state < Quest_Evil::Loc::TalkedAfterEnterLevel)
					{
						talker = questEvil->cleric;
						text = txPortalCloseLevel;
						loc.state = Quest_Evil::Loc::TalkedAfterEnterLevel;
					}
				}
				else if(gameLevel->dungeonLevel == 0 && loc.state == Quest_Evil::Loc::None)
				{
					talker = questEvil->cleric;
					text = txPortalClose;
					loc.state = Quest_Evil::Loc::TalkedAfterEnterLocation;
				}
			}
		}
		else if(gameLevel->location == questEvil->targetLoc && !questEvil->toldAboutBoss)
		{
			questEvil->toldAboutBoss = true;
			talker = questEvil->cleric;
			text = txXarDanger;
		}
	}

	// orc talking after entering level
	Quest_Orcs2* questOrcs2 = questMgr->questOrcs2;
	if(!talker && (questOrcs2->orcsState == Quest_Orcs2::State::GenerateOrcs || questOrcs2->orcsState == Quest_Orcs2::State::GeneratedOrcs) && gameLevel->location == questOrcs2->targetLoc)
	{
		if(gameLevel->dungeonLevel == 0)
		{
			if(questOrcs2->talked < Quest_Orcs2::Talked::AboutBase)
			{
				questOrcs2->talked = Quest_Orcs2::Talked::AboutBase;
				talker = questOrcs2->orc;
				text = txGorushDanger;
			}
		}
		else if(gameLevel->dungeonLevel == gameLevel->location->GetLastLevel())
		{
			if(questOrcs2->talked < Quest_Orcs2::Talked::AboutBoss)
			{
				questOrcs2->talked = Quest_Orcs2::Talked::AboutBoss;
				talker = questOrcs2->orc;
				text = txGorushCombat;
			}
		}
	}

	// old mage talking after entering location
	Quest_Mages2* questMages2 = questMgr->questMages2;
	if(!talker && (questMages2->magesState == Quest_Mages2::State::OldMageJoined || questMages2->magesState == Quest_Mages2::State::MageRecruited))
	{
		if(questMages2->targetLoc == gameLevel->location)
		{
			if(questMages2->magesState == Quest_Mages2::State::OldMageJoined)
			{
				if(gameLevel->dungeonLevel == 0 && questMages2->talked == Quest_Mages2::Talked::No)
				{
					questMages2->talked = Quest_Mages2::Talked::AboutHisTower;
					text = txMageHere;
				}
			}
			else
			{
				if(gameLevel->dungeonLevel == 0)
				{
					if(questMages2->talked < Quest_Mages2::Talked::AfterEnter)
					{
						questMages2->talked = Quest_Mages2::Talked::AfterEnter;
						text = Format(txMageEnter, questMages2->evilMageName.c_str());
					}
				}
				else if(gameLevel->dungeonLevel == gameLevel->location->GetLastLevel() && questMages2->talked < Quest_Mages2::Talked::BeforeBoss)
				{
					questMages2->talked = Quest_Mages2::Talked::BeforeBoss;
					text = txMageFinal;
				}
			}
		}

		if(text)
			talker = team->FindTeamMember("q_magowie_stary");
	}

	// default talking about location
	if(!talker && gameLevel->dungeonLevel == 0 && (gameLevel->enterFrom == ENTER_FROM_OUTSIDE || gameLevel->enterFrom >= ENTER_FROM_PORTAL))
	{
		TeamInfo info;
		team->GetTeamInfo(info);

		if(info.saneHeroes > 0)
		{
			LocalString s;

			switch(gameLevel->location->type)
			{
			case L_DUNGEON:
				{
					InsideLocation* inside = (InsideLocation*)gameLevel->location;
					switch(inside->target)
					{
					case HUMAN_FORT:
					case THRONE_FORT:
					case TUTORIAL_FORT:
						s = txAiFort;
						break;
					case DWARF_FORT:
						s = txAiDwarfFort;
						break;
					case MAGE_TOWER:
						s = txAiTower;
						break;
					case ANCIENT_ARMORY:
						s = txAiArmory;
						break;
					case BANDITS_HIDEOUT:
						s = txAiHideout;
						break;
					case VAULT:
					case THRONE_VAULT:
						s = txAiVault;
						break;
					case HERO_CRYPT:
					case MONSTER_CRYPT:
						s = txAiCrypt;
						break;
					case OLD_TEMPLE:
						s = txAiTemple;
						break;
					case NECROMANCER_BASE:
						s = txAiNecromancerBase;
						break;
					case LABYRINTH:
						s = txAiLabyrinth;
						break;
					}

					if(inside->group->IsEmpty())
						s += txAiNoEnemies;
					else
						s += Format(txAiNearEnemies, inside->group->name2.c_str());
				}
				break;
			case L_CAVE:
				s = txAiCave;
				break;
			}

			team->GetRandomSaneHero()->Talk(s->c_str());
			return;
		}
	}

	if(talker)
		talker->Talk(text);
}

void Game::OnEnterLevelOrLocation()
{
	gameGui->Clear(false, true);
	pc->data.autowalk = false;
	pc->data.selectedUnit = pc->unit;
	fallbackTimer = -0.5f;
	fallbackType = FALLBACK::NONE;
	if(Net::IsLocal())
	{
		for(Unit& unit : team->members)
			unit.frozen = FROZEN::NO;
	}

	// events v2
	for(Event& e : gameLevel->location->events)
	{
		if(e.type == EVENT_ENTER)
		{
			ScriptEvent event(EVENT_ENTER);
			event.onEnter.location = gameLevel->location;
			e.quest->FireEvent(event);
		}
	}

	for(LocationPart& locPart : gameLevel->ForEachPart())
		locPart.BuildScene();

	if(Net::IsClient() && gameLevel->boss)
		gameGui->levelGui->SetBoss(gameLevel->boss, true);
}
