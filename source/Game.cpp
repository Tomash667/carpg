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
#include <Profiler.h>
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
CustomCollisionWorld* phy_world;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;
void HumanPredraw(void* ptr, Matrix* mat, int n);

//=================================================================================================
Game::Game() : quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0), draw_particle_sphere(false), draw_unit_radius(false),
draw_hitbox(false), noai(false), testing(false), game_speed(1.f), devmode(false), force_seed(0), next_seed(0), force_seed_all(false), dont_wander(false),
check_updates(true), skip_tutorial(false), portal_anim(0), music_type(MusicType::None), end_of_game(false), prepared_stream(64 * 1024), paused(false),
draw_flags(0xFFFFFFFF), prev_game_state(GS_LOAD), rt_save(nullptr), rt_item_rot(nullptr), use_postfx(true), mp_timeout(10.f),
profiler_mode(ProfilerMode::Disabled), screenshot_format(ImageFormat::JPG), game_state(GS_LOAD), default_devmode(false), default_player_devmode(false),
quickstart_slot(SaveSlot::MAX_SLOTS), clear_color(Color::Black), in_load(false), tMinimap(nullptr)
{
	if(IsDebug())
	{
		default_devmode = true;
		default_player_devmode = true;
	}
	devmode = default_devmode;

	dialog_context.is_local = true;

	LocalString s;
	GetTitle(s);
	engine->SetTitle(s.c_str());

	uv_mod = Terrain::DEFAULT_UV_MOD;

	SetupConfigVars();

	aiMgr = new AIManager;
	arena = new Arena;
	cmdp = new CommandParser;
	dun_mesh_builder = new DungeonMeshBuilder;
	game_gui = new GameGui;
	game_level = new Level;
	game_res = new GameResources;
	game_stats = new GameStats;
	loc_gen_factory = new LocationGeneratorFactory;
	messenger = new Messenger;
	net = new Net;
	pathfinding = new Pathfinding;
	quest_mgr = new QuestManager;
	script_mgr = new ScriptManager;
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
	game_state = GS_LOAD_MENU;

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

	phy_world = engine->GetPhysicsWorld();
	engine->UnlockCursor(false);

	// set animesh callback
	MeshInstance::Predraw = HumanPredraw;

	game_gui->PreInit();

	PreloadLanguage();
	PreloadData();
	res_mgr->SetProgressCallback(ProgressCallback(this, &Game::OnLoadProgress));
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
	res_mgr->AddDir("data/preload");

	GameGui::font = game_gui->gui->GetFont("Arial", 12, 8, 2);

	// loadscreen textures
	game_gui->load_screen->LoadData();

	// intro music
	if(!sound_mgr->IsMusicDisabled())
	{
		Ptr<MusicTrack> track;
		track->music = res_mgr->Load<Music>("Intro.ogg");
		track->type = MusicType::Intro;
		MusicTrack::tracks.push_back(track.Pin());
		SetMusic(MusicType::Intro);
	}
}

//=================================================================================================
// Load system.
//=================================================================================================
void Game::LoadSystem()
{
	Info("Game: Loading system.");
	game_gui->load_screen->Setup(0.f, 0.33f, 16, txCreatingListOfFiles);

	AddFilesystem();
	arena->Init();
	game_gui->Init();
	game_res->Init();
	net->Init();
	quest_mgr->Init();
	script_mgr->Init();
	game_gui->main_menu->UpdateCheckVersion();
	LoadDatafiles();
	LoadLanguageFiles();
	game_gui->server->Init();
	game_gui->load_screen->Tick(txLoadingShaders);
	ConfigureGame();
}

//=================================================================================================
// Add filesystem.
//=================================================================================================
void Game::AddFilesystem()
{
	Info("Game: Creating list of files.");
	res_mgr->AddDir("data");
	res_mgr->AddPak("data/data.pak", "KrystaliceFire");
}

//=================================================================================================
// Load datafiles (items/units/etc).
//=================================================================================================
void Game::LoadDatafiles()
{
	Info("Game: Loading system.");
	load_errors = 0;
	load_warnings = 0;

	// content
	content.system_dir = g_system_dir;
	content.LoadContent([this](Content::Id id)
	{
		switch(id)
		{
		case Content::Id::Abilities:
			game_gui->load_screen->Tick(txLoadingAbilities);
			break;
		case Content::Id::Buildings:
			game_gui->load_screen->Tick(txLoadingBuildings);
			break;
		case Content::Id::Classes:
			game_gui->load_screen->Tick(txLoadingClasses);
			break;
		case Content::Id::Dialogs:
			game_gui->load_screen->Tick(txLoadingDialogs);
			break;
		case Content::Id::Items:
			game_gui->load_screen->Tick(txLoadingItems);
			break;
		case Content::Id::Locations:
			game_gui->load_screen->Tick(txLoadingLocations);
			break;
		case Content::Id::Musics:
			game_gui->load_screen->Tick(txLoadingMusics);
			break;
		case Content::Id::Objects:
			game_gui->load_screen->Tick(txLoadingObjects);
			break;
		case Content::Id::Perks:
			game_gui->load_screen->Tick(txLoadingPerks);
			break;
		case Content::Id::Quests:
			game_gui->load_screen->Tick(txLoadingQuests);
			break;
		case Content::Id::Required:
			game_gui->load_screen->Tick(txLoadingRequired);
			break;
		case Content::Id::Units:
			game_gui->load_screen->Tick(txLoadingUnits);
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
	game_gui->load_screen->Tick(txLoadingLanguageFiles);

	Language::LoadLanguageFiles();

	txHaveErrors = Str("haveErrors");
	SetGameCommonText();
	SetItemStatsText();
	NameHelper::SetHeroNames();
	SetGameText();
	SetStatsText();
	arena->LoadLanguage();
	game_gui->LoadLanguage();
	game_level->LoadLanguage();
	game_res->LoadLanguage();
	net->LoadLanguage();
	quest_mgr->LoadLanguage();
	world->LoadLanguage();

	uint language_errors = Language::GetErrors();
	if(language_errors != 0)
	{
		Error("Game: %u language errors.", language_errors);
		load_warnings += language_errors;
	}
}

//=================================================================================================
// Initialize everything needed by game before loading content.
//=================================================================================================
void Game::ConfigureGame()
{
	Info("Game: Configuring game.");
	game_gui->load_screen->Tick(txConfiguringGame);

	cmdp->AddCommands();
	settings.ResetGameKeys();
	settings.LoadGameKeys(cfg);
	load_errors += BaseLocation::SetRoomPointers();

	// shaders
	render->RegisterShader(basic_shader = new BasicShader);
	render->RegisterShader(grass_shader = new GrassShader);
	render->RegisterShader(particle_shader = new ParticleShader);
	render->RegisterShader(postfx_shader = new PostfxShader);
	render->RegisterShader(skybox_shader = new SkyboxShader);
	render->RegisterShader(terrain_shader = new TerrainShader);
	render->RegisterShader(glow_shader = new GlowShader(postfx_shader));

	tMinimap = render->CreateDynamicTexture(Int2(128));
	CreateRenderTargets();
}

//=================================================================================================
// Load game data.
//=================================================================================================
void Game::LoadData()
{
	Info("Game: Loading data.");

	res_mgr->PrepareLoadScreen(0.33f);
	game_gui->load_screen->Tick(txPreloadAssets);
	game_res->LoadData();
	res_mgr->StartLoadScreen();
}

//=================================================================================================
// Configure game after loading content.
//=================================================================================================
void Game::PostconfigureGame()
{
	Info("Game: Postconfiguring game.");

	engine->LockCursor();
	game_gui->PostInit();
	game_level->Init();
	loc_gen_factory->Init();
	world->Init();
	quest_mgr->InitLists();
	dun_mesh_builder->Build();

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
	load_errors += content.errors;
	load_warnings += content.warnings;
	if(load_errors > 0 || load_warnings > 0)
	{
		// show message in release, notification in debug
		if(load_errors > 0)
			Error("Game: %u loading errors, %u warnings.", load_errors, load_warnings);
		else
			Warn("Game: %u loading warnings.", load_warnings);
		Texture* img = (load_errors > 0 ? game_res->tError : game_res->tWarning);
		cstring text = Format(txHaveErrors, load_errors, load_warnings);
		if(IsDebug())
			game_gui->notifications->Add(text, img, 5.f);
		else
		{
			DialogInfo info;
			info.name = "have_errors";
			info.text = text;
			info.type = DIALOG_OK;
			info.img = img;
			info.event = [this](int result) { StartGameMode(); };
			info.parent = game_gui->main_menu;
			info.order = ORDER_TOPMOST;
			info.pause = false;
			info.auto_wrap = true;
			gui->ShowDialog(info);
			start_game_mode = false;
		}
	}

	// save config
	cfg.Add("adapter", render->GetAdapter());
	cfg.Add("resolution", engine->GetWindowSize());
	SaveCfg();

	// end load screen, show menu
	clear_color = Color::Black;
	game_state = GS_MAIN_MENU;
	game_gui->load_screen->visible = false;
	game_gui->main_menu->Show();
	if(music_type != MusicType::Intro)
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
		if(player_name.empty())
		{
			Warn("Quickstart: Can't create server, no player nick.");
			break;
		}
		if(net->server_name.empty())
		{
			Warn("Quickstart: Can't create server, no server name.");
			break;
		}

		if(quickstart == QUICKSTART_LOAD_MP)
		{
			net->mp_load = true;
			net->mp_quickload = false;
			if(TryLoadGame(quickstart_slot, false, false))
			{
				game_gui->create_server->CloseDialog();
				game_gui->server->autoready = true;
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
		if(!player_name.empty())
		{
			game_gui->server->autoready = true;
			game_gui->pick_server->Show(true);
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_JOIN_IP:
		if(!player_name.empty())
		{
			if(!server_ip.empty())
			{
				game_gui->server->autoready = true;
				QuickJoinIp();
			}
			else
				Warn("Quickstart: Can't join server, no server ip.");
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_LOAD:
		if(!TryLoadGame(quickstart_slot, false, false))
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
	if(game_state != GS_QUIT && game_state != GS_LOAD_MENU)
		ClearGame();

	if(game_gui && game_gui->main_menu)
		game_gui->main_menu->ShutdownThread();

	content.CleanupContent();

	ClearQuadtree();

	draw_batch.Clear();
	delete tMinimap;

	Language::Cleanup();

	delete aiMgr;
	delete arena;
	delete cmdp;
	delete dun_mesh_builder;
	delete game_gui;
	delete game_level;
	delete game_res;
	delete game_stats;
	delete loc_gen_factory;
	delete messenger;
	delete net;
	delete pathfinding;
	delete quest_mgr;
	delete script_mgr;
	delete team;
	delete world;
}

//=================================================================================================
void Game::OnDraw()
{
	if(profiler_mode == ProfilerMode::Rendering)
		Profiler::g_profiler.Start();
	else if(profiler_mode == ProfilerMode::Disabled)
		Profiler::g_profiler.Clear();

	DrawGame();
	render->Present();

	Profiler::g_profiler.End();
}

//=================================================================================================
void Game::DrawGame()
{
	PROFILER_BLOCK("Draw");

	if(game_state == GS_LEVEL)
	{
		LevelArea& area = *pc->unit->area;
		bool outside;
		if(area.area_type == LevelArea::Type::Outside)
			outside = true;
		else if(area.area_type == LevelArea::Type::Inside)
			outside = false;
		else if(game_level->city_ctx->inside_buildings[area.area_id]->top > 0.f)
			outside = false;
		else
			outside = true;

		ListDrawObjects(area, game_level->camera.frustum, outside);

		vector<PostEffect> postEffects;
		GetPostEffects(postEffects);

		const bool usePostfx = !postEffects.empty();
		const bool useGlow = !draw_batch.glow_nodes.empty();

		if(usePostfx || useGlow)
			postfx_shader->SetTarget();

		render->Clear(clear_color);

		// draw level
		DrawScene(outside);

		// draw glow
		if(useGlow)
		{
			PROFILER_BLOCK("DrawGlowingNodes");
			glow_shader->Draw(game_level->camera, draw_batch.glow_nodes, usePostfx);
		}

		if(usePostfx)
		{
			PROFILER_BLOCK("DrawPostFx");
			if(!useGlow)
				postfx_shader->Prepare();
			postfx_shader->Draw(postEffects);
		}
	}
	else
		render->Clear(clear_color);

	// draw gui
	game_gui->Draw(game_level->camera.mat_view_proj, IsSet(draw_flags, DF_GUI), IsSet(draw_flags, DF_MENU));
}

//=================================================================================================
void HumanPredraw(void* ptr, Matrix* mat, int n)
{
	if(n != 0)
		return;

	Unit* u = static_cast<Unit*>(ptr);

	if(u->data->type == UNIT_TYPE::HUMAN)
	{
		int bone = u->mesh_inst->mesh->GetBone("usta")->id;
		static Matrix mat2;
		float val = u->talking ? sin(u->talk_timer * 6) : 0.f;
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

	if(profiler_mode == ProfilerMode::Update)
		Profiler::g_profiler.Start();
	else if(profiler_mode == ProfilerMode::Disabled)
		Profiler::g_profiler.Clear();

	api->Update();
	script_mgr->UpdateScripts(dt);

	UpdateMusic();

	if(Net::IsSingleplayer() || !paused)
	{
		// update time spent in game
		if(game_state != GS_MAIN_MENU && game_state != GS_LOAD)
			game_stats->Update(dt);
	}

	GKey.allow_input = GameKeys::ALLOW_INPUT;

	if(!engine->IsActive() || !engine->IsCursorLocked())
	{
		input->SetFocus(false);
		if(Net::IsSingleplayer() && !inactive_update)
		{
			Profiler::g_profiler.End();
			return;
		}
	}
	else
		input->SetFocus(true);

	// fast quit (alt+f4)
	if(input->Focus() && input->Shortcut(KEY_ALT, Key::F4) && !gui->HaveTopDialog("dialog_alt_f4"))
		game_gui->ShowQuitDialog();

	if(end_of_game)
	{
		death_fade += dt;
		GKey.allow_input = GameKeys::ALLOW_NONE;
	}

	// global keys handling
	if(GKey.allow_input == GameKeys::ALLOW_INPUT)
	{
		// handle open/close of console
		if(!gui->HaveTopDialog("dialog_alt_f4") && !gui->HaveDialog("console") && GKey.KeyDownUpAllowed(GK_CONSOLE))
			gui->ShowDialog(game_gui->console);

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
		if(GKey.KeyPressedReleaseAllowed(GK_PAUSE) && !Net::IsClient())
			PauseGame();

		if(!cutscene)
		{
			// quicksave, quickload
			bool console_open = gui->HaveTopDialog("console");
			bool special_key_allowed = (GKey.allow_input == GameKeys::ALLOW_KEYBOARD || GKey.allow_input == GameKeys::ALLOW_INPUT || (!gui->HaveDialog() || console_open));
			if(GKey.KeyPressedReleaseSpecial(GK_QUICKSAVE, special_key_allowed))
				Quicksave(console_open);
			if(GKey.KeyPressedReleaseSpecial(GK_QUICKLOAD, special_key_allowed))
				Quickload(console_open);
		}
	}

	// update game gui
	game_gui->UpdateGui(dt);
	if(game_state == GS_EXIT_TO_MENU)
	{
		ExitToMenu();
		return;
	}
	else if(game_state == GS_QUIT)
	{
		ClearGame();
		engine->Shutdown();
		return;
	}

	// open game menu
	if(GKey.AllowKeyboard() && CanShowMenu() && !cutscene && input->PressedRelease(Key::Escape))
		game_gui->ShowMenu();

	// update game
	if(!end_of_game && !cutscene)
	{
		arena->UpdatePvpRequest(dt);

		// update fallback, can leave level so we need to check if we are still in GS_LEVEL
		if(game_state == GS_LEVEL)
			UpdateFallback(dt);

		bool isPaused = false;

		if(game_state == GS_LEVEL)
		{
			isPaused = (paused || (gui->HavePauseDialog() && Net::IsSingleplayer()));
			if(!isPaused)
				UpdateGame(dt);
		}

		if(game_state == GS_LEVEL)
		{
			if(Net::IsLocal())
			{
				if(Net::IsOnline())
					net->UpdateWarpData(dt);
				game_level->ProcessUnitWarps();
			}
			game_level->camera.Update(isPaused ? 0.f : dt);
		}
	}
	else if(cutscene)
		UpdateFallback(dt);

	if(Net::IsOnline() && Any(game_state, GS_LEVEL, GS_WORLDMAP))
		UpdateGameNet(dt);

	messenger->Process();

	if(Net::IsSingleplayer() && game_state != GS_MAIN_MENU)
	{
		assert(Net::changes.empty());
	}

	Profiler::g_profiler.End();
}

//=================================================================================================
void Game::GetTitle(LocalString& s)
{
	s = "CaRpg " VERSION_STR;
	bool none = true;

	if(IsDebug())
	{
		none = false;
		s += " - DEBUG";
	}

	if((game_state != GS_MAIN_MENU && game_state != GS_LOAD) || (game_gui && game_gui->server && game_gui->server->visible))
	{
		if(none)
			s += " - ";
		else
			s += ", ";
		if(Net::IsOnline())
		{
			if(Net::IsServer())
				s += "SERVER";
			else
				s += "CLIENT";
		}
		else
			s += "SINGLE";
	}

	s += Format(" [%d]", GetCurrentProcessId());
}

//=================================================================================================
void Game::ChangeTitle()
{
	LocalString s;
	GetTitle(s);
	SetConsoleTitle(s->c_str());
	engine->SetTitle(s->c_str());
}

//=================================================================================================
void Game::TakeScreenshot(bool no_gui)
{
	if(no_gui)
	{
		int old_flags = draw_flags;
		draw_flags = (0xFFFF & ~DF_GUI);
		DrawGame();
		draw_flags = old_flags;
	}
	else
		DrawGame();

	io::CreateDirectory("screenshots");

	time_t t = ::time(0);
	tm lt;
	localtime_s(&lt, &t);

	if(t == last_screenshot)
		++screenshot_count;
	else
	{
		last_screenshot = t;
		screenshot_count = 1;
	}

	cstring path = Format("screenshots\\%04d%02d%02d_%02d%02d%02d_%02d.%s", lt.tm_year + 1900, lt.tm_mon + 1,
		lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, screenshot_count, ImageFormatMethods::GetExtension(screenshot_format));

	render->SaveScreenshot(path, screenshot_format);

	cstring msg = Format("Screenshot saved to '%s'.", path);
	game_gui->console->AddMsg(msg);
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
	prev_game_state = game_state;
	game_state = GS_EXIT_TO_MENU;

	StopAllSounds();
	ClearGame();

	if(res_mgr->IsLoadScreen())
		res_mgr->CancelLoadScreen(true);

	game_state = GS_MAIN_MENU;
	paused = false;
	net->mp_load = false;
	net->was_client = false;

	SetMusic(MusicType::Title);
	end_of_game = false;

	game_gui->CloseAllPanels();
	string msg;
	DialogBox* box = gui->GetDialog("fatal");
	bool console = game_gui->console->visible;
	if(box)
		msg = box->text;
	gui->CloseDialogs();
	if(!msg.empty())
		gui->SimpleDialog(msg.c_str(), nullptr, "fatal");
	if(console)
		gui->ShowDialog(game_gui->console);
	game_gui->game_menu->visible = false;
	game_gui->level_gui->visible = false;
	game_gui->world_map->Hide();
	game_gui->main_menu->Show();
	units_mesh_load.clear();

	ChangeTitle();
}

//=================================================================================================
void Game::LoadCfg()
{
	// last save
	lastSave = cfg.GetInt("lastSave", -1);
	if(lastSave < 1 || lastSave > SaveSlot::MAX_SLOTS)
		lastSave = -1;

	// miscellaneous
	check_updates = cfg.GetBool("check_updates", true);
	skip_tutorial = cfg.GetBool("skip_tutorial", false);
}

//=================================================================================================
void Game::SaveCfg()
{
	if(cfg.Save(cfg_file.c_str()) == Config::CANT_SAVE)
		Error("Failed to save configuration file '%s'!", cfg_file.c_str());
}

//=================================================================================================
void Game::CreateRenderTargets()
{
	rt_save = render->CreateRenderTarget(Int2(256, 256));
	rt_item_rot = render->CreateRenderTarget(Int2(128, 128));
}

//=================================================================================================
void Game::RestartGame()
{
	// create mutex
	HANDLE mutex = CreateMutex(nullptr, TRUE, RESTART_MUTEX_NAME);
	DWORD dwLastError = GetLastError();
	bool AlreadyRunning = (dwLastError == ERROR_ALREADY_EXISTS || dwLastError == ERROR_ACCESS_DENIED);
	if(AlreadyRunning)
	{
		WaitForSingleObject(mutex, INFINITE);
		CloseHandle(mutex);
		return;
	}

	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi = { 0 };

	// append -restart to cmdline, hopefuly noone will use it 100 times in a row to overflow Format
	CreateProcess(nullptr, (char*)Format("%s -restart", GetCommandLine()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

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
	txServer = Str("server");
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
	if(!use_postfx)
		return;

	// vignette
	PostEffect effect;
	effect.id = POSTFX_VIGNETTE;
	effect.power = 1.f;
	effect.skill = Vec4(0.9f, 0.9f, 0.9f, 1.f);
	effect.tex = game_res->tVignette->tex;
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
		err += res_mgr->VerifyResources();

	if(err == 0)
		Info("Test: Validation succeeded.");
	else
		Error("Test: Validation failed, %u errors found.", err);

	content.warnings += err;
	return err;
}

void Game::SetupConfigVars()
{
	cfg.AddVar(ConfigVar("devmode", default_devmode));
	cfg.AddVar(ConfigVar("players_devmode", default_player_devmode));
}

cstring Game::GetShortcutText(GAME_KEYS key, cstring action)
{
	auto& k = GKey[key];
	bool first_key = (k[0] != Key::None),
		second_key = (k[1] != Key::None);
	if(!action)
		action = k.text;

	if(first_key && second_key)
		return Format("%s (%s, %s)", action, game_gui->controls->GetKeyText(k[0]), game_gui->controls->GetKeyText(k[1]));
	else if(first_key || second_key)
	{
		Key used = k[first_key ? 0 : 1];
		return Format("%s (%s)", action, game_gui->controls->GetKeyText(used));
	}
	else
		return action;
}

void Game::PauseGame()
{
	paused = !paused;
	if(Net::IsOnline())
	{
		game_gui->mp_box->Add(paused ? txGamePaused : txGameResumed);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PAUSED;
		c.id = (paused ? 1 : 0);
		if(paused && game_state == GS_WORLDMAP && world->GetState() == World::State::TRAVEL)
			Net::PushChange(NetChange::UPDATE_MAP_POS);
	}
}

void Game::GenerateWorld()
{
	if(next_seed != 0)
	{
		Srand(next_seed);
		next_seed = 0;
	}
	else if(force_seed != 0 && force_seed_all)
		Srand(force_seed);

	Info("Generating world, seed %u.", RandVal());

	world->GenerateWorld();

	Info("Randomness integrity: %d", RandVal());
}

void Game::EnterLocation(int level, int from_portal, bool close_portal)
{
	Location& l = *game_level->location;
	game_level->entering = true;
	game_level->lvl = nullptr;

	game_gui->world_map->Hide();
	game_gui->level_gui->Reset();
	game_gui->level_gui->visible = true;

	game_level->is_open = true;
	if(world->GetState() != World::State::INSIDE_ENCOUNTER)
		world->SetState(World::State::INSIDE_LOCATION);
	if(from_portal != -1)
		game_level->enter_from = ENTER_FROM_PORTAL + from_portal;
	else
		game_level->enter_from = ENTER_FROM_OUTSIDE;
	game_level->light_angle = Random(PI * 2);

	game_level->dungeon_level = level;
	game_level->event_handler = nullptr;
	pc->data.before_player = BP_NONE;
	arena->Reset();
	game_gui->inventory->lock = nullptr;

	bool first = false;

	if(l.last_visit == -1)
		first = true;

	InitQuadTree();

	if(Net::IsOnline() && net->active_players > 1)
	{
		BitStreamWriter f;
		f << ID_CHANGE_LEVEL;
		f << (byte)game_level->location_index;
		f << (byte)0;
		f << (world->GetState() == World::State::INSIDE_ENCOUNTER);
		int ack = net->SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
		for(PlayerInfo& info : net->players)
		{
			if(info.id == team->my_id)
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
	LocationGenerator* loc_gen = loc_gen_factory->Get(&l, first);
	int steps = loc_gen->GetNumberOfSteps();
	LoadingStart(steps);
	LoadingStep(txEnteringLocation);

	// generate map on first visit
	if(first)
	{
		if(next_seed != 0)
		{
			Srand(next_seed);
			next_seed = 0;
		}
		else if(force_seed != 0 && force_seed_all)
			Srand(force_seed);

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
			InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
				multi->infos[game_level->dungeon_level].seed = l.seed;
			}
		}

		// mark as visited
		if(l.state != LS_HIDDEN)
			l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		loc_gen->Generate();
	}
	else if(!Any(l.type, L_DUNGEON, L_CAVE))
		Info("Entering location '%s'.", l.name.c_str());

	if(game_level->location->outside)
	{
		loc_gen->OnEnter();
		SetTerrainTextures();
		CalculateQuadtree();
	}
	else
		EnterLevel(loc_gen);

	bool loaded_resources = game_level->location->RequireLoadingResources(nullptr);
	LoadResources(txLoadingComplete, false);

	l.last_visit = world->GetWorldtime();
	game_level->CheckIfLocationCleared();
	game_level->camera.Reset();
	pc->data.rot_buf = 0.f;
	SetMusic();

	if(close_portal)
	{
		delete game_level->location->portal;
		game_level->location->portal = nullptr;
	}

	if(game_level->location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	if(Net::IsOnline())
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		if(net->active_players > 1)
		{
			prepared_stream.Reset();
			BitStreamWriter f(prepared_stream);
			net->WriteLevelData(f, loaded_resources);
			Info("Generated location packet: %d.", prepared_stream.GetNumberOfBytesUsed());
		}
		else
			net->GetMe().state = PlayerInfo::IN_GAME;

		game_gui->info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color_next;
		game_state = GS_LEVEL;
		game_gui->load_screen->visible = false;
		game_gui->main_menu->visible = false;
		game_gui->level_gui->visible = true;
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");
	game_level->entering = false;
}

void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(!game_level->is_open)
		return;

	if(Net::IsLocal() && !net->was_client && !clear)
	{
		if(quest_mgr->quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			quest_mgr->quest_tournament->Clean();
		if(!arena->free)
			arena->Clean();
	}

	if(clear)
	{
		LeaveLevel(true);
		return;
	}

	Info("Leaving location.");

	if(Net::IsLocal() && (quest_mgr->quest_crazies->check_stone
		|| (quest_mgr->quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && quest_mgr->quest_crazies->crazies_state < Quest_Crazies::State::End)))
		quest_mgr->quest_crazies->CheckStone();

	// drinking contest
	Quest_Contest* contest = quest_mgr->quest_contest;
	if(contest->state >= Quest_Contest::CONTEST_STARTING)
		contest->Cleanup();

	// clear blood & bodies from orc base
	if(Net::IsLocal() && quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::ClearDungeon && game_level->location == quest_mgr->quest_orcs2->targetLoc)
	{
		quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::End;
		game_level->UpdateLocation(31, 100, false);
	}

	if(game_level->city_ctx && game_state != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// ai buy iteams when leaving city
		team->BuyTeamItems();
	}

	LeaveLevel();

	if(game_level->is_open)
	{
		if(Net::IsLocal())
			quest_mgr->RemoveQuestUnits(true);

		game_level->ProcessRemoveUnits(true);

		if(game_level->location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = static_cast<OutsideLocation*>(game_level->location);
			outside->Clear();
			outside->last_visit = -1;
		}
	}

	if(team->crazies_attack)
	{
		team->crazies_attack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	// end temporay effects
	if(Net::IsLocal() && end_buffs)
	{
		for(Unit& unit : team->members)
			unit.EndEffects();
	}

	game_level->is_open = false;
	game_level->city_ctx = nullptr;
	game_level->lvl = nullptr;
}

void Game::Event_RandomEncounter(int)
{
	game_gui->world_map->dialog_enc = nullptr;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CLOSE_ENCOUNTER);
	world->StartEncounter();
	EnterLocation();
}

//=================================================================================================
void Game::OnResize()
{
	game_gui->OnResize();
}

//=================================================================================================
void Game::OnFocus(bool focus, const Int2& activationPoint)
{
	game_gui->OnFocus(focus, activationPoint);
}

//=================================================================================================
void Game::ReportError(int id, cstring text, bool once)
{
	if(once)
	{
		for(int error : reported_errors)
		{
			if(id == error)
				return;
		}
		reported_errors.push_back(id);
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
		game_gui->messages->AddGameMsg(str, 5.f);
	api->Report(id, Format("[%s] %s", mode, text));
}

//=================================================================================================
void Game::CutsceneStart(bool instant)
{
	cutscene = true;
	game_gui->CloseAllPanels();
	game_gui->level_gui->ResetCutscene();
	fallback_type = FALLBACK::CUTSCENE;
	fallback_t = instant ? 0.f : -1.f;

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
		tex = res_mgr->TryLoadInstant<Texture>(image);
		if(!tex)
			Warn("CutsceneImage: missing texture '%s'.", image.c_str());
	}
	game_gui->level_gui->SetCutsceneImage(tex, time);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CUTSCENE_IMAGE;
		c.str = StringPool.Get();
		*c.str = image;
		c.f[0] = time;
		net->net_strs.push_back(c.str);
	}
}

//=================================================================================================
void Game::CutsceneText(const string& text, float time)
{
	assert(cutscene && time > 0);
	game_gui->level_gui->SetCutsceneText(text, time);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CUTSCENE_TEXT;
		c.str = StringPool.Get();
		*c.str = text;
		c.f[0] = time;
		net->net_strs.push_back(c.str);
	}
}

//=================================================================================================
void Game::CutsceneEnd()
{
	assert(cutscene);
	cutscene_script = script_mgr->SuspendScript();
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
	fallback_type = FALLBACK::CUTSCENE_END;
	fallback_t = -fallback_t;

	if(Net::IsLocal())
		script_mgr->ResumeScript(cutscene_script);
}

//=================================================================================================
bool Game::CutsceneShouldSkip()
{
	return cfg.GetBool("skip_cutscene");
}

//=================================================================================================
void Game::UpdateGame(float dt)
{
	dt *= game_speed;
	if(dt == 0)
		return;

	PROFILER_BLOCK("UpdateGame");

	// sanity checks
	if(IsDebug())
	{
		if(Net::IsLocal())
		{
			assert(pc->is_local);
			if(Net::IsServer())
			{
				for(PlayerInfo& info : net->players)
				{
					if(info.left == PlayerInfo::LEFT_NO)
					{
						assert(info.pc == info.pc->player_info->pc);
						if(info.id != 0)
							assert(!info.pc->is_local);
					}
				}
			}
		}
		else
			assert(pc->is_local && pc == pc->player_info->pc);
	}

	game_level->minimap_opened_doors = false;

	if(quest_mgr->quest_tutorial->in_tutorial && !Net::IsOnline())
		quest_mgr->quest_tutorial->Update();

	portal_anim += dt;
	if(portal_anim >= 1.f)
		portal_anim -= 1.f;
	game_level->light_angle = Clip(game_level->light_angle + dt / 100);
	game_level->scene->light_dir = Vec3(sin(game_level->light_angle), 2.f, cos(game_level->light_angle)).Normalize();

	if(Net::IsLocal() && !quest_mgr->quest_tutorial->in_tutorial)
	{
		// arena
		if(arena->mode != Arena::NONE)
			arena->Update(dt);

		// sharing of team items between team members
		team->UpdateTeamItemShares();

		// quests
		quest_mgr->UpdateQuestsLocal(dt);
	}

	// show info about completing all unique quests
	if(CanShowEndScreen())
	{
		if(Net::IsLocal())
			quest_mgr->unique_completed_show = true;
		else
			quest_mgr->unique_completed_show = false;

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
			text = hardcore_mode ? txWinHardcore : txWin;

		gui->SimpleDialog(Format(text, pc->kills, game_stats->total_kills - pc->kills), nullptr);
	}

	if(devmode && GKey.AllowKeyboard())
	{
		if(!game_level->location->outside)
		{
			InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
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
						game_level->WarpUnit(*pc->unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
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
						game_level->WarpUnit(*pc->unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
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
	if((Net::IsLocal() && !team->IsAnyoneAlive()) || death_screen != 0)
	{
		if(death_screen == 0)
		{
			Info("Game over: all players died.");
			SetMusic(MusicType::Death);
			game_gui->CloseAllPanels();
			++death_screen;
			death_fade = 0;
			death_solo = (team->GetTeamSize() == 1u);
			if(Net::IsOnline())
			{
				Net::PushChange(NetChange::GAME_STATS);
				Net::PushChange(NetChange::GAME_OVER);
			}
		}
		else if(death_screen == 1 || death_screen == 2)
		{
			death_fade += dt / 2;
			if(death_fade >= 1.f)
			{
				death_fade = 0;
				++death_screen;
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
	for(LevelArea& area : game_level->ForEachArea())
		area.Update(dt);

	// update minimap
	game_level->UpdateDungeonMinimap(true);

	// update dialogs
	if(Net::IsSingleplayer())
	{
		if(dialog_context.dialog_mode)
			dialog_context.Update(dt);
	}
	else if(Net::IsServer())
	{
		for(PlayerInfo& info : net->players)
		{
			if(info.left != PlayerInfo::LEFT_NO)
				continue;
			DialogContext& dialogCtx = *info.u->player->dialog_ctx;
			if(dialogCtx.dialog_mode)
			{
				if(!dialogCtx.talker->IsStanding() || !dialogCtx.talker->IsIdle() || dialogCtx.talker->to_remove || dialogCtx.talker->frozen != FROZEN::NO)
					dialogCtx.EndDialog();
				else
					dialogCtx.Update(dt);
			}
		}
	}
	else
	{
		if(dialog_context.dialog_mode)
			dialog_context.UpdateClient();
	}

	UpdateAttachedSounds(dt);

	game_level->ProcessRemoveUnits(false);
}

//=================================================================================================
void Game::UpdateFallback(float dt)
{
	if(fallback_type == FALLBACK::NO)
		return;

	if(fallback_t <= 0.f)
	{
		fallback_t += dt * 2;

		if(fallback_t > 0.f)
		{
			switch(fallback_type)
			{
			case FALLBACK::TRAIN:
				if(Net::IsLocal())
				{
					switch(fallback_1)
					{
					case 0:
						pc->Train(false, fallback_2);
						break;
					case 1:
						pc->Train(true, fallback_2);
						break;
					case 2:
						quest_mgr->quest_tournament->Train(*pc);
						break;
					case 3:
						pc->AddPerk(Perk::Get(fallback_2), -1);
						game_gui->messages->AddGameMsg3(GMS_LEARNED_PERK);
						break;
					case 4:
						pc->AddAbility(Ability::Get(fallback_2));
						game_gui->messages->AddGameMsg3(GMS_LEARNED_ABILITY);
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
					fallback_type = FALLBACK::CLIENT;
					fallback_t = 0.f;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRAIN;
					c.id = fallback_1;
					c.count = fallback_2;
				}
				break;
			case FALLBACK::REST:
				if(Net::IsLocal())
				{
					pc->Rest(fallback_1, true);
					if(Net::IsOnline())
						pc->UseDays(fallback_1);
					else
						world->Update(fallback_1, UM_NORMAL);
				}
				else
				{
					fallback_type = FALLBACK::CLIENT;
					fallback_t = 0.f;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::REST;
					c.id = fallback_1;
				}
				break;
			case FALLBACK::ENTER: // enter/exit building
				game_level->WarpUnit(pc->unit, fallback_1, fallback_2);
				break;
			case FALLBACK::EXIT:
				ExitToMap();
				break;
			case FALLBACK::CHANGE_LEVEL:
				ChangeLevel(fallback_1);
				break;
			case FALLBACK::USE_PORTAL:
				{
					LeaveLocation(false, false);
					Portal* portal = game_level->location->GetPortal(fallback_1);
					world->ChangeLevel(portal->target_loc, false);
					// currently it is only allowed to teleport from X level to first, can teleport back because X level is already visited
					int at_level = 0;
					if(game_level->location->portal)
						at_level = game_level->location->portal->at_level;
					EnterLocation(at_level, portal->index);
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
				fallback_t = 0.f;
				break;
			default:
				assert(0);
				break;
			}
		}
	}
	else
	{
		fallback_t += dt * 2;

		if(fallback_t >= 1.f)
		{
			if(Net::IsLocal())
			{
				if(fallback_type != FALLBACK::ARENA2)
				{
					if(fallback_type == FALLBACK::CHANGE_LEVEL || fallback_type == FALLBACK::USE_PORTAL || fallback_type == FALLBACK::EXIT)
					{
						for(Unit& unit : team->members)
							unit.frozen = FROZEN::NO;
					}
					pc->unit->frozen = FROZEN::NO;
				}
			}
			else if(fallback_type == FALLBACK::CLIENT2)
			{
				pc->unit->frozen = FROZEN::NO;
				Net::PushChange(NetChange::END_FALLBACK);
			}
			fallback_type = FALLBACK::NO;
		}
	}
}

//=================================================================================================
void Game::UpdateCamera(float dt)
{
	GameCamera& camera = game_level->camera;

	// rotate camera up/down
	if(team->IsAnyoneAlive())
	{
		if(dialog_context.dialog_mode || game_gui->inventory->mode > I_INVENTORY || game_gui->craft->visible)
		{
			// in dialog/trade look at target
			camera.RotateTo(dt, 4.2875104f);
			Vec3 zoom_pos;
			if(game_gui->inventory->mode == I_LOOT_CHEST)
				zoom_pos = pc->action_chest->GetCenter();
			else if(game_gui->inventory->mode == I_LOOT_CONTAINER)
				zoom_pos = pc->action_usable->GetCenter();
			else if(game_gui->craft->visible)
				zoom_pos = pc->unit->usable->GetCenter();
			else
			{
				if(pc->action_unit->IsAlive())
					zoom_pos = pc->action_unit->GetHeadSoundPos();
				else
					zoom_pos = pc->action_unit->GetLootCenter();
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
			res_mgr->LoadMeshMetadata(w.mesh);
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
			res_mgr->LoadMeshMetadata(s.mesh);
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
		if(ud.item_script)
			ud.item_script->Test(str, errors);

		// attacks
		if(ud.frames)
		{
			if(ud.frames->extra)
			{
				if(InRange(ud.frames->attacks, 1, NAMES::max_attacks))
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
			res_mgr->Load(&mesh);

			for(uint i = 0; i < NAMES::n_ani_base; ++i)
			{
				if(!mesh.GetAnimation(NAMES::ani_base[i]))
				{
					str += Format("\tMissing animation '%s'.\n", NAMES::ani_base[i]);
					++errors;
				}
			}

			if(!IsSet(ud.flags, F_SLOW))
			{
				if(!mesh.GetAnimation(NAMES::ani_run))
				{
					str += Format("\tMissing animation '%s'.\n", NAMES::ani_run);
					++errors;
				}
			}

			if(!IsSet(ud.flags, F_DONT_SUFFER))
			{
				if(!mesh.GetAnimation(NAMES::ani_hurt))
				{
					str += Format("\tMissing animation '%s'.\n", NAMES::ani_hurt);
					++errors;
				}
			}

			if(IsSet(ud.flags, F_HUMAN) || IsSet(ud.flags, F_HUMANOID))
			{
				for(uint i = 0; i < NAMES::n_points; ++i)
				{
					if(!mesh.GetPoint(NAMES::points[i]))
					{
						str += Format("\tMissing attachment point '%s'.\n", NAMES::points[i]);
						++errors;
					}
				}

				for(uint i = 0; i < NAMES::n_ani_humanoid; ++i)
				{
					if(!mesh.GetAnimation(NAMES::ani_humanoid[i]))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_humanoid[i]);
						++errors;
					}
				}
			}

			// attack animations
			if(ud.frames)
			{
				if(ud.frames->attacks > NAMES::max_attacks)
				{
					str += Format("\tToo many attacks (%d)!\n", ud.frames->attacks);
					++errors;
				}
				for(int i = 0; i < min(ud.frames->attacks, NAMES::max_attacks); ++i)
				{
					if(!mesh.GetAnimation(NAMES::ani_attacks[i]))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_attacks[i]);
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
			if(ud.abilities && !mesh.GetPoint(NAMES::point_cast))
			{
				str += Format("\tMissing attachment point '%s'.\n", NAMES::point_cast);
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

	game_level->entering = true;
	game_level->event_handler = nullptr;
	game_level->UpdateDungeonMinimap(false);

	if(!quest_mgr->quest_tutorial->in_tutorial && quest_mgr->quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone
		&& quest_mgr->quest_crazies->crazies_state < Quest_Crazies::State::End)
		quest_mgr->quest_crazies->CheckStone();

	if(Net::IsOnline() && net->active_players > 1)
	{
		int level = game_level->dungeon_level;
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
		if(game_level->dungeon_level == 0)
		{
			if(quest_mgr->quest_tutorial->in_tutorial)
			{
				quest_mgr->quest_tutorial->OnEvent(Quest_Tutorial::Exit);
				fallback_type = FALLBACK::CLIENT;
				fallback_t = 0.f;
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

			MultiInsideLocation* inside = static_cast<MultiInsideLocation*>(game_level->location);
			LeaveLevel();
			--game_level->dungeon_level;
			LocationGenerator* loc_gen = loc_gen_factory->Get(inside);
			game_level->enter_from = ENTER_FROM_NEXT_LEVEL;
			EnterLevel(loc_gen);
		}
	}
	else
	{
		MultiInsideLocation* inside = (MultiInsideLocation*)game_level->location;

		int steps = 3; // txLevelDown, txGeneratingMinimap, txLoadingComplete
		if(game_level->dungeon_level + 1 >= inside->generated)
			steps += 3; // txGeneratingMap, txGeneratingObjects, txGeneratingUnits
		else
			++steps; // txRegeneratingLevel

		LoadingStart(steps);
		LoadingStep(txLevelDown);

		// lower level
		LeaveLevel();
		++game_level->dungeon_level;

		LocationGenerator* loc_gen = loc_gen_factory->Get(inside);

		// is this first visit?
		if(game_level->dungeon_level >= inside->generated)
		{
			if(next_seed != 0)
			{
				Srand(next_seed);
				next_seed = 0;
			}
			else if(force_seed != 0 && force_seed_all)
				Srand(force_seed);

			inside->generated = game_level->dungeon_level + 1;
			inside->infos[game_level->dungeon_level].seed = RandVal();

			Info("Generating location '%s', seed %u.", game_level->location->name.c_str(), RandVal());
			Info("Generating dungeon, level %d, target %d.", game_level->dungeon_level + 1, inside->target);

			LoadingStep(txGeneratingMap);

			loc_gen->first = true;
			loc_gen->Generate();
		}

		game_level->enter_from = ENTER_FROM_PREV_LEVEL;
		EnterLevel(loc_gen);
	}

	game_level->location->last_visit = world->GetWorldtime();
	game_level->CheckIfLocationCleared();
	bool loaded_resources = game_level->location->RequireLoadingResources(nullptr);
	LoadResources(txLoadingComplete, false);

	SetMusic();

	if(Net::IsOnline() && net->active_players > 1)
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		prepared_stream.Reset();
		BitStreamWriter f(prepared_stream);
		net->WriteLevelData(f, loaded_resources);
		Info("Generated location packet: %d.", prepared_stream.GetNumberOfBytesUsed());
		game_gui->info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color_next;
		game_state = GS_LEVEL;
		game_gui->load_screen->visible = false;
		game_gui->main_menu->visible = false;
		game_gui->level_gui->visible = true;
	}

	Info("Randomness integrity: %d", RandVal());
	game_level->entering = false;
}

void Game::ExitToMap()
{
	game_gui->CloseAllPanels();

	clear_color = Color::Black;
	game_state = GS_WORLDMAP;
	if(game_level->is_open)
		LeaveLocation();

	net->ClearFastTravel();
	world->ExitToMap();
	SetMusic(MusicType::Travel);

	if(Net::IsServer())
		Net::PushChange(NetChange::EXIT_TO_MAP);

	game_gui->world_map->Show();
	game_gui->level_gui->visible = false;
}

void Game::PlayAttachedSound(Unit& unit, Sound* sound, float distance)
{
	assert(sound);

	if(!sound_mgr->CanPlaySound())
		return;

	Vec3 pos = unit.GetHeadSoundPos();
	FMOD::Channel* channel = sound_mgr->CreateChannel(sound, pos, distance);
	AttachedSound& s = Add1(attached_sounds);
	s.channel = channel;
	s.unit = &unit;
}

void Game::StopAllSounds()
{
	sound_mgr->StopSounds();
	attached_sounds.clear();
}

void Game::UpdateAttachedSounds(float dt)
{
	LoopAndRemove(attached_sounds, [](AttachedSound& sound)
	{
		Unit* unit = sound.unit;
		if(unit)
		{
			if(!sound_mgr->UpdateChannelPosition(sound.channel, unit->GetHeadSoundPos()))
				return false;
		}
		else
		{
			if(!sound_mgr->IsPlaying(sound.channel))
				return true;
		}
		return false;
	});
}

// clear game vars on new game or load
void Game::ClearGameVars(bool new_game)
{
	game_gui->inventory->mode = I_NONE;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	death_screen = 0;
	end_of_game = false;
	cutscene = false;
	game_gui->minimap->city = nullptr;
	team->Clear(new_game);
	draw_flags = 0xFFFFFFFF;
	game_gui->level_gui->Reset();
	game_gui->journal->Reset();
	arena->Reset();
	game_gui->level_gui->visible = false;
	game_gui->inventory->lock = nullptr;
	game_level->camera.Reset(new_game);
	pc->data.Reset();
	script_mgr->Reset();
	game_level->Reset();
	aiMgr->Reset();
	pathfinding->SetTarget(nullptr);
	game_gui->world_map->Clear();
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
	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;

	if(new_game)
	{
		devmode = default_devmode;
		scene_mgr->use_lighting = true;
		scene_mgr->use_fog = true;
		draw_particle_sphere = false;
		draw_unit_radius = false;
		draw_hitbox = false;
		noai = false;
		draw_phy = false;
		draw_col = false;
		game_speed = 1.f;
		game_level->dungeon_level = 0;
		quest_mgr->Reset();
		world->OnNewGame();
		game_stats->Reset();
		dont_wander = false;
		pc->data.picking_item_state = 0;
		game_level->is_open = false;
		game_gui->level_gui->PositionPanels();
		game_gui->Clear(true, false);
		if(!net->mp_quickload)
			game_gui->mp_box->visible = Net::IsOnline();
		game_level->light_angle = Random(PI * 2);
		pc->data.rot_buf = 0.f;
		start_version = VERSION;

		if(IsDebug())
		{
			noai = true;
			dont_wander = true;
		}
	}
}

// called before starting new game/loading/at exit
void Game::ClearGame()
{
	Info("Clearing game.");

	EntitySystem::clear = true;
	draw_batch.Clear();
	script_mgr->StopAllScripts();

	LeaveLocation(true, false);

	// delete units on world map
	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP || (net->mp_load && prev_game_state == GS_LOAD)) && !game_level->is_open
		&& Net::IsLocal() && !net->was_client)
	{
		for(Unit& unit : team->members)
		{
			if(unit.interp)
				EntityInterpolator::Pool.Free(unit.interp);

			delete unit.ai;
			delete &unit;
		}
		team->members.clear();
		prev_game_state = GS_LOAD;
	}
	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP || prev_game_state == GS_LOAD) && !game_level->is_open && (Net::IsClient() || net->was_client))
	{
		delete pc;
		pc = nullptr;
	}

	if(!net->net_strs.empty())
		StringPool.Free(net->net_strs);

	game_level->is_open = false;
	game_level->city_ctx = nullptr;
	quest_mgr->Clear();
	world->Reset();
	game_gui->Clear(true, false);
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

void Game::ApplyLocationTextureOverride(TexOverride& floor, TexOverride& wall, TexOverride& ceil, LocationTexturePack& tex)
{
	ApplyLocationTextureOverride(floor, tex.floor, game_res->tFloorBase);
	ApplyLocationTextureOverride(wall, tex.wall, game_res->tWallBase);
	ApplyLocationTextureOverride(ceil, tex.ceil, game_res->tCeilBase);
}

void Game::ApplyLocationTextureOverride(TexOverride& tex_o, LocationTexturePack::Entry& e, TexOverride& tex_o_def)
{
	if(e.tex)
	{
		tex_o.diffuse = e.tex;
		tex_o.normal = e.tex_normal;
		tex_o.specular = e.tex_specular;
	}
	else
		tex_o = tex_o_def;

	res_mgr->Load(tex_o.diffuse);
	if(tex_o.normal)
		res_mgr->Load(tex_o.normal);
	if(tex_o.specular)
		res_mgr->Load(tex_o.specular);
}

void Game::SetDungeonParamsAndTextures(BaseLocation& base)
{
	// scene parameters
	game_level->camera.zfar = base.draw_range;
	game_level->scene->fog_range = base.fog_range;
	game_level->scene->fog_color = base.fog_color;
	game_level->scene->ambient_color = base.ambient_color;
	game_level->scene->use_light_dir = false;
	clear_color_next = game_level->scene->fog_color;

	// first dungeon textures
	ApplyLocationTextureOverride(game_res->tFloor[0], game_res->tWall[0], game_res->tCeil[0], base.tex);

	// second dungeon textures
	if(base.tex2 != -1)
	{
		BaseLocation& base2 = g_base_locations[base.tex2];
		ApplyLocationTextureOverride(game_res->tFloor[1], game_res->tWall[1], game_res->tCeil[1], base2.tex);
	}
	else
	{
		game_res->tFloor[1] = game_res->tFloor[0];
		game_res->tCeil[1] = game_res->tCeil[0];
		game_res->tWall[1] = game_res->tWall[0];
	}
}

void Game::SetDungeonParamsToMeshes()
{
	// doors/stairs/traps textures
	ApplyTextureOverrideToSubmesh(game_res->aStairsDown->subs[0], game_res->tFloor[0]);
	ApplyTextureOverrideToSubmesh(game_res->aStairsDown->subs[2], game_res->tWall[0]);
	ApplyTextureOverrideToSubmesh(game_res->aStairsDown2->subs[0], game_res->tFloor[0]);
	ApplyTextureOverrideToSubmesh(game_res->aStairsDown2->subs[2], game_res->tWall[0]);
	ApplyTextureOverrideToSubmesh(game_res->aStairsUp->subs[0], game_res->tFloor[0]);
	ApplyTextureOverrideToSubmesh(game_res->aStairsUp->subs[2], game_res->tWall[0]);
	ApplyTextureOverrideToSubmesh(game_res->aDoorWall->subs[0], game_res->tWall[0]);
	ApplyDungeonLightToMesh(*game_res->aStairsDown);
	ApplyDungeonLightToMesh(*game_res->aStairsDown2);
	ApplyDungeonLightToMesh(*game_res->aStairsUp);
	ApplyDungeonLightToMesh(*game_res->aDoorWall);
	ApplyDungeonLightToMesh(*game_res->aDoorWall2);

	// apply texture/lighting to trap to make it same texture as dungeon
	if(BaseTrap::traps[TRAP_ARROW].mesh->state == ResourceState::Loaded)
	{
		ApplyTextureOverrideToSubmesh(BaseTrap::traps[TRAP_ARROW].mesh->subs[0], game_res->tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_ARROW].mesh);
	}
	if(BaseTrap::traps[TRAP_POISON].mesh->state == ResourceState::Loaded)
	{
		ApplyTextureOverrideToSubmesh(BaseTrap::traps[TRAP_POISON].mesh->subs[0], game_res->tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_POISON].mesh);
	}

	// second texture
	ApplyTextureOverrideToSubmesh(game_res->aDoorWall2->subs[0], game_res->tWall[1]);
}

void Game::EnterLevel(LocationGenerator* loc_gen)
{
	if(!loc_gen->first)
		Info("Entering location '%s' level %d.", game_level->location->name.c_str(), game_level->dungeon_level + 1);

	game_gui->inventory->lock = nullptr;

	loc_gen->OnEnter();

	OnEnterLevelOrLocation();
	OnEnterLevel();

	if(!loc_gen->first)
		Info("Entered level.");
}

void Game::LeaveLevel(bool clear)
{
	Info("Leaving level.");

	if(game_gui->level_gui)
		game_gui->level_gui->Reset();

	if(game_level->is_open)
	{
		game_level->boss = nullptr;
		for(LevelArea& area : game_level->ForEachArea())
		{
			LeaveLevel(area, clear);
			area.tmp->Free();
			area.tmp = nullptr;
		}
		if(game_level->city_ctx && (Net::IsClient() || net->was_client))
			DeleteElements(game_level->city_ctx->inside_buildings);
		if(Net::IsClient() && !game_level->location->outside)
		{
			InsideLocation* inside = (InsideLocation*)game_level->location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			Room::Free(lvl.rooms);
		}
		if(clear)
			pc = nullptr;
	}

	ais.clear();
	game_level->RemoveColliders();
	StopAllSounds();

	ClearQuadtree();

	game_gui->CloseAllPanels();

	game_level->camera.Reset();
	PlayerController::data.rot_buf = 0.f;
	dialog_context.dialog_mode = false;
	game_gui->inventory->mode = I_NONE;
	PlayerController::data.before_player = BP_NONE;
	if(Net::IsClient())
		game_level->is_open = false;
}

void Game::LeaveLevel(LevelArea& area, bool clear)
{
	// cleanup units
	if(Net::IsLocal() && !clear && !net->was_client)
	{
		LoopAndRemove(area.units, [&](Unit* p_unit)
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
						if(unit.live_state == Unit::DYING)
						{
							unit.live_state = Unit::DEAD;
							unit.mesh_inst->SetToEnd();
							game_level->CreateBlood(area, unit, true);
						}
						else if(Any(unit.live_state, Unit::FALLING, Unit::FALL))
							unit.Standup(false, true);

						if(unit.IsAlive())
						{
							// warp to inn if unit wanted to go there
							if(order == ORDER_GOTO_INN)
							{
								unit.OrderNext();
								if(game_level->city_ctx)
								{
									InsideBuilding* inn = game_level->city_ctx->FindInn();
									game_level->WarpToRegion(*inn, (Rand() % 5 == 0 ? inn->region2 : inn->region1), unit.GetUnitRadius(), unit.pos, 20);
									unit.visual_pos = unit.pos;
									unit.area = inn;
									inn->units.push_back(&unit);
									return true;
								}
							}

							// reset units rotation to don't stay back to shop counter
							if(IsSet(unit.data->flags, F_AI_GUARD) || IsSet(unit.data->flags2, F2_LIMITED_ROT))
								unit.rot = unit.ai->start_rot;
						}

						delete unit.mesh_inst;
						unit.mesh_inst = nullptr;
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
				unit.mesh_inst->need_update = true;
				unit.usable = nullptr;
				return true;
			}
		});

		for(Object* obj : area.objects)
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
		for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
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

		area.units.clear();
	}

	// temporary entities
	area.tmp->Clear();

	if(Net::IsLocal() && !net->was_client)
	{
		// remove chest meshes
		for(Chest* chest : area.chests)
			chest->Cleanup();

		// remove door meshes
		for(Door* door : area.doors)
			door->Cleanup();
	}
	else
	{
		// delete entities
		DeleteElements(area.objects);
		DeleteElements(area.chests);
		DeleteElements(area.doors);
		DeleteElements(area.traps);
		DeleteElements(area.usables);
		DeleteElements(area.items);
	}

	if(!clear)
	{
		// make blood splatter full size
		for(vector<Blood>::iterator it = area.bloods.begin(), end = area.bloods.end(); it != end; ++it)
			it->size = 1.f;
	}
}

void Game::PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const Vec3& hitpoint, float range, bool dmg)
{
	// sounds
	sound_mgr->PlaySound3d(game_res->GetMaterialSound(mat2, mat), hitpoint, range);
	if(mat != MAT_BODY && dmg)
		sound_mgr->PlaySound3d(game_res->GetMaterialSound(mat2, MAT_BODY), hitpoint, range);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::HIT_SOUND;
		c.pos = hitpoint;
		c.id = mat2;
		c.count = mat;

		if(mat != MAT_BODY && dmg)
		{
			NetChange& c2 = Add1(Net::changes);
			c2.type = NetChange::HIT_SOUND;
			c2.pos = hitpoint;
			c2.id = mat2;
			c2.count = MAT_BODY;
		}
	}
}

void Game::LoadingStart(int steps)
{
	game_gui->load_screen->Reset();
	loading_t.Reset();
	loading_dt = 0.f;
	loading_cap = 0.66f;
	loading_steps = steps;
	loading_index = 0;
	loading_first_step = true;
	clear_color = Color::Black;
	game_state = GS_LOAD;
	game_gui->load_screen->visible = true;
	game_gui->main_menu->visible = false;
	game_gui->level_gui->visible = false;
	res_mgr->PrepareLoadScreen(loading_cap);
}

void Game::LoadingStep(cstring text, int end)
{
	float progress;
	if(end == 0)
		progress = float(loading_index) / loading_steps * loading_cap;
	else if(end == 1)
		progress = loading_cap;
	else
		progress = 1.f;
	game_gui->load_screen->SetProgressOptional(progress, text);

	if(end != 2)
	{
		++loading_index;
		if(end == 1)
			assert(loading_index == loading_steps);
	}
	if(end != 1)
	{
		loading_dt += loading_t.Tick();
		if(loading_dt >= 1.f / 30 || end == 2 || loading_first_step)
		{
			loading_dt = 0.f;
			engine->DoPseudotick();
			loading_t.Tick();
			loading_first_step = false;
		}
	}
}

// Preload resources and start load screen if required
void Game::LoadResources(cstring text, bool worldmap, bool postLoad)
{
	LoadingStep(nullptr, 1);

	PreloadResources(worldmap);

	// check if there is anything to load
	if(res_mgr->HaveTasks())
	{
		Info("Loading new resources (%d).", res_mgr->GetLoadTasksCount());
		loading_resources = true;
		loading_dt = 0;
		loading_t.Reset();
		try
		{
			res_mgr->StartLoadScreen(txLoadingResources);
		}
		catch(...)
		{
			loading_resources = false;
			throw;
		}

		// apply mesh instance for newly loaded meshes
		for(auto& unit_mesh : units_mesh_load)
		{
			Unit::CREATE_MESH mode;
			if(unit_mesh.second)
				mode = Unit::CREATE_MESH::ON_WORLDMAP;
			else if(net->mp_load && Net::IsClient())
				mode = Unit::CREATE_MESH::AFTER_PRELOAD;
			else
				mode = Unit::CREATE_MESH::NORMAL;
			unit_mesh.first->CreateMesh(mode);
		}
		units_mesh_load.clear();
	}
	else
	{
		Info("Nothing new to load.");
		res_mgr->CancelLoadScreen();
	}

	if(postLoad && game_level->location)
	{
		// spawn blood for units that are dead and their mesh just loaded
		game_level->SpawnBlood();

		// create mesh instance for objects
		game_level->CreateObjectsMeshInstance();

		// finished
		if((Net::IsLocal() || !net->mp_load_worldmap) && !game_level->location->outside)
			SetDungeonParamsToMeshes();
	}

	LoadingStep(text, 2);
}

// When there is something new to load, add task to load it when entering location etc
void Game::PreloadResources(bool worldmap)
{
	if(Net::IsLocal())
		items_load.clear();

	if(!worldmap)
	{
		for(LevelArea& area : game_level->ForEachArea())
		{
			// load units - units respawn so need to check everytime...
			for(Unit* unit : area.units)
				PreloadUnit(unit);

			// some traps respawn
			for(Trap* trap : area.traps)
			{
				BaseTrap& base = *trap->base;
				if(base.state != ResourceState::NotLoaded)
					continue;

				if(base.mesh)
					res_mgr->Load(base.mesh);
				if(base.mesh2)
					res_mgr->Load(base.mesh2);
				if(base.sound)
					res_mgr->Load(base.sound);
				if(base.sound2)
					res_mgr->Load(base.sound2);
				if(base.sound3)
					res_mgr->Load(base.sound3);

				base.state = ResourceState::Loaded;
			}

			// preload items, this info is sent by server so no need to redo this by clients (and it will be less complete)
			if(Net::IsLocal())
			{
				for(GroundItem* ground_item : area.items)
				{
					assert(ground_item->item);
					items_load.insert(ground_item->item);
				}
				for(Chest* chest : area.chests)
					PreloadItems(chest->items);
				for(Usable* usable : area.usables)
				{
					if(usable->container)
						PreloadItems(usable->container->items);
				}
			}
		}

		bool new_value = true;
		if(game_level->location->RequireLoadingResources(&new_value) == false)
		{
			// load music
			game_res->LoadMusic(game_level->GetLocationMusic(), false);

			for(LevelArea& area : game_level->ForEachArea())
			{
				// load objects
				for(Object* obj : area.objects)
					res_mgr->Load(obj->mesh);
				for(Chest* chest : area.chests)
					res_mgr->Load(chest->base->mesh);

				// load usables
				for(Usable* use : area.usables)
				{
					BaseUsable* base = use->base;
					if(base->state == ResourceState::NotLoaded)
					{
						if(base->variants)
						{
							for(Mesh* mesh : base->variants->meshes)
								res_mgr->Load(mesh);
						}
						else
							res_mgr->Load(base->mesh);
						if(base->sound)
							res_mgr->Load(base->sound);
						base->state = ResourceState::Loaded;
					}
				}
			}

			// load buildings
			if(game_level->city_ctx)
			{
				for(CityBuilding& city_building : game_level->city_ctx->buildings)
				{
					Building& building = *city_building.building;
					if(building.state == ResourceState::NotLoaded)
					{
						if(building.mesh)
							res_mgr->Load(building.mesh);
						if(building.inside_mesh)
							res_mgr->Load(building.inside_mesh);
						building.state = ResourceState::Loaded;
					}
				}
			}
		}
	}

	for(const Item* item : items_load)
		game_res->PreloadItem(item);
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
				items_load.insert(item);
		}
		PreloadItems(unit->items);
		if(unit->stock)
			PreloadItems(unit->stock->items);
	}

	if(data.state == ResourceState::Loaded)
		return;

	if(data.mesh)
		res_mgr->Load(data.mesh);

	if(!sound_mgr->IsDisabled())
	{
		for(int i = 0; i < SOUND_MAX; ++i)
		{
			for(SoundPtr sound : data.sounds->sounds[i])
				res_mgr->Load(sound);
		}
	}

	if(data.tex)
	{
		for(TexOverride& tex_o : data.tex->textures)
		{
			if(tex_o.diffuse)
				res_mgr->Load(tex_o.diffuse);
		}
	}

	data.state = ResourceState::Loaded;
}

void Game::PreloadItems(vector<ItemSlot>& items)
{
	for(auto& slot : items)
	{
		assert(slot.item);
		items_load.insert(slot.item);
	}
}

void Game::VerifyResources()
{
	for(LevelArea& area : game_level->ForEachArea())
	{
		for(GroundItem* item : area.items)
			VerifyItemResources(item->item);
		for(Object* obj : area.objects)
			assert(obj->mesh->state == ResourceState::Loaded);
		for(Unit* unit : area.units)
			VerifyUnitResources(unit);
		for(Usable* u : area.usables)
		{
			BaseUsable* base = u->base;
			assert(base->state == ResourceState::Loaded);
			if(base->sound)
				assert(base->sound->IsLoaded());
		}
		for(Trap* trap : area.traps)
		{
			assert(trap->base->state == ResourceState::Loaded);
			if(trap->base->mesh)
				assert(trap->base->mesh->IsLoaded());
			if(trap->base->mesh2)
				assert(trap->base->mesh2->IsLoaded());
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
			for(SoundPtr sound : unit->data->sounds->sounds[i])
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

	if(game_level->is_open)
	{
		RemoveElement(unit->area->units, unit);
		game_gui->level_gui->RemoveUnitView(unit);
		if(pc->data.before_player == BP_UNIT && pc->data.before_player_ptr.unit == unit)
			pc->data.before_player = BP_NONE;
		if(unit == pc->data.selected_unit)
			pc->data.selected_unit = nullptr;
		if(Net::IsClient())
		{
			if(pc->action == PlayerAction::LootUnit && pc->action_unit == unit)
				pc->unit->BreakAction();
		}
		else
		{
			for(PlayerInfo& player : net->players)
			{
				PlayerController* pc = player.pc;
				if(pc->action == PlayerAction::LootUnit && pc->action_unit == unit)
					pc->action_unit = nullptr;
			}
		}

		if(unit->player && Net::IsLocal())
		{
			switch(unit->player->action)
			{
			case PlayerAction::LootChest:
				unit->player->action_chest->OpenClose(nullptr);
				break;
			case PlayerAction::LootUnit:
				unit->player->action_unit->busy = Unit::Busy_No;
				break;
			case PlayerAction::Trade:
			case PlayerAction::Talk:
			case PlayerAction::GiveItems:
			case PlayerAction::ShareItems:
				unit->player->action_unit->busy = Unit::Busy_No;
				unit->player->action_unit->look_target = nullptr;
				break;
			case PlayerAction::LootContainer:
				unit->UseUsable(nullptr);
				break;
			}
		}

		if(quest_mgr->quest_contest->state >= Quest_Contest::CONTEST_STARTING)
			RemoveElementTry(quest_mgr->quest_contest->units, unit);
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
		phy_world->removeCollisionObject(unit->cobj);
		delete unit->cobj;
	}

	if(--unit->refs == 0)
		delete unit;
}

void Game::OnCloseInventory()
{
	if(game_gui->inventory->mode == I_TRADE)
	{
		if(Net::IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(game_gui->inventory->mode == I_SHARE || game_gui->inventory->mode == I_GIVE)
	{
		if(Net::IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(game_gui->inventory->mode == I_LOOT_CHEST && Net::IsLocal())
		pc->action_chest->OpenClose(nullptr);
	else if(game_gui->inventory->mode == I_LOOT_CONTAINER)
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

	if(Net::IsOnline() && (game_gui->inventory->mode == I_LOOT_BODY || game_gui->inventory->mode == I_LOOT_CHEST))
	{
		if(Net::IsClient())
			Net::PushChange(NetChange::STOP_TRADE);
		else if(game_gui->inventory->mode == I_LOOT_BODY)
			pc->action_unit->busy = Unit::Busy_No;
	}

	if(Any(pc->next_action, NA_PUT, NA_GIVE, NA_SELL))
		pc->next_action = NA_NONE;

	pc->action = PlayerAction::None;
	game_gui->inventory->mode = I_NONE;
}

void Game::CloseInventory()
{
	OnCloseInventory();
	game_gui->inventory->mode = I_NONE;
	if(game_gui->level_gui)
	{
		game_gui->inventory->inv_mine->Hide();
		game_gui->inventory->gp_trade->Hide();
	}
}

bool Game::CanShowEndScreen()
{
	if(Net::IsLocal())
		return !quest_mgr->unique_completed_show && quest_mgr->unique_quests_completed == quest_mgr->unique_quests && game_level->city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
	else
		return quest_mgr->unique_completed_show && game_level->city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
}

void Game::UpdateGameNet(float dt)
{
	if(game_gui->info_box->visible)
		return;

	if(Net::IsServer())
		net->UpdateServer(dt);
	else
		net->UpdateClient(dt);
}

DialogContext* Game::FindDialogContext(Unit* talker)
{
	assert(talker);
	if(dialog_context.dialog_mode && dialog_context.talker == talker)
		return &dialog_context;
	if(Net::IsOnline())
	{
		for(PlayerInfo& info : net->players)
		{
			DialogContext* ctx = info.pc->dialog_ctx;
			if(ctx->dialog_mode && ctx->talker == talker)
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
	if(quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::ToldAboutCamp && quest_mgr->quest_orcs2->targetLoc == game_level->location
		&& quest_mgr->quest_orcs2->talked == Quest_Orcs2::Talked::No)
	{
		quest_mgr->quest_orcs2->talked = Quest_Orcs2::Talked::AboutCamp;
		talker = quest_mgr->quest_orcs2->orc;
		text = txOrcCamp;
	}

	if(!talker)
	{
		TeamInfo info;
		team->GetTeamInfo(info);

		if(info.sane_heroes > 0)
		{
			bool always_use = false;
			switch(game_level->location->type)
			{
			case L_CITY:
				if(LocationHelper::IsCity(game_level->location))
					text = RandomString(txAiCity);
				else
					text = RandomString(txAiVillage);
				break;
			case L_OUTSIDE:
				switch(game_level->location->target)
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
				if(game_level->location->state != LS_CLEARED && !game_level->location->group->IsEmpty())
				{
					if(!game_level->location->group->name2.empty())
					{
						always_use = true;
						text = Format(txAiCampFull, game_level->location->group->name2.c_str());
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
	Quest_Evil* quest_evil = quest_mgr->quest_evil;
	if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss)
	{
		if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals)
		{
			int d = quest_evil->GetLocId(game_level->location);
			if(d != -1)
			{
				Quest_Evil::Loc& loc = quest_evil->loc[d];

				if(game_level->dungeon_level == game_level->location->GetLastLevel())
				{
					if(loc.state < Quest_Evil::Loc::TalkedAfterEnterLevel)
					{
						talker = quest_evil->cleric;
						text = txPortalCloseLevel;
						loc.state = Quest_Evil::Loc::TalkedAfterEnterLevel;
					}
				}
				else if(game_level->dungeon_level == 0 && loc.state == Quest_Evil::Loc::None)
				{
					talker = quest_evil->cleric;
					text = txPortalClose;
					loc.state = Quest_Evil::Loc::TalkedAfterEnterLocation;
				}
			}
		}
		else if(game_level->location == quest_evil->targetLoc && !quest_evil->told_about_boss)
		{
			quest_evil->told_about_boss = true;
			talker = quest_evil->cleric;
			text = txXarDanger;
		}
	}

	// orc talking after entering level
	Quest_Orcs2* quest_orcs2 = quest_mgr->quest_orcs2;
	if(!talker && (quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs || quest_orcs2->orcs_state == Quest_Orcs2::State::GeneratedOrcs) && game_level->location == quest_orcs2->targetLoc)
	{
		if(game_level->dungeon_level == 0)
		{
			if(quest_orcs2->talked < Quest_Orcs2::Talked::AboutBase)
			{
				quest_orcs2->talked = Quest_Orcs2::Talked::AboutBase;
				talker = quest_orcs2->orc;
				text = txGorushDanger;
			}
		}
		else if(game_level->dungeon_level == game_level->location->GetLastLevel())
		{
			if(quest_orcs2->talked < Quest_Orcs2::Talked::AboutBoss)
			{
				quest_orcs2->talked = Quest_Orcs2::Talked::AboutBoss;
				talker = quest_orcs2->orc;
				text = txGorushCombat;
			}
		}
	}

	// old mage talking after entering location
	Quest_Mages2* quest_mages2 = quest_mgr->quest_mages2;
	if(!talker && (quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined || quest_mages2->mages_state == Quest_Mages2::State::MageRecruited))
	{
		if(quest_mages2->targetLoc == game_level->location)
		{
			if(quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined)
			{
				if(game_level->dungeon_level == 0 && quest_mages2->talked == Quest_Mages2::Talked::No)
				{
					quest_mages2->talked = Quest_Mages2::Talked::AboutHisTower;
					text = txMageHere;
				}
			}
			else
			{
				if(game_level->dungeon_level == 0)
				{
					if(quest_mages2->talked < Quest_Mages2::Talked::AfterEnter)
					{
						quest_mages2->talked = Quest_Mages2::Talked::AfterEnter;
						text = Format(txMageEnter, quest_mages2->evil_mage_name.c_str());
					}
				}
				else if(game_level->dungeon_level == game_level->location->GetLastLevel() && quest_mages2->talked < Quest_Mages2::Talked::BeforeBoss)
				{
					quest_mages2->talked = Quest_Mages2::Talked::BeforeBoss;
					text = txMageFinal;
				}
			}
		}

		if(text)
			talker = team->FindTeamMember("q_magowie_stary");
	}

	// default talking about location
	if(!talker && game_level->dungeon_level == 0 && (game_level->enter_from == ENTER_FROM_OUTSIDE || game_level->enter_from >= ENTER_FROM_PORTAL))
	{
		TeamInfo info;
		team->GetTeamInfo(info);

		if(info.sane_heroes > 0)
		{
			LocalString s;

			switch(game_level->location->type)
			{
			case L_DUNGEON:
				{
					InsideLocation* inside = (InsideLocation*)game_level->location;
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
	game_gui->Clear(false, true);
	pc->data.autowalk = false;
	pc->data.selected_unit = pc->unit;
	fallback_t = -0.5f;
	fallback_type = FALLBACK::NONE;
	if(Net::IsLocal())
	{
		for(Unit& unit : team->members)
			unit.frozen = FROZEN::NO;
	}

	// events v2
	for(Event& e : game_level->location->events)
	{
		if(e.type == EVENT_ENTER)
		{
			ScriptEvent event(EVENT_ENTER);
			event.on_enter.location = game_level->location;
			e.quest->FireEvent(event);
		}
	}

	if(Net::IsClient() && game_level->boss)
		game_gui->level_gui->SetBoss(game_level->boss, true);
}
