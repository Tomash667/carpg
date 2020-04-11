#include "Pch.h"
#include "Game.h"
#include "GameStats.h"
#include "ParticleSystem.h"
#include "Terrain.h"
#include "ItemScript.h"
#include "RoomType.h"
#include "SaveState.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "Minimap.h"
#include "QuestManager.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "Version.h"
#include "LocationHelper.h"
#include "MultiInsideLocation.h"
#include "SingleInsideLocation.h"
#include "Encounter.h"
#include "LevelGui.h"
#include "Console.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "WorldMapGui.h"
#include "MpBox.h"
#include "GameMessages.h"
#include "AIController.h"
#include "Ability.h"
#include "Team.h"
#include "Ability.h"
#include "ItemContainer.h"
#include "Stock.h"
#include "UnitGroup.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "Profiler.h"
#include "Portal.h"
#include "BitStreamFunc.h"
#include "EntityInterpolator.h"
#include "World.h"
#include "Level.h"
#include "DirectX.h"
#include "Var.h"
#include "News.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "LocationGeneratorFactory.h"
#include "DungeonGenerator.h"
#include "Texture.h"
#include "Pathfinding.h"
#include "Arena.h"
#include "ResourceManager.h"
#include "ItemHelper.h"
#include "GameGui.h"
#include "FOV.h"
#include "PlayerInfo.h"
#include "CombatHelper.h"
#include "Quest_Scripted.h"
#include "Render.h"
#include "RenderTarget.h"
#include "BookPanel.h"
#include "Engine.h"
#include "PhysicCallbacks.h"
#include "SaveSlot.h"
#include "BasicShader.h"
#include "LobbyApi.h"
#include "ServerPanel.h"
#include "GameMenu.h"
#include "Language.h"
#include "CommandParser.h"
#include "Controls.h"
#include "GameResources.h"
#include "NameHelper.h"
#include "GrassShader.h"
#include "SuperShader.h"
#include "TerrainShader.h"
#include "Notifications.h"
#include "GameGui.h"
#include "CreateServerPanel.h"
#include "PickServerPanel.h"
#include "ParticleShader.h"
#include "GlowShader.h"
#include "PostfxShader.h"
#include "SkyboxShader.h"
#include "SceneManager.h"
#include "Scene.h"
#include "CraftPanel.h"
#include "DungeonMeshBuilder.h"

const float LIMIT_DT = 0.3f;
Game* global::game;
CustomCollisionWorld* global::phy_world;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;
void HumanPredraw(void* ptr, Matrix* mat, int n);

const float ALERT_RANGE = 20.f;
const float ALERT_SPAWN_RANGE = 25.f;
const float PICKUP_RANGE = 2.f;
const float TRAP_ARROW_SPEED = 45.f;
const float ARROW_TIMER = 5.f;
const float MIN_H = 1.5f; // hardcoded in GetPhysicsPos
const float TRAIN_KILL_RATIO = 0.1f;
const int NN = 64;
const float HIT_SOUND_DIST = 1.5f;
const float ARROW_HIT_SOUND_DIST = 1.5f;
const float SHOOT_SOUND_DIST = 1.f;
const float SPAWN_SOUND_DIST = 1.5f;
const float MAGIC_SCROLL_SOUND_DIST = 1.5f;

//=================================================================================================
Game::Game() : quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0), draw_particle_sphere(false), draw_unit_radius(false),
draw_hitbox(false), noai(false), testing(false), game_speed(1.f), devmode(false), force_seed(0), next_seed(0), force_seed_all(false), dont_wander(false),
check_updates(true), skip_tutorial(false), portal_anim(0), music_type(MusicType::None), end_of_game(false), prepared_stream(64 * 1024), paused(false),
draw_flags(0xFFFFFFFF), prev_game_state(GS_LOAD), rt_save(nullptr), rt_item_rot(nullptr), use_postfx(true), mp_timeout(10.f),
profiler_mode(ProfilerMode::Disabled), screenshot_format(ImageFormat::JPG), game_state(GS_LOAD), default_devmode(false), default_player_devmode(false),
quickstart_slot(SaveSlot::MAX_SLOTS), clear_color(Color::Black), in_load(false), tMinimap(nullptr)
{
#ifdef _DEBUG
	default_devmode = true;
	default_player_devmode = true;
#endif
	devmode = default_devmode;

	dialog_context.is_local = true;

	LocalString s;
	GetTitle(s);
	engine->SetTitle(s.c_str());

	uv_mod = Terrain::DEFAULT_UV_MOD;

	SetupConfigVars();

	arena = new Arena;
	cmdp = new CommandParser;
	dun_mesh_builder = new DungeonMeshBuilder;
	game_gui = new GameGui;
	game_level = new Level;
	game_res = new GameResources;
	game_stats = new GameStats;
	loc_gen_factory = new LocationGeneratorFactory;
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

	InitScene();
	cmdp->AddCommands();
	settings.ResetGameKeys();
	settings.LoadGameKeys(cfg);
	load_errors += BaseLocation::SetRoomPointers();

	// shaders
	render->RegisterShader(basic_shader = new BasicShader);
	render->RegisterShader(grass_shader = new GrassShader);
	render->RegisterShader(glow_shader = new GlowShader);
	render->RegisterShader(particle_shader = new ParticleShader);
	render->RegisterShader(postfx_shader = new PostfxShader);
	render->RegisterShader(skybox_shader = new SkyboxShader);
	render->RegisterShader(terrain_shader = new TerrainShader);

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

	// create save folders
	io::CreateDirectory("saves");
	io::CreateDirectory("saves/single");
	io::CreateDirectory("saves/multi");

	ItemScript::Init();

	// test & validate game data (in debug always check some things)
	if(testing)
		ValidateGameData(true);
#ifdef _DEBUG
	else
		ValidateGameData(false);
#endif

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
#ifdef _DEBUG
		game_gui->notifications->Add(text, img, 5.f);
#else
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
#endif
	}

	// save config
	cfg.Add("adapter", render->GetAdapter());
	cfg.Add("resolution", engine->GetWindowSize());
	cfg.Add("refresh", render->GetRefreshRate());
	SaveCfg();

	// end load screen, show menu
	clear_color = Color::Black;
	game_state = GS_MAIN_MENU;
	game_gui->load_screen->visible = false;
	game_gui->main_menu->visible = true;
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

	delete arena;
	delete cmdp;
	delete dun_mesh_builder;
	delete game_gui;
	delete game_level;
	delete game_res;
	delete game_stats;
	delete loc_gen_factory;
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

	DrawGame(nullptr);
	render->Present();

	Profiler::g_profiler.End();
}

//=================================================================================================
void Game::Draw()
{
	PROFILER_BLOCK("Draw");

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
	DrawScene(outside);
}

//=================================================================================================
void Game::DrawGame(RenderTarget* target)
{
	//IDirect3DDevice9* device = render->GetDevice();

	//vector<PostEffect> post_effects;
	//GetPostEffects(post_effects);

	//if(post_effects.empty())
	//{
	//	if(target)
	//		render->SetTarget(target);

		render->Clear(clear_color);

		if(game_state == GS_LEVEL)
		{
			// draw level
			Draw();

			// draw glow
			/*if(!draw_batch.glow_nodes.empty())
			{
				V(device->EndScene());
				DrawGlowingNodes(draw_batch.glow_nodes, false);
				V(device->BeginScene());
			}*/
			FIXME;
		}

		// draw gui
		game_gui->Draw(game_level->camera.mat_view_proj, IsSet(draw_flags, DF_GUI), IsSet(draw_flags, DF_MENU));

		//if(target)
		//	render->SetTarget(nullptr);
	/*}
	else
	{
		ID3DXEffect* effect = postfx_shader->effect;

		// render scene to texture
		SURFACE sPost;
		if(!render->IsMultisamplingEnabled())
			V(postfx_shader->tex[2]->GetSurfaceLevel(0, &sPost));
		else
			sPost = postfx_shader->surf[2];

		V(device->SetRenderTarget(0, sPost));
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));

		if(game_state == GS_LEVEL)
		{
			V(device->BeginScene());
			Draw();
			V(device->EndScene());
			if(!draw_batch.glow_nodes.empty())
				DrawGlowingNodes(draw_batch.glow_nodes, true);
		}

		PROFILER_BLOCK("PostEffects");

		TEX t;
		if(!render->IsMultisamplingEnabled())
		{
			sPost->Release();
			t = postfx_shader->tex[2];
		}
		else
		{
			SURFACE surf2;
			V(postfx_shader->tex[0]->GetSurfaceLevel(0, &surf2));
			V(device->StretchRect(sPost, nullptr, surf2, nullptr, D3DTEXF_NONE));
			surf2->Release();
			t = postfx_shader->tex[0];
		}

		// post effects
		V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_TEX)));
		V(device->SetStreamSource(0, postfx_shader->vbFullscreen, 0, sizeof(VTex)));
		render->SetAlphaBlend(false);
		render->SetNoCulling(false);
		render->SetNoZWrite(true);

		uint passes;
		int index_surf = 1;
		for(vector<PostEffect>::iterator it = post_effects.begin(), end = post_effects.end(); it != end; ++it)
		{
			SURFACE surf;
			if(it + 1 == end)
			{
				// last pass
				if(target)
					surf = target->GetSurface();
				else
					V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf));
			}
			else
			{
				// using next pass
				if(!render->IsMultisamplingEnabled())
					V(postfx_shader->tex[index_surf]->GetSurfaceLevel(0, &surf));
				else
					surf = postfx_shader->surf[index_surf];
			}

			V(device->SetRenderTarget(0, surf));
			V(device->BeginScene());

			V(effect->SetTechnique(it->tech));
			V(effect->SetTexture(postfx_shader->hTex, t));
			V(effect->SetFloat(postfx_shader->hPower, it->power));
			V(effect->SetVector(postfx_shader->hSkill, (D3DXVECTOR4*)&it->skill));

			V(effect->Begin(&passes, 0));
			V(effect->BeginPass(0));
			V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
			V(effect->EndPass());
			V(effect->End());

			if(it + 1 == end)
				game_gui->Draw(game_level->camera.mat_view_proj, IsSet(draw_flags, DF_GUI), IsSet(draw_flags, DF_MENU));

			V(device->EndScene());

			if(it + 1 == end)
			{
				if(!target)
					surf->Release();
			}
			else if(!render->IsMultisamplingEnabled())
			{
				surf->Release();
				t = postfx_shader->tex[index_surf];
			}
			else
			{
				SURFACE surf2;
				V(postfx_shader->tex[0]->GetSurfaceLevel(0, &surf2));
				V(device->StretchRect(surf, nullptr, surf2, nullptr, D3DTEXF_NONE));
				surf2->Release();
				t = postfx_shader->tex[0];
			}

			index_surf = (index_surf + 1) % 3;
		}
	}*/
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
		if(death_fade >= 1.f && GKey.AllowKeyboard() && input->PressedRelease(Key::Escape))
		{
			ExitToMenu();
			end_of_game = false;
		}
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
			engine->ChangeMode(!engine->IsFullscreen());

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

		if(game_state == GS_LEVEL)
		{
			if(!paused && !(gui->HavePauseDialog() && Net::IsSingleplayer()))
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
			game_level->camera.Update(dt);
		}
	}
	else if(cutscene)
		UpdateFallback(dt);

	if(Net::IsOnline() && Any(game_state, GS_LEVEL, GS_WORLDMAP))
		UpdateGameNet(dt);

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

#ifdef _DEBUG
	none = false;
	s += " - DEBUG";
#endif

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
	FIXME;
	/*if(no_gui)
	{
		int old_flags = draw_flags;
		draw_flags = (0xFFFF & ~DF_GUI);
		render->Draw(false);
		draw_flags = old_flags;
	}
	else
		render->Draw(false);

	SURFACE back_buffer;
	HRESULT hr = render->GetDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	if(FAILED(hr))
	{
		cstring msg = Format("Failed to get front buffer data to save screenshot (%d)!", hr);
		game_gui->console->AddMsg(msg);
		Error(msg);
	}
	else
	{
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

		cstring ext;
		D3DXIMAGE_FILEFORMAT format;
		switch(screenshot_format)
		{
		case ImageFormat::BMP:
			ext = "bmp";
			format = D3DXIFF_BMP;
			break;
		default:
		case ImageFormat::JPG:
			ext = "jpg";
			format = D3DXIFF_JPG;
			break;
		case ImageFormat::TGA:
			ext = "tga";
			format = D3DXIFF_TGA;
			break;
		case ImageFormat::PNG:
			ext = "png";
			format = D3DXIFF_PNG;
			break;
		}

		cstring path = Format("screenshots\\%04d%02d%02d_%02d%02d%02d_%02d.%s", lt.tm_year + 1900, lt.tm_mon + 1,
			lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, screenshot_count, ext);

		D3DXSaveSurfaceToFileA(path, format, back_buffer, nullptr, nullptr);

		cstring msg = Format("Screenshot saved to '%s'.", path);
		game_gui->console->AddMsg(msg);
		Info(msg);

		back_buffer->Release();
	}*/
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
	game_gui->main_menu->visible = true;
	units_mesh_load.clear();

	ChangeTitle();
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
	// stwórz mutex
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

	// drugi parametr tak na prawdê nie jest modyfikowany o ile nie u¿ywa siê unicode (tak jest napisane w doku)
	// z ka¿dym restartem dodaje prze³¹cznik, mam nadzieje ¿e nikt nie bêdzie restartowa³ 100 razy pod rz¹d bo mo¿e skoñczyæ siê miejsce w cmdline albo co
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
	LoadArray(txAiNoHpPot, "aiNoHpPot");
	LoadArray(txAiNoMpPot, "aiNoMpPot");
	LoadArray(txAiCity, "aiCity");
	LoadArray(txAiVillage, "aiVillage");
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
	LoadArray(txAiInsaneText, "aiInsaneText");
	LoadArray(txAiDefaultText, "aiDefaultText");
	LoadArray(txAiOutsideText, "aiOutsideText");
	LoadArray(txAiInsideText, "aiInsideText");
	LoadArray(txAiHumanText, "aiHumanText");
	LoadArray(txAiOrcText, "aiOrcText");
	LoadArray(txAiGoblinText, "aiGoblinText");
	LoadArray(txAiMageText, "aiMageText");
	LoadArray(txAiSecretText, "aiSecretText");
	LoadArray(txAiHeroDungeonText, "aiHeroDungeonText");
	LoadArray(txAiHeroCityText, "aiHeroCityText");
	LoadArray(txAiBanditText, "aiBanditText");
	LoadArray(txAiHeroOutsideText, "aiHeroOutsideText");
	LoadArray(txAiDrunkMageText, "aiDrunkMageText");
	LoadArray(txAiDrunkText, "aiDrunkText");
	LoadArray(txAiDrunkContestText, "aiDrunkContestText");
	LoadArray(txAiWildHunterText, "aiWildHunterText");

	// mapa
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

	// plotki
	LoadArray(txRumor, "rumor_");
	LoadArray(txRumorD, "rumor_d_");

	// dialogi 1
	LoadArray(txMayorQFailed, "mayorQFailed");
	LoadArray(txQuestAlreadyGiven, "questAlreadyGiven");
	LoadArray(txMayorNoQ, "mayorNoQ");
	LoadArray(txCaptainQFailed, "captainQFailed");
	LoadArray(txCaptainNoQ, "captainNoQ");
	LoadArray(txLocationDiscovered, "locationDiscovered");
	LoadArray(txAllDiscovered, "allDiscovered");
	LoadArray(txCampDiscovered, "campDiscovered");
	LoadArray(txAllCampDiscovered, "allCampDiscovered");
	LoadArray(txNoQRumors, "noQRumors");
	txNeedMoreGold = Str("needMoreGold");
	txNoNearLoc = Str("noNearLoc");
	txNearLoc = Str("nearLoc");
	LoadArray(txNearLocEmpty, "nearLocEmpty");
	txNearLocCleared = Str("nearLocCleared");
	LoadArray(txNearLocEnemy, "nearLocEnemy");
	LoadArray(txNoNews, "noNews");
	LoadArray(txAllNews, "allNews");
	txAllNearLoc = Str("allNearLoc");
	txLearningPoint = Str("learningPoint");
	txLearningPoints = Str("learningPoints");
	txNeedLearningPoints = Str("needLearningPoints");
	txTeamTooBig = Str("teamTooBig");
	txHeroJoined = Str("heroJoined");
	txCantLearnAbility = Str("cantLearnAbility");
	txSpell = Str("spell");
	txCantLearnSkill = Str("cantLearnSkill");

	// dystans / si³a
	txNear = Str("near");
	txFar = Str("far");
	txVeryFar = Str("veryFar");
	LoadArray(txELvlVeryWeak, "eLvlVeryWeak");
	LoadArray(txELvlWeak, "eLvlWeak");
	LoadArray(txELvlAverage, "eLvlAverage");
	LoadArray(txELvlQuiteStrong, "eLvlQuiteStrong");
	LoadArray(txELvlStrong, "eLvlStrong");

	// questy
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
	LoadArray(txQuest, "quest");
	txForMayor = Str("forMayor");
	txForSoltys = Str("forSoltys");

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

	// dialogi
	LoadArray(txYell, "yell");
}

//=================================================================================================
void Game::UpdateLights(vector<GameLight>& lights)
{
	for(GameLight& light : lights)
	{
		light.pos = light.start_pos + Vec3::Random(Vec3(-0.05f, -0.05f, -0.05f), Vec3(0.05f, 0.05f, 0.05f));
		light.color = Vec4((light.start_color + Vec3::Random(Vec3(-0.1f, -0.1f, -0.1f), Vec3(0.1f, 0.1f, 0.1f))).Clamped(), 1);
	}
}

//=================================================================================================
void Game::GetPostEffects(vector<PostEffect>& post_effects)
{
	FIXME;
	/*post_effects.clear();
	if(!use_postfx || game_state != GS_LEVEL || !postfx_shader->effect)
		return;

	// gray effect
	if(pc->data.grayout > 0.f)
	{
		PostEffect& e = Add1(post_effects);
		e.tech = postfx_shader->techMonochrome;
		e.power = pc->data.grayout;
	}

	// drunk effect
	float drunk = pc->unit->alcohol / pc->unit->hpmax;
	if(drunk > 0.1f)
	{
		PostEffect* e, *e2;
		post_effects.resize(post_effects.size() + 2);
		e = &*(post_effects.end() - 2);
		e2 = &*(post_effects.end() - 1);

		e->id = e2->id = 0;
		e->tech = postfx_shader->techBlurX;
		e2->tech = postfx_shader->techBlurY;
		// 0.1-0.5 - 1
		// 1 - 2
		float mod;
		if(drunk < 0.5f)
			mod = 1.f;
		else
			mod = 1.f + (drunk - 0.5f) * 2;
		e->skill = e2->skill = Vec4(1.f / engine->GetWindowSize().x * mod, 1.f / engine->GetWindowSize().y * mod, 0, 0);
		// 0.1-0
		// 1-1
		e->power = e2->power = (drunk - 0.1f) / 0.9f;
	}*/
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

		// nie odwiedzono, trzeba wygenerowaæ
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

// dru¿yna opuœci³a lokacje
void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(!game_level->is_open)
		return;

	if(Net::IsLocal() && !net->was_client)
	{
		// zawody
		if(quest_mgr->quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			quest_mgr->quest_tournament->Clean();
		// arena
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
	if(Net::IsLocal() && quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::ClearDungeon && game_level->location_index == quest_mgr->quest_orcs2->target_loc)
	{
		quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::End;
		game_level->UpdateLocation(31, 100, false);
	}

	if(game_level->city_ctx && game_state != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// opuszczanie miasta
		team->BuyTeamItems();
	}

	LeaveLevel();

	if(game_level->is_open)
	{
		if(Net::IsLocal())
		{
			// usuñ questowe postacie
			quest_mgr->RemoveQuestUnits(true);
		}

		game_level->ProcessRemoveUnits(true);

		if(game_level->location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = static_cast<OutsideLocation*>(game_level->location);
			outside->Clear();
		}
	}

	if(team->crazies_attack)
	{
		team->crazies_attack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	if(Net::IsLocal() && end_buffs)
	{
		// usuñ tymczasowe bufy
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
#ifdef _DEBUG
	game_gui->messages->AddGameMsg(str, 5.f);
#endif
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
#ifdef _DEBUG
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
	{
		assert(pc->is_local && pc == pc->player_info->pc);
	}
#endif

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

	// info o uczoñczeniu wszystkich unikalnych questów
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
			InsideLocation* inside = (InsideLocation*)game_level->location;
			InsideLocationLevel& lvl = inside->GetLevelData();

			// key [<,] - warp to stairs up or upper level
			if(input->Down(Key::Shift) && input->Pressed(Key::Comma) && inside->HaveUpStairs())
			{
				if(!input->Down(Key::Control))
				{
					if(Net::IsLocal())
					{
						Int2 tile = lvl.GetUpStairsFrontTile();
						pc->unit->rot = DirToRot(lvl.staircase_up_dir);
						game_level->WarpUnit(*pc->unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 0;
					}
				}
				else
				{
					// poziom w górê
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
			if(input->Down(Key::Shift) && input->Pressed(Key::Period) && inside->HaveDownStairs())
			{
				if(!input->Down(Key::Control))
				{
					// teleportuj gracza do schodów w dó³
					if(Net::IsLocal())
					{
						Int2 tile = lvl.GetDownStairsFrontTile();
						pc->unit->rot = DirToRot(lvl.staircase_down_dir);
						game_level->WarpUnit(*pc->unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 1;
					}
				}
				else
				{
					// poziom w dó³
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

	// umieranie
	if((Net::IsLocal() && !team->IsAnyoneAlive()) || death_screen != 0)
	{
		if(death_screen == 0)
		{
			Info("Game over: all players died.");
			SetMusic(MusicType::Death);
			game_gui->CloseAllPanels(true);
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
		if(death_screen >= 2 && GKey.AllowKeyboard() && input->Pressed2Release(Key::Escape, Key::Enter) != Key::None)
		{
			ExitToMenu();
			return;
		}
	}

	pc->Update(dt);

	// aktualizuj ai
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

	// aktualizuj konteksty poziomów
	game_level->lights_dt += dt;
	for(LevelArea& area : game_level->ForEachArea())
		UpdateArea(area, dt);
	if(game_level->lights_dt >= 1.f / 60)
		game_level->lights_dt = 0.f;

	// aktualizacja minimapy
	game_level->UpdateDungeonMinimap(true);

	// aktualizuj dialogi
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
			DialogContext& area = *info.u->player->dialog_ctx;
			if(area.dialog_mode)
			{
				if(!area.talker->IsStanding() || !area.talker->IsIdle() || area.talker->to_remove || area.talker->frozen != FROZEN::NO)
					area.EndDialog();
				else
					area.Update(dt);
			}
		}
	}
	else
	{
		if(dialog_context.dialog_mode)
			UpdateGameDialogClient();
	}

	UpdateAttachedSounds(dt);

	game_level->ProcessRemoveUnits(false);
}

//=================================================================================================
void Game::UpdateFallback(float dt)
{
	if(fallback_type == FALLBACK::NO)
		return;

	dt /= game_speed;

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
						world->Update(10, World::UM_NORMAL);
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
						world->Update(fallback_1, World::UM_NORMAL);
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
					// aktualnie mo¿na siê tepn¹æ z X poziomu na 1 zawsze ale ¿eby z X na X to musi byæ odwiedzony
					// np w sekrecie z 3 na 1 i spowrotem do
					int at_level = 0;
					if(game_level->location->portal)
						at_level = game_level->location->portal->at_level;
					EnterLocation(at_level, portal->target);
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

	// obracanie kamery góra/dó³
	if(!Net::IsLocal() || team->IsAnyoneAlive())
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
			if(GKey.AllowMouse())
			{
				// use mouse wheel to set distance
				if(!game_gui->level_gui->IsMouseInsideDialog())
				{
					camera.dist -= input->GetMouseWheel();
					camera.dist = Clamp(camera.dist, 0.5f, 6.f);
				}
				if(input->PressedRelease(Key::MiddleButton))
					camera.dist = 3.5f;
			}
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

	// bronie
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

	// tarcze
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

	// postacie
	Info("Test: Checking units...");
	for(UnitData* ud_ptr : UnitData::units)
	{
		UnitData& ud = *ud_ptr;
		str.clear();

		// przedmioty postaci
		if(ud.item_script)
			ud.item_script->Test(str, errors);

		// ataki
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

		// model postaci
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

			// animacje ataków
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

			// animacje idle
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

			// punkt czaru
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

//=================================================================================================
bool Game::CheckForHit(LevelArea& area, Unit& unit, Unit*& hitted, Vec3& hitpoint)
{
	// atak broni¹ lub naturalny

	Mesh::Point* hitbox, *point;

	if(unit.mesh_inst->mesh->head.n_groups > 1)
	{
		Mesh* mesh = unit.GetWeapon().mesh;
		if(!mesh)
			return false;
		hitbox = mesh->FindPoint("hit");
		point = unit.mesh_inst->mesh->GetPoint(NAMES::point_weapon);
		assert(point);
	}
	else
	{
		point = nullptr;
		hitbox = unit.mesh_inst->mesh->GetPoint(Format("hitbox%d", unit.act.attack.index + 1));
		if(!hitbox)
			hitbox = unit.mesh_inst->mesh->FindPoint("hitbox");
	}

	assert(hitbox);

	return CheckForHit(area, unit, hitted, *hitbox, point, hitpoint);
}

// Sprawdza czy jest kolizja hitboxa z jak¹œ postaci¹
// Jeœli zwraca true a hitted jest nullem to trafiono w obiekt
// S¹ dwie opcje:
//  - bone to punkt "bron", hitbox to hitbox z bronii
//  - bone jest nullptr, hitbox jest na modelu postaci
// Na drodze testów ustali³em ¿e najlepiej dzia³a gdy stosuje siê sam¹ funkcjê OrientedBoxToOrientedBox
bool Game::CheckForHit(LevelArea& area, Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint)
{
	assert(hitted && hitbox.IsBox());

	// ustaw koœci
	unit.mesh_inst->SetupBones();

	// oblicz macierz hitbox

	// transformacja postaci
	Matrix m1 = Matrix::RotationY(unit.rot) * Matrix::Translation(unit.pos); // m1 (World) = Rot * Pos

	if(bone)
	{
		// m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitRot * UnitPos
		m1 = hitbox.mat * (bone->mat * unit.mesh_inst->mat_bones[bone->bone] * m1);
	}
	else
	{
		m1 = hitbox.mat * unit.mesh_inst->mat_bones[hitbox.bone] * m1;
	}

	// a - hitbox broni, b - hitbox postaci
	Oob a, b;
	m1._11 = m1._22 = m1._33 = 1.f;
	a.c = Vec3::TransformZero(m1);
	a.e = hitbox.size;
	a.u[0] = Vec3(m1._11, m1._12, m1._13);
	a.u[1] = Vec3(m1._21, m1._22, m1._23);
	a.u[2] = Vec3(m1._31, m1._32, m1._33);
	b.u[0] = Vec3(1, 0, 0);
	b.u[1] = Vec3(0, 1, 0);
	b.u[2] = Vec3(0, 0, 1);

	// szukaj kolizji
	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		if(*it == &unit || !(*it)->IsAlive() || Vec3::Distance((*it)->pos, unit.pos) > 5.f || unit.IsFriend(**it, true))
			continue;

		Box box2;
		(*it)->GetBox(box2);
		b.c = box2.Midpoint();
		b.e = box2.Size() / 2;

		if(OOBToOOB(b, a))
		{
			hitpoint = a.c;
			hitted = *it;
			return true;
		}
	}

	// collisions with melee_target
	b.e = Vec3(0.6f, 2.f, 0.6f);
	for(Object* obj : area.objects)
	{
		if(obj->base && obj->base->id == "melee_target")
		{
			b.c = obj->pos;
			b.c.y += 1.f;

			if(OOBToOOB(b, a))
			{
				hitpoint = a.c;
				hitted = nullptr;

				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = game_res->tSpark;
				pe->emision_interval = 0.01f;
				pe->life = 5.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 10;
				pe->spawn_max = 15;
				pe->max_particles = 15;
				pe->pos = hitpoint;
				pe->speed_min = Vec3(-1, 0, -1);
				pe->speed_max = Vec3(1, 1, 1);
				pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
				pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
				pe->size = 0.3f;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 0;
				pe->Init();
				area.tmp->pes.push_back(pe);

				sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, HIT_SOUND_DIST);

				if(Net::IsLocal() && unit.IsPlayer())
				{
					if(quest_mgr->quest_tutorial->in_tutorial)
						quest_mgr->quest_tutorial->HandleMeleeAttackCollision();
					unit.player->Train(TrainWhat::AttackNoDamage, 0.f, 1);
				}

				return true;
			}
		}
	}

	return false;
}

void Game::UpdateParticles(LevelArea& area, float dt)
{
	LoopAndRemove(area.tmp->pes, [dt](ParticleEmitter* pe)
		{
			if(pe->Update(dt))
			{
				if(pe->manual_delete == 0)
					delete pe;
				else
					pe->manual_delete = 2;
				return true;
			}
			return false;
		});

	LoopAndRemove(area.tmp->tpes, [dt](TrailParticleEmitter* tpe)
		{
			if(tpe->Update(dt, nullptr))
			{
				delete tpe;
				return true;
			}
			return false;
		});
}

Game::ATTACK_RESULT Game::DoAttack(LevelArea& area, Unit& unit)
{
	Vec3 hitpoint;
	Unit* hitted;

	if(!CheckForHit(area, unit, hitted, hitpoint))
		return ATTACK_NOT_HIT;

	if(!hitted)
		return ATTACK_OBJECT;

	float power;
	if(unit.data->frames->extra)
		power = 1.f;
	else
		power = unit.data->frames->attack_power[unit.act.attack.index];
	return DoGenericAttack(area, unit, *hitted, hitpoint, unit.CalculateAttack() * unit.act.attack.power * power, unit.GetDmgType(), false);
}

void Game::GiveDmg(Unit& taker, float dmg, Unit* giver, const Vec3* hitpoint, int dmg_flags)
{
	// blood particles
	if(!IsSet(dmg_flags, DMG_NO_BLOOD))
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = game_res->tBlood[taker.data->blood];
		pe->emision_interval = 0.01f;
		pe->life = 5.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 10;
		pe->spawn_max = 15;
		pe->max_particles = 15;
		if(hitpoint)
			pe->pos = *hitpoint;
		else
		{
			pe->pos = taker.pos;
			pe->pos.y += taker.GetUnitHeight() * 2.f / 3;
		}
		pe->speed_min = Vec3(-1, 0, -1);
		pe->speed_max = Vec3(1, 1, 1);
		pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
		pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
		pe->size = 0.3f;
		pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
		pe->alpha = 0.9f;
		pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
		pe->mode = 0;
		pe->Init();
		taker.area->tmp->pes.push_back(pe);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPAWN_BLOOD;
			c.id = taker.data->blood;
			c.pos = pe->pos;
		}
	}

	// apply magic resistance
	if(IsSet(dmg_flags, DMG_MAGICAL))
		dmg *= taker.CalculateMagicResistance();

	if(giver && giver->IsPlayer())
	{
		// update player damage done
		giver->player->dmg_done += (int)dmg;
		giver->player->stat_flags |= STAT_DMG_DONE;
	}

	if(taker.IsPlayer())
	{
		// update player damage taken
		taker.player->dmg_taken += (int)dmg;
		taker.player->stat_flags |= STAT_DMG_TAKEN;

		// train endurance
		taker.player->Train(TrainWhat::TakeDamage, min(dmg, taker.hp) / taker.hpmax, (giver ? giver->level : -1));

		// red screen
		taker.player->last_dmg += dmg;
	}

	// aggregate units
	if(giver && giver->IsPlayer())
		AttackReaction(taker, *giver);

	if((taker.hp -= dmg) <= 0.f && !taker.IsImmortal())
	{
		// unit killed
		taker.Die(giver);
	}
	else
	{
		// unit hurt sound
		if(taker.hurt_timer <= 0.f && taker.data->sounds->Have(SOUND_PAIN))
		{
			PlayAttachedSound(taker, taker.data->sounds->Random(SOUND_PAIN), Unit::PAIN_SOUND_DIST);
			taker.hurt_timer = Random(1.f, 1.5f);
			if(IsSet(dmg_flags, DMG_NO_BLOOD))
				taker.hurt_timer += 1.f;
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::UNIT_SOUND;
				c.unit = &taker;
				c.id = SOUND_PAIN;
			}
		}

		// immortality
		if(taker.hp < 1.f)
			taker.hp = 1.f;

		// send update hp
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = &taker;
		}
	}
}

//=================================================================================================
// Update units in area
//=================================================================================================
void Game::UpdateUnits(LevelArea& area, float dt)
{
	PROFILER_BLOCK("UpdateUnits");

	// new units can be added inside this loop - so no iterators!
	for(uint i = 0, count = area.units.size(); i < count; ++i)
		area.units[i]->Update(dt);
}

bool Game::DoShieldSmash(LevelArea& area, Unit& attacker)
{
	assert(attacker.HaveShield());

	Vec3 hitpoint;
	Unit* hitted;
	Mesh* mesh = attacker.GetShield().mesh;

	if(!mesh)
		return false;

	if(!CheckForHit(area, attacker, hitted, *mesh->FindPoint("hit"), attacker.mesh_inst->mesh->GetPoint(NAMES::point_shield), hitpoint))
		return false;

	if(!hitted)
		return true;

	if(!IsSet(hitted->data->flags, F_DONT_SUFFER) && hitted->last_bash <= 0.f)
	{
		hitted->last_bash = 1.f + float(hitted->Get(AttributeId::END)) / 50.f;

		hitted->BreakAction();

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->animation_state = AS_POSITION_HURT;

		if(hitted->mesh_inst->mesh->head.n_groups == 2)
		{
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
		}
		else
		{
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
			hitted->animation = ANI_PLAY;
		}

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.unit = hitted;
			c.type = NetChange::STUNNED;
		}
	}

	DoGenericAttack(area, attacker, *hitted, hitpoint, attacker.CalculateAttack(&attacker.GetShield()), DMG_BLUNT, true);

	return true;
}

void Game::UpdateBullets(LevelArea& area, float dt)
{
	bool deletions = false;

	for(vector<Bullet>::iterator it = area.tmp->bullets.begin(), end = area.tmp->bullets.end(); it != end; ++it)
	{
		// update position
		Vec3 prev_pos = it->pos;
		it->pos += Vec3(sin(it->rot.y) * it->speed, it->yspeed, cos(it->rot.y) * it->speed) * dt;
		if(it->ability && it->ability->type == Ability::Ball)
			it->yspeed -= 10.f * dt;

		// update particles
		if(it->pe)
			it->pe->pos = it->pos;
		if(it->trail)
			it->trail->Update(0, &it->pos);

		// remove bullet on timeout
		if((it->timer -= dt) <= 0.f)
		{
			// timeout, delete bullet
			it->remove = true;
			deletions = true;
			if(it->trail)
				it->trail->destroy = true;
			if(it->pe)
				it->pe->destroy = true;
			continue;
		}

		// do contact test
		btCollisionShape* shape;
		if(!it->ability)
			shape = game_level->shape_arrow;
		else
			shape = it->ability->shape;
		assert(shape->isConvex());

		btTransform tr_from, tr_to;
		tr_from.setOrigin(ToVector3(prev_pos));
		tr_from.setRotation(btQuaternion(it->rot.y, it->rot.x, it->rot.z));
		tr_to.setOrigin(ToVector3(it->pos));
		tr_to.setRotation(tr_from.getRotation());

		BulletCallback callback(it->owner ? it->owner->cobj : nullptr);
		phy_world->convexSweepTest((btConvexShape*)shape, tr_from, tr_to, callback);
		if(!callback.hasHit())
			continue;

		Vec3 hitpoint = ToVec3(callback.hitpoint);
		Unit* hitted = nullptr;
		if(IsSet(callback.target->getCollisionFlags(), CG_UNIT))
			hitted = (Unit*)callback.target->getUserPointer();

		// something was hit, remove bullet
		it->remove = true;
		deletions = true;
		if(it->trail)
			it->trail->destroy = true;
		if(it->pe)
			it->pe->destroy = true;

		if(hitted)
		{
			if(!Net::IsLocal())
				continue;

			if(!it->ability)
			{
				if(it->owner && it->owner->IsFriend(*hitted, true) || it->attack < -50.f)
				{
					// friendly fire
					if(hitted->IsBlocking() && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::HIT_SOUND;
							c.id = MAT_IRON;
							c.count = mat;
							c.pos = hitpoint;
						}
					}
					else
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, ARROW_HIT_SOUND_DIST, false);
					continue;
				}

				// hit enemy unit
				if(it->owner && it->owner->IsAlive() && hitted->IsAI())
					hitted->ai->HitReaction(it->start_pos);

				// calculate modifiers
				int mod = CombatHelper::CalculateModifier(DMG_PIERCE, hitted->data->flags);
				float m = 1.f;
				if(mod == -1)
					m += 0.25f;
				else if(mod == 1)
					m -= 0.25f;
				if(hitted->IsNotFighting())
					m += 0.1f;
				if(hitted->action == A_PAIN)
					m += 0.1f;

				// backstab bonus damage
				float angle_dif = AngleDiff(it->rot.y, hitted->rot);
				float backstab_mod = it->backstab;
				if(IsSet(hitted->data->flags2, F2_BACKSTAB_RES))
					backstab_mod /= 2;
				m += angle_dif / PI * backstab_mod;

				// apply modifiers
				float attack = it->attack * m;

				if(hitted->IsBlocking() && angle_dif < PI * 2 / 5)
				{
					// play sound
					MATERIAL_TYPE mat = hitted->GetShield().material;
					sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::HIT_SOUND;
						c.id = MAT_IRON;
						c.count = mat;
						c.pos = hitpoint;
					}

					// train blocking
					if(hitted->IsPlayer())
						hitted->player->Train(TrainWhat::BlockBullet, attack / hitted->hpmax, it->level);

					// reduce damage
					float block = hitted->CalculateBlock() * hitted->GetBlockMod() * (1.f - angle_dif / (PI * 2 / 5));
					float stamina = min(block, attack);
					attack -= block;
					hitted->RemoveStaminaBlock(stamina);

					// pain animation & break blocking
					if(hitted->stamina < 0)
					{
						hitted->BreakAction();

						if(!IsSet(hitted->data->flags, F_DONT_SUFFER))
						{
							if(hitted->action != A_POSITION)
								hitted->action = A_PAIN;
							else
								hitted->animation_state = AS_POSITION_HURT;

							if(hitted->mesh_inst->mesh->head.n_groups == 2)
								hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
							else
							{
								hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
								hitted->animation = ANI_PLAY;
							}
						}
					}

					if(attack < 0)
					{
						// shot blocked by shield
						if(it->owner && it->owner->IsPlayer())
						{
							// train player in bow
							it->owner->player->Train(TrainWhat::BowNoDamage, 0.f, hitted->level);
							// aggregate
							AttackReaction(*hitted, *it->owner);
						}
						continue;
					}
				}

				// calculate defense
				float def = hitted->CalculateDefense();

				// calculate damage
				float dmg = CombatHelper::CalculateDamage(attack, def);

				// hit sound
				PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, dmg > 0.f);

				// train player armor skill
				if(hitted->IsPlayer())
					hitted->player->Train(TrainWhat::TakeDamageArmor, attack / hitted->hpmax, it->level);

				if(dmg < 0)
				{
					if(it->owner && it->owner->IsPlayer())
					{
						// train player in bow
						it->owner->player->Train(TrainWhat::BowNoDamage, 0.f, hitted->level);
						// aggregate
						AttackReaction(*hitted, *it->owner);
					}
					continue;
				}

				// train player in bow
				if(it->owner && it->owner->IsPlayer())
				{
					float v = dmg / hitted->hpmax;
					if(hitted->hp - dmg < 0.f && !hitted->IsImmortal())
						v = max(TRAIN_KILL_RATIO, v);
					if(v > 1.f)
						v = 1.f;
					it->owner->player->Train(TrainWhat::BowAttack, v, hitted->level);
				}

				GiveDmg(*hitted, dmg, it->owner, &hitpoint);

				// apply poison
				if(it->poison_attack > 0.f)
				{
					float poison_res = hitted->GetPoisonResistance();
					if(poison_res > 0.f)
					{
						Effect e;
						e.effect = EffectId::Poison;
						e.source = EffectSource::Temporary;
						e.source_id = -1;
						e.value = -1;
						e.power = it->poison_attack / 5 * poison_res;
						e.time = 5.f;
						hitted->AddEffect(e);
					}
				}
			}
			else
			{
				// trafienie w postaæ z czara
				if(it->owner && it->owner->IsFriend(*hitted, true))
				{
					// frendly fire
					SpellHitEffect(area, *it, hitpoint, hitted);

					// dŸwiêk trafienia w postaæ
					if(hitted->IsBlocking() && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::HIT_SOUND;
							c.id = MAT_IRON;
							c.count = mat;
							c.pos = hitpoint;
						}
					}
					else
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, false);
					continue;
				}

				if(hitted->IsAI() && it->owner && it->owner->IsAlive())
					hitted->ai->HitReaction(it->start_pos);

				float dmg = it->attack;
				if(it->owner)
					dmg += it->owner->level * it->ability->dmg_bonus;
				float angle_dif = AngleDiff(it->rot.y, hitted->rot);
				float base_dmg = dmg;

				if(hitted->IsBlocking() && angle_dif < PI * 2 / 5)
				{
					float block = hitted->CalculateBlock() * hitted->GetBlockMod() * (1.f - angle_dif / (PI * 2 / 5));
					float stamina = min(dmg, block);
					dmg -= block / 2;
					hitted->RemoveStaminaBlock(stamina);

					if(hitted->IsPlayer())
					{
						// player blocked spell, train him
						hitted->player->Train(TrainWhat::BlockBullet, base_dmg / hitted->hpmax, it->level);
					}

					if(dmg < 0)
					{
						// blocked by shield
						SpellHitEffect(area, *it, hitpoint, hitted);
						continue;
					}
				}

				GiveDmg(*hitted, dmg, it->owner, &hitpoint, !IsSet(it->ability->flags, Ability::Poison) ? DMG_MAGICAL : 0);

				// apply poison
				if(IsSet(it->ability->flags, Ability::Poison))
				{
					float poison_res = hitted->GetPoisonResistance();
					if(poison_res > 0.f)
					{
						Effect e;
						e.effect = EffectId::Poison;
						e.source = EffectSource::Temporary;
						e.source_id = -1;
						e.value = -1;
						e.power = dmg / 5 * poison_res;
						e.time = 5.f;
						hitted->AddEffect(e);
					}
				}

				// apply spell effect
				SpellHitEffect(area, *it, hitpoint, hitted);
			}
		}
		else
		{
			// trafiono w obiekt
			if(!it->ability)
			{
				sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, ARROW_HIT_SOUND_DIST);

				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = game_res->tSpark;
				pe->emision_interval = 0.01f;
				pe->life = 5.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 10;
				pe->spawn_max = 15;
				pe->max_particles = 15;
				pe->pos = hitpoint;
				pe->speed_min = Vec3(-1, 0, -1);
				pe->speed_max = Vec3(1, 1, 1);
				pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
				pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
				pe->size = 0.3f;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 0;
				pe->Init();
				area.tmp->pes.push_back(pe);

				if(it->owner && it->owner->IsPlayer() && Net::IsLocal() && callback.target && IsSet(callback.target->getCollisionFlags(), CG_OBJECT))
				{
					Object* obj = static_cast<Object*>(callback.target->getUserPointer());
					if(obj && obj->base && obj->base->id == "bow_target")
					{
						if(quest_mgr->quest_tutorial->in_tutorial)
							quest_mgr->quest_tutorial->HandleBulletCollision();
						it->owner->player->Train(TrainWhat::BowNoDamage, 0.f, 1);
					}
				}
			}
			else
			{
				// trafienie czarem w obiekt
				SpellHitEffect(area, *it, hitpoint, nullptr);
			}
		}
	}

	if(deletions)
		RemoveElements(area.tmp->bullets, [](const Bullet& b) { return b.remove; });
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
		// poziom w górê
		if(game_level->dungeon_level == 0)
		{
			if(quest_mgr->quest_tutorial->in_tutorial)
			{
				quest_mgr->quest_tutorial->OnEvent(Quest_Tutorial::Exit);
				fallback_type = FALLBACK::CLIENT;
				fallback_t = 0.f;
				return;
			}

			// wyjœcie na powierzchniê
			ExitToMap();
			return;
		}
		else
		{
			LoadingStart(4);
			LoadingStep(txLevelUp);

			MultiInsideLocation* inside = (MultiInsideLocation*)game_level->location;
			LeaveLevel();
			--game_level->dungeon_level;
			LocationGenerator* loc_gen = loc_gen_factory->Get(inside);
			game_level->enter_from = ENTER_FROM_DOWN_LEVEL;
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

		// poziom w dó³
		LeaveLevel();
		++game_level->dungeon_level;

		LocationGenerator* loc_gen = loc_gen_factory->Get(inside);

		// czy to pierwsza wizyta?
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

		game_level->enter_from = ENTER_FROM_UP_LEVEL;
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
	// zamknij gui layer
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

Game::ATTACK_RESULT Game::DoGenericAttack(LevelArea& area, Unit& attacker, Unit& hitted, const Vec3& hitpoint, float attack, int dmg_type, bool bash)
{
	// calculate modifiers
	int mod = CombatHelper::CalculateModifier(dmg_type, hitted.data->flags);
	float m = 1.f;
	if(mod == -1)
		m += 0.25f;
	else if(mod == 1)
		m -= 0.25f;
	if(hitted.IsNotFighting())
		m += 0.1f;
	else if(hitted.IsHoldingBow())
		m += 0.05f;
	if(hitted.action == A_PAIN)
		m += 0.1f;

	// backstab bonus
	float angle_dif = AngleDiff(Clip(attacker.rot + PI), hitted.rot);
	float backstab_mod = attacker.GetBackstabMod(bash ? attacker.slots[SLOT_SHIELD] : attacker.slots[SLOT_WEAPON]);
	if(IsSet(hitted.data->flags2, F2_BACKSTAB_RES))
		backstab_mod /= 2;
	m += angle_dif / PI * backstab_mod;

	// apply modifiers
	attack *= m;

	// blocking
	if(hitted.IsBlocking() && angle_dif < PI / 2)
	{
		// play sound
		MATERIAL_TYPE hitted_mat = hitted.GetShield().material;
		MATERIAL_TYPE weapon_mat = (!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HIT_SOUND;
			c.pos = hitpoint;
			c.id = weapon_mat;
			c.count = hitted_mat;
		}
		sound_mgr->PlaySound3d(game_res->GetMaterialSound(weapon_mat, hitted_mat), hitpoint, HIT_SOUND_DIST);

		// train blocking
		if(hitted.IsPlayer())
			hitted.player->Train(TrainWhat::BlockAttack, attack / hitted.hpmax, attacker.level);

		// reduce damage
		float block = hitted.CalculateBlock() * hitted.GetBlockMod();
		float stamina = min(attack, block);
		if(IsSet(attacker.data->flags2, F2_IGNORE_BLOCK))
			block *= 2.f / 3;
		if(attacker.act.attack.power >= 1.9f)
			stamina *= 4.f / 3;
		attack -= block;
		hitted.RemoveStaminaBlock(stamina);

		// pain animation & break blocking
		if(hitted.stamina < 0)
		{
			hitted.BreakAction();

			if(!IsSet(hitted.data->flags, F_DONT_SUFFER))
			{
				if(hitted.action != A_POSITION)
					hitted.action = A_PAIN;
				else
					hitted.animation_state = AS_POSITION_HURT;

				if(hitted.mesh_inst->mesh->head.n_groups == 2)
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
				else
				{
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
					hitted.animation = ANI_PLAY;
				}
			}
		}

		// attack fully blocked
		if(attack < 0)
		{
			if(attacker.IsPlayer())
			{
				// player attack blocked
				attacker.player->Train(bash ? TrainWhat::BashNoDamage : TrainWhat::AttackNoDamage, 0.f, hitted.level);
				// aggregate
				AttackReaction(hitted, attacker);
			}
			return ATTACK_BLOCKED;
		}
	}

	// calculate defense
	float def = hitted.CalculateDefense();

	// calculate damage
	float dmg = CombatHelper::CalculateDamage(attack, def);

	// hit sound
	PlayHitSound(!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material, hitted.GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, dmg > 0.f);

	// train player armor skill
	if(hitted.IsPlayer())
		hitted.player->Train(TrainWhat::TakeDamageArmor, attack / hitted.hpmax, attacker.level);

	// fully blocked by armor
	if(dmg < 0)
	{
		if(attacker.IsPlayer())
		{
			// player attack blocked
			attacker.player->Train(bash ? TrainWhat::BashNoDamage : TrainWhat::AttackNoDamage, 0.f, hitted.level);
			// aggregate
			AttackReaction(hitted, attacker);
		}
		return ATTACK_NO_DAMAGE;
	}

	if(attacker.IsPlayer())
	{
		// player hurt someone - train
		float dmgf = (float)dmg;
		float ratio;
		if(hitted.hp - dmgf <= 0.f && !hitted.IsImmortal())
			ratio = Clamp(dmgf / hitted.hpmax, TRAIN_KILL_RATIO, 1.f);
		else
			ratio = min(1.f, dmgf / hitted.hpmax);
		attacker.player->Train(bash ? TrainWhat::BashHit : TrainWhat::AttackHit, ratio, hitted.level);
	}

	GiveDmg(hitted, dmg, &attacker, &hitpoint);

	// apply poison
	if(IsSet(attacker.data->flags, F_POISON_ATTACK))
	{
		float poison_res = hitted.GetPoisonResistance();
		if(poison_res > 0.f)
		{
			Effect e;
			e.effect = EffectId::Poison;
			e.source = EffectSource::Temporary;
			e.source_id = -1;
			e.value = -1;
			e.power = dmg / 10 * poison_res;
			e.time = 5.f;
			hitted.AddEffect(e);
		}
	}

	return hitted.action == A_PAIN ? ATTACK_CLEAN_HIT : ATTACK_HIT;
}

void Game::SpellHitEffect(LevelArea& area, Bullet& bullet, const Vec3& pos, Unit* hitted)
{
	Ability& ability = *bullet.ability;

	// dŸwiêk
	if(ability.sound_hit)
		sound_mgr->PlaySound3d(ability.sound_hit, pos, ability.sound_hit_dist);

	// cz¹steczki
	if(ability.tex_particle && ability.type == Ability::Ball)
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = ability.tex_particle;
		pe->emision_interval = 0.01f;
		pe->life = 0.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 8;
		pe->spawn_max = 12;
		pe->max_particles = 12;
		pe->pos = pos;
		pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
		pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
		pe->pos_min = Vec3(-ability.size, -ability.size, -ability.size);
		pe->pos_max = Vec3(ability.size, ability.size, ability.size);
		pe->size = ability.size / 2;
		pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
		pe->alpha = 1.f;
		pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
		pe->mode = 1;
		pe->Init();
		area.tmp->pes.push_back(pe);
	}

	// wybuch
	if(Net::IsLocal() && IsSet(ability.flags, Ability::Explode))
	{
		Explo* explo = new Explo;
		explo->dmg = (float)ability.dmg;
		if(bullet.owner)
			explo->dmg += float((bullet.owner->level + bullet.owner->CalculateMagicPower()) * ability.dmg_bonus);
		explo->size = 0.f;
		explo->sizemax = ability.explode_range;
		explo->pos = pos;
		explo->ability = &ability;
		explo->owner = bullet.owner;
		if(hitted)
			explo->hitted.push_back(hitted);
		area.tmp->explos.push_back(explo);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CREATE_EXPLOSION;
			c.ability = &ability;
			c.pos = explo->pos;
		}
	}
}

void Game::UpdateExplosions(LevelArea& area, float dt)
{
	LoopAndRemove(area.tmp->explos, [&](Explo* p_explo)
		{
			Explo& explo = *p_explo;

			// increase size
			bool delete_me = false;
			explo.size += explo.sizemax * dt;
			if(explo.size >= explo.sizemax)
			{
				delete_me = true;
				explo.size = explo.sizemax;
			}

			if(Net::IsLocal())
			{
				// deal damage
				Unit* owner = explo.owner;
				float dmg = explo.dmg * Lerp(1.f, 0.1f, explo.size / explo.sizemax);
				for(Unit* unit : area.units)
				{
					if(!unit->IsAlive() || (owner && owner->IsFriend(*unit, true)))
						continue;

					if(!IsInside(explo.hitted, unit))
					{
						Box box;
						unit->GetBox(box);

						if(SphereToBox(explo.pos, explo.size, box))
						{
							GiveDmg(*unit, dmg, owner, nullptr, DMG_NO_BLOOD | DMG_MAGICAL);
							explo.hitted.push_back(unit);
						}
					}
				}
			}

			if(delete_me)
				delete p_explo;
			return delete_me;
		});
}

void Game::UpdateTraps(LevelArea& area, float dt)
{
	const bool is_local = Net::IsLocal();
	LoopAndRemove(area.traps, [&](Trap* p_trap)
		{
			Trap& trap = *p_trap;

			if(trap.state == -1)
			{
				trap.time -= dt;
				if(trap.time <= 0.f)
					trap.state = 0;
				return false;
			}

			switch(trap.base->type)
			{
			case TRAP_SPEAR:
				if(trap.state == 0)
				{
					// check if someone is step on it
					bool trigger = false;
					if(is_local)
					{
						for(Unit* unit : area.units)
						{
							if(unit->IsStanding() && !IsSet(unit->data->flags, F_SLIGHT)
								&& CircleToCircle(trap.pos.x, trap.pos.z, trap.base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
							{
								trigger = true;
								break;
							}
						}
					}
					else if(trap.trigger)
					{
						trigger = true;
						trap.trigger = false;
					}

					if(trigger)
					{
						sound_mgr->PlaySound3d(trap.base->sound, trap.pos, trap.base->sound_dist);
						trap.state = 1;
						trap.time = Random(0.5f, 0.75f);

						if(Net::IsServer())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::TRIGGER_TRAP;
							c.id = trap.id;
						}
					}
				}
				else if(trap.state == 1)
				{
					// count timer to spears come out
					bool trigger = false;
					if(is_local)
					{
						trap.time -= dt;
						if(trap.time <= 0.f)
							trigger = true;
					}
					else if(trap.trigger)
					{
						trigger = true;
						trap.trigger = false;
					}

					if(trigger)
					{
						trap.state = 2;
						trap.time = 0.f;

						sound_mgr->PlaySound3d(trap.base->sound2, trap.pos, trap.base->sound_dist2);

						if(Net::IsServer())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::TRIGGER_TRAP;
							c.id = trap.id;
						}
					}
				}
				else if(trap.state == 2)
				{
					// move spears
					bool end = false;
					trap.time += dt;
					if(trap.time >= 0.27f)
					{
						trap.time = 0.27f;
						end = true;
					}

					trap.obj2.pos.y = trap.obj.pos.y - 2.f + 2.f * (trap.time / 0.27f);

					if(is_local)
					{
						for(Unit* unit : area.units)
						{
							if(!unit->IsAlive())
								continue;
							if(CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
							{
								bool found = false;
								for(Unit* unit2 : *trap.hitted)
								{
									if(unit == unit2)
									{
										found = true;
										break;
									}
								}

								if(!found)
								{
									// hit unit with spears
									int mod = CombatHelper::CalculateModifier(DMG_PIERCE, unit->data->flags);
									float m = 1.f;
									if(mod == -1)
										m += 0.25f;
									else if(mod == 1)
										m -= 0.25f;
									if(unit->action == A_PAIN)
										m += 0.1f;

									// calculate attack & defense
									float attack = float(trap.base->attack) * m;
									float def = unit->CalculateDefense();
									float dmg = CombatHelper::CalculateDamage(attack, def);

									// dŸwiêk trafienia
									sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, unit->GetBodyMaterial()), unit->pos + Vec3(0, 1.f, 0), HIT_SOUND_DIST);

									// train player armor skill
									if(unit->IsPlayer())
										unit->player->Train(TrainWhat::TakeDamageArmor, attack / unit->hpmax, 4);

									// damage
									if(dmg > 0)
										GiveDmg(*unit, dmg);

									trap.hitted->push_back(unit);
								}
							}
						}
					}

					if(end)
					{
						trap.state = 3;
						if(is_local)
							trap.hitted->clear();
						trap.time = 1.f;
					}
				}
				else if(trap.state == 3)
				{
					// count timer to hide spears
					trap.time -= dt;
					if(trap.time <= 0.f)
					{
						trap.state = 4;
						trap.time = 1.5f;
						sound_mgr->PlaySound3d(trap.base->sound3, trap.pos, trap.base->sound_dist3);
					}
				}
				else if(trap.state == 4)
				{
					// hiding spears
					trap.time -= dt;
					if(trap.time <= 0.f)
					{
						trap.time = 0.f;
						trap.state = 5;
					}

					trap.obj2.pos.y = trap.obj.pos.y - 2.f + trap.time / 1.5f * 2.f;
				}
				else if(trap.state == 5)
				{
					// spears are hidden, wait until unit moves away to reactivate
					bool reactivate;
					if(is_local)
					{
						reactivate = true;
						for(Unit* unit : area.units)
						{
							if(!IsSet(unit->data->flags, F_SLIGHT)
								&& CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
							{
								reactivate = false;
								break;
							}
						}
					}
					else
					{
						if(trap.trigger)
						{
							reactivate = true;
							trap.trigger = false;
						}
						else
							reactivate = false;
					}

					if(reactivate)
					{
						trap.state = 0;
						if(Net::IsServer())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::TRIGGER_TRAP;
							c.id = trap.id;
						}
					}
				}
				break;
			case TRAP_ARROW:
			case TRAP_POISON:
				if(trap.state == 0)
				{
					// check if someone is step on it
					bool trigger = false;
					if(is_local)
					{
						for(Unit* unit : area.units)
						{
							if(unit->IsStanding() && !IsSet(unit->data->flags, F_SLIGHT)
								&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
							{
								trigger = true;
								break;
							}
						}
					}
					else if(trap.trigger)
					{
						trigger = true;
						trap.trigger = false;
					}

					if(trigger)
					{
						// someone step on trap, shoot arrow
						trap.state = is_local ? 1 : 2;
						trap.time = Random(5.f, 7.5f);
						sound_mgr->PlaySound3d(trap.base->sound, trap.pos, trap.base->sound_dist);

						if(is_local)
						{
							Bullet& b = Add1(area.tmp->bullets);
							b.level = 4;
							b.backstab = 0.25f;
							b.attack = float(trap.base->attack);
							b.mesh = game_res->aArrow;
							b.pos = Vec3(2.f * trap.tile.x + trap.pos.x - float(int(trap.pos.x / 2) * 2) + Random(-trap.base->rw, trap.base->rw) - 1.2f * DirToPos(trap.dir).x,
								Random(0.5f, 1.5f),
								2.f * trap.tile.y + trap.pos.z - float(int(trap.pos.z / 2) * 2) + Random(-trap.base->h, trap.base->h) - 1.2f * DirToPos(trap.dir).y);
							b.start_pos = b.pos;
							b.rot = Vec3(0, DirToRot(trap.dir), 0);
							b.owner = nullptr;
							b.pe = nullptr;
							b.remove = false;
							b.speed = TRAP_ARROW_SPEED;
							b.ability = nullptr;
							b.tex = nullptr;
							b.tex_size = 0.f;
							b.timer = ARROW_TIMER;
							b.yspeed = 0.f;
							b.poison_attack = (trap.base->type == TRAP_POISON ? float(trap.base->attack) : 0.f);

							TrailParticleEmitter* tpe = new TrailParticleEmitter;
							tpe->fade = 0.3f;
							tpe->color1 = Vec4(1, 1, 1, 0.5f);
							tpe->color2 = Vec4(1, 1, 1, 0);
							tpe->Init(50);
							area.tmp->tpes.push_back(tpe);
							b.trail = tpe;

							sound_mgr->PlaySound3d(game_res->sBow[Rand() % 2], b.pos, SHOOT_SOUND_DIST);

							if(Net::IsServer())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::SHOOT_ARROW;
								c.unit = nullptr;
								c.pos = b.start_pos;
								c.f[0] = b.rot.y;
								c.f[1] = 0.f;
								c.f[2] = 0.f;
								c.extra_f = b.speed;

								NetChange& c2 = Add1(Net::changes);
								c2.type = NetChange::TRIGGER_TRAP;
								c2.id = trap.id;
							}
						}
					}
				}
				else if(trap.state == 1)
				{
					trap.time -= dt;
					if(trap.time <= 0.f)
						trap.state = 2;
				}
				else
				{
					// check if units leave trap
					bool empty;
					if(is_local)
					{
						empty = true;
						for(Unit* unit : area.units)
						{
							if(!IsSet(unit->data->flags, F_SLIGHT)
								&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
							{
								empty = false;
								break;
							}
						}
					}
					else
					{
						if(trap.trigger)
						{
							empty = true;
							trap.trigger = false;
						}
						else
							empty = false;
					}

					if(empty)
					{
						trap.state = 0;
						if(Net::IsServer())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::TRIGGER_TRAP;
							c.id = trap.id;
						}
					}
				}
				break;
			case TRAP_FIREBALL:
				{
					if(!is_local)
						break;

					bool trigger = false;
					for(Unit* unit : area.units)
					{
						if(unit->IsStanding()
							&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							trigger = true;
							break;
						}
					}

					if(trigger)
					{
						Ability* fireball = Ability::Get("fireball");

						Explo* explo = new Explo;
						explo->pos = trap.pos;
						explo->pos.y += 0.2f;
						explo->size = 0.f;
						explo->sizemax = 2.f;
						explo->dmg = float(trap.base->attack);
						explo->ability = fireball;

						sound_mgr->PlaySound3d(fireball->sound_hit, explo->pos, fireball->sound_hit_dist);

						area.tmp->explos.push_back(explo);

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CREATE_EXPLOSION;
							c.ability = fireball;
							c.pos = explo->pos;

							NetChange& c2 = Add1(Net::changes);
							c2.type = NetChange::REMOVE_TRAP;
							c2.id = trap.id;
						}

						delete p_trap;
						return true;
					}
				}
				break;
			default:
				assert(0);
				break;
			}

			return false;
		});
}

void Game::PreloadTraps(vector<Trap*>& traps)
{
	for(Trap* trap : traps)
	{
		auto& base = *trap->base;
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
}

void Game::UpdateElectros(LevelArea& area, float dt)
{
	LoopAndRemove(area.tmp->electros, [&](Electro* p_electro)
		{
			Electro& electro = *p_electro;
			electro.Update(dt);

			if(!Net::IsLocal())
			{
				if(electro.lines.back().t >= 0.5f)
				{
					delete p_electro;
					return true;
				}
			}
			else if(electro.valid)
			{
				if(electro.lines.back().t >= 0.25f)
				{
					Unit* hitted = electro.hitted.back();
					Unit* owner = electro.owner;
					if(!hitted || !owner)
					{
						electro.valid = false;
						return false;
					}

					const Vec3 target_pos = electro.lines.back().to;

					// deal damage
					if(!owner->IsFriend(*hitted, true))
					{
						if(hitted->IsAI() && owner->IsAlive())
							hitted->ai->HitReaction(electro.start_pos);
						GiveDmg(*hitted, electro.dmg, owner, nullptr, DMG_NO_BLOOD | DMG_MAGICAL);
					}

					// play sound
					if(electro.ability->sound_hit)
						sound_mgr->PlaySound3d(electro.ability->sound_hit, target_pos, electro.ability->sound_hit_dist);

					// add particles
					if(electro.ability->tex_particle)
					{
						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = electro.ability->tex_particle;
						pe->emision_interval = 0.01f;
						pe->life = 0.f;
						pe->particle_life = 0.5f;
						pe->emisions = 1;
						pe->spawn_min = 8;
						pe->spawn_max = 12;
						pe->max_particles = 12;
						pe->pos = target_pos;
						pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
						pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
						pe->pos_min = Vec3(-electro.ability->size, -electro.ability->size, -electro.ability->size);
						pe->pos_max = Vec3(electro.ability->size, electro.ability->size, electro.ability->size);
						pe->size = electro.ability->size_particle;
						pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
						pe->alpha = 1.f;
						pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
						pe->mode = 1;
						pe->Init();
						area.tmp->pes.push_back(pe);
					}

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ELECTRO_HIT;
						c.pos = target_pos;
					}

					if(electro.dmg >= 10.f)
					{
						static vector<pair<Unit*, float>> targets;

						// traf w kolejny cel
						for(vector<Unit*>::iterator it2 = area.units.begin(), end2 = area.units.end(); it2 != end2; ++it2)
						{
							if(!(*it2)->IsAlive() || IsInside(electro.hitted, *it2))
								continue;

							float dist = Vec3::Distance((*it2)->pos, hitted->pos);
							if(dist <= 5.f)
								targets.push_back(pair<Unit*, float>(*it2, dist));
						}

						if(!targets.empty())
						{
							if(targets.size() > 1)
							{
								std::sort(targets.begin(), targets.end(), [](const pair<Unit*, float>& target1, const pair<Unit*, float>& target2)
									{
										return target1.second < target2.second;
									});
							}

							Unit* target = nullptr;
							float dist;

							for(vector<pair<Unit*, float>>::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
							{
								Vec3 hitpoint;
								Unit* new_hitted;
								if(game_level->RayTest(target_pos, it2->first->GetCenter(), hitted, hitpoint, new_hitted))
								{
									if(new_hitted == it2->first)
									{
										target = it2->first;
										dist = it2->second;
										break;
									}
								}
							}

							if(target)
							{
								// kolejny cel
								electro.dmg = min(electro.dmg / 2, Lerp(electro.dmg, electro.dmg / 5, dist / 5));
								electro.valid = true;
								electro.hitted.push_back(target);
								Vec3 from = electro.lines.back().to;
								Vec3 to = target->GetCenter();
								electro.AddLine(from, to);

								if(Net::IsOnline())
								{
									NetChange& c = Add1(Net::changes);
									c.type = NetChange::UPDATE_ELECTRO;
									c.e_id = electro.id;
									c.pos = to;
								}
							}
							else
							{
								// brak kolejnego celu
								electro.valid = false;
							}

							targets.clear();
						}
						else
							electro.valid = false;
					}
					else
					{
						// trafi³ ju¿ wystarczaj¹co du¿o postaci
						electro.valid = false;
					}
				}
			}
			else
			{
				if(electro.hitsome && electro.lines.back().t >= 0.25f)
				{
					const Vec3 target_pos = electro.lines.back().to;
					electro.hitsome = false;

					if(electro.ability->sound_hit)
						sound_mgr->PlaySound3d(electro.ability->sound_hit, target_pos, electro.ability->sound_hit_dist);

					// cz¹steczki
					if(electro.ability->tex_particle)
					{
						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = electro.ability->tex_particle;
						pe->emision_interval = 0.01f;
						pe->life = 0.f;
						pe->particle_life = 0.5f;
						pe->emisions = 1;
						pe->spawn_min = 8;
						pe->spawn_max = 12;
						pe->max_particles = 12;
						pe->pos = target_pos;
						pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
						pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
						pe->pos_min = Vec3(-electro.ability->size, -electro.ability->size, -electro.ability->size);
						pe->pos_max = Vec3(electro.ability->size, electro.ability->size, electro.ability->size);
						pe->size = electro.ability->size_particle;
						pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
						pe->alpha = 1.f;
						pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
						pe->mode = 1;
						pe->Init();
						area.tmp->pes.push_back(pe);
					}

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ELECTRO_HIT;
						c.pos = target_pos;
					}
				}
				if(electro.lines.back().t >= 0.5f)
				{
					delete p_electro;
					return true;
				}
			}

			return false;
		});
}

void Game::UpdateDrains(LevelArea& area, float dt)
{
	LoopAndRemove(area.tmp->drains, [&](Drain& drain)
		{
			drain.t += dt;

			if(drain.pe->manual_delete == 2)
			{
				delete drain.pe;
				return true;
			}

			if(Unit* target = drain.target)
			{
				Vec3 center = target->GetCenter();
				for(ParticleEmitter::Particle& p : drain.pe->particles)
					p.pos = Vec3::Lerp(p.pos, center, drain.t / 1.5f);

				return false;
			}
			else
			{
				drain.pe->time = 0.3f;
				drain.pe->manual_delete = 0;
				return true;
			}
		});
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
	team->ClearOnNewGameOrLoad();
	draw_flags = 0xFFFFFFFF;
	game_gui->level_gui->Reset();
	game_gui->journal->Reset();
	arena->Reset();
	game_gui->level_gui->visible = false;
	game_gui->inventory->lock = nullptr;
	game_level->camera.Reset(new_game);
	game_level->lights_dt = 1.f;
	pc->data.Reset();
	script_mgr->Reset();
	game_level->Reset();
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

	// odciemnianie na pocz¹tku
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
		team->Reset();
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

#ifdef _DEBUG
		noai = true;
		dont_wander = true;
#endif
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
	// ustawienia t³a
	game_level->camera.zfar = base.draw_range;
	game_level->scene->fog_range = base.fog_range;
	game_level->scene->fog_color = base.fog_color;
	game_level->scene->ambient_color = base.ambient_color;
	game_level->scene->use_light_dir = false;
	clear_color_next = game_level->scene->fog_color;

	// tekstury podziemi
	ApplyLocationTextureOverride(game_res->tFloor[0], game_res->tWall[0], game_res->tCeil[0], base.tex);

	// druga tekstura
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

	// ustawienia uv podziemi
	dun_mesh_builder->ChangeTexWrap(!IsSet(base.options, BLO_NO_TEX_WRAP));
}

void Game::SetDungeonParamsToMeshes()
{
	// tekstury schodów / pu³apek
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

	// druga tekstura
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
		for(LevelArea& area : game_level->ForEachArea())
		{
			LeaveLevel(area, clear);
			area.tmp->Free();
			area.tmp = nullptr;
		}
		if(game_level->city_ctx && (!Net::IsLocal() || net->was_client))
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
		{
			delete chest->mesh_inst;
			chest->mesh_inst = nullptr;
		}

		// remove door meshes
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.state == Door::Closing || door.state == Door::Closing2)
				door.state = Door::Closed;
			else if(door.state == Door::Opening || door.state == Door::Opening2)
				door.state = Door::Opened;
			delete door.mesh_inst;
			door.mesh_inst = nullptr;
		}
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

void Game::UpdateArea(LevelArea& area, float dt)
{
	ProfilerBlock profiler_block([&] { return Format("UpdateArea %s", area.GetName()); });

	// aktualizuj postacie
	UpdateUnits(area, dt);

	if(game_level->lights_dt >= 1.f / 60)
		UpdateLights(area.lights);

	// update chests
	for(vector<Chest*>::iterator it = area.chests.begin(), end = area.chests.end(); it != end; ++it)
		(*it)->mesh_inst->Update(dt);

	// update doors
	for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
	{
		Door& door = **it;
		door.mesh_inst->Update(dt);
		if(door.state == Door::Opening || door.state == Door::Opening2)
		{
			bool done = door.mesh_inst->IsEnded();
			if(door.state == Door::Opening)
			{
				if(done || door.mesh_inst->GetProgress() >= 0.25f)
				{
					door.state = Door::Opening2;
					btVector3& pos = door.phy->getWorldTransform().getOrigin();
					pos.setY(pos.y() - 100.f);
				}
			}
			if(done)
				door.state = Door::Opened;
		}
		else if(door.state == Door::Closing || door.state == Door::Closing2)
		{
			bool done = door.mesh_inst->IsEnded();
			if(door.state == Door::Closing)
			{
				if(done || door.mesh_inst->GetProgress() <= 0.25f)
				{
					bool blocking = false;

					for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
					{
						if((*it)->IsAlive() && CircleToRotatedRectangle((*it)->pos.x, (*it)->pos.z, (*it)->GetUnitRadius(), door.pos.x, door.pos.z, Door::WIDTH, Door::THICKNESS, door.rot))
						{
							blocking = true;
							break;
						}
					}

					if(!blocking)
					{
						door.state = Door::Closing2;
						btVector3& pos = door.phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() + 100.f);
					}
				}
			}
			if(done)
			{
				if(door.state == Door::Closing2)
					door.state = Door::Closed;
				else
				{
					// nie mo¿na zamknaæ drzwi bo coœ blokuje
					door.state = Door::Opening2;
					door.mesh_inst->Play(&door.mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_NO_BLEND | PLAY_STOP_AT_END, 0);
					// mo¿na by daæ lepszy punkt dŸwiêku
					sound_mgr->PlaySound3d(game_res->sDoorBudge, door.pos, Door::BLOCKED_SOUND_DIST);
				}
			}
		}
	}

	// aktualizuj pu³apki
	UpdateTraps(area, dt);

	// aktualizuj pociski i efekty czarów
	UpdateBullets(area, dt);
	UpdateElectros(area, dt);
	UpdateExplosions(area, dt);
	UpdateParticles(area, dt);
	UpdateDrains(area, dt);

	// update blood spatters
	for(Blood& blood : area.bloods)
	{
		blood.size += dt;
		if(blood.size >= 1.f)
			blood.size = 1.f;
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
void Game::LoadResources(cstring text, bool worldmap)
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

	if(game_level->location)
	{
		// spawn blood for units that are dead and their mesh just loaded
		game_level->SpawnBlood();

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
			PreloadUnits(area.units);
			// some traps respawn
			PreloadTraps(area.traps);

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

			// load objects
			for(Object* obj : game_level->local_area->objects)
				res_mgr->Load(obj->mesh);

			// load usables
			PreloadUsables(game_level->local_area->usables);

			if(game_level->city_ctx)
			{
				// load buildings
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

				for(InsideBuilding* ib : game_level->city_ctx->inside_buildings)
				{
					// load building objects
					for(Object* obj : ib->objects)
						res_mgr->Load(obj->mesh);

					// load building usables
					PreloadUsables(ib->usables);

					// load units inside building
					PreloadUnits(ib->units);
				}
			}
		}
	}

	for(const Item* item : items_load)
		game_res->PreloadItem(item);
}

void Game::PreloadUsables(vector<Usable*>& usables)
{
	for(auto u : usables)
	{
		auto base = u->base;
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

void Game::PreloadUnits(vector<Unit*>& units)
{
	for(Unit* unit : units)
		PreloadUnit(unit);
}

void Game::PreloadUnit(Unit* unit)
{
	UnitData& data = *unit->data;

	if(Net::IsLocal())
	{
		for(uint i = 0; i < SLOT_MAX; ++i)
		{
			if(unit->slots[i])
				items_load.insert(unit->slots[i]);
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

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(unit->slots[i])
			VerifyItemResources(unit->slots[i]);
	}
	for(auto& slot : unit->items)
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

	if(unit->cobj)
	{
		delete unit->cobj->getCollisionShape();
		phy_world->removeCollisionObject(unit->cobj);
		delete unit->cobj;
	}

	if(--unit->refs == 0)
		delete unit;
}

void Game::AttackReaction(Unit& attacked, Unit& attacker)
{
	if(attacker.IsPlayer() && attacked.IsAI() && attacked.in_arena == -1 && !attacked.attack_team)
	{
		if(attacked.data->group == G_CITIZENS)
		{
			if(!team->is_bandit)
			{
				if(attacked.dont_attack && IsSet(attacked.data->flags, F_PEACEFUL))
				{
					if(attacked.ai->state == AIController::Idle)
					{
						attacked.ai->state = AIController::Escape;
						attacked.ai->timer = Random(2.f, 4.f);
						attacked.ai->target = attacker;
						attacked.ai->target_last_pos = attacker.pos;
						attacked.ai->st.escape.room = nullptr;
						attacked.ai->ignore = 0.f;
						attacked.ai->in_combat = false;
					}
				}
				else
					team->SetBandit(true);
			}
		}
		else if(attacked.data->group == G_CRAZIES)
		{
			if(!team->crazies_attack)
			{
				team->crazies_attack = true;
				if(Net::IsOnline())
					Net::PushChange(NetChange::CHANGE_FLAGS);
			}
		}
		if(attacked.dont_attack && !IsSet(attacked.data->flags, F_PEACEFUL))
		{
			for(vector<Unit*>::iterator it = game_level->local_area->units.begin(), end = game_level->local_area->units.end(); it != end; ++it)
			{
				if((*it)->dont_attack && !IsSet((*it)->data->flags, F_PEACEFUL))
				{
					(*it)->dont_attack = false;
					(*it)->ai->change_ai_mode = true;
				}
			}
		}
	}
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

// zamyka ekwipunek i wszystkie okna które on móg³by utworzyæ
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

void Game::UpdateGameDialogClient()
{
	if(dialog_context.show_choices)
	{
		if(game_gui->level_gui->UpdateChoice(dialog_context, dialog_choices.size()))
		{
			dialog_context.show_choices = false;
			dialog_context.dialog_text = "";

			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHOICE;
			c.id = dialog_context.choice_selected;
		}
	}
	else if(dialog_context.dialog_wait > 0.f && dialog_context.skip_id != -1)
	{
		if(GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE)
			|| (GKey.AllowKeyboard() && input->PressedRelease(Key::Escape)))
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SKIP_DIALOG;
			c.id = dialog_context.skip_id;
			dialog_context.skip_id = -1;
		}
	}
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
	if(quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::ToldAboutCamp && quest_mgr->quest_orcs2->target_loc == game_level->location_index
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
		bool always_use = false;

		if(info.sane_heroes > 0)
		{
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
			int d = quest_evil->GetLocId(game_level->location_index);
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
		else if(game_level->location_index == quest_evil->target_loc && !quest_evil->told_about_boss)
		{
			quest_evil->told_about_boss = true;
			talker = quest_evil->cleric;
			text = txXarDanger;
		}
	}

	// orc talking after entering level
	Quest_Orcs2* quest_orcs2 = quest_mgr->quest_orcs2;
	if(!talker && (quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs || quest_orcs2->orcs_state == Quest_Orcs2::State::GeneratedOrcs) && game_level->location_index == quest_orcs2->target_loc)
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
		if(quest_mages2->target_loc == game_level->location_index)
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

cstring Game::GetRandomIdleText(Unit& u)
{
	if(IsSet(u.data->flags3, F3_DRUNK_MAGE) && quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::MageCured)
		return RandomString(txAiDrunkMageText);

	int n = Rand() % 100;
	if(n == 0)
		return RandomString(txAiSecretText);

	bool hero_text;

	switch(u.data->group)
	{
	case G_CRAZIES:
		if(u.IsTeamMember())
		{
			if(n < 33)
				return RandomString(txAiInsaneText);
			else if(n < 66)
				hero_text = true;
			else
				hero_text = false;
		}
		else
		{
			if(n < 50)
				return RandomString(txAiInsaneText);
			else
				hero_text = false;
		}
		break;
	case G_CITIZENS:
		if(!u.IsTeamMember())
		{
			if(u.area->area_type == LevelArea::Type::Building && (IsSet(u.data->flags, F_AI_DRUNKMAN) || IsSet(u.data->flags3, F3_DRUNKMAN_AFTER_CONTEST)))
			{
				if(game_level->city_ctx->FindInn() == u.area)
				{
					if(IsSet(u.data->flags, F_AI_DRUNKMAN) || quest_mgr->quest_contest->state != Quest_Contest::CONTEST_TODAY)
					{
						if(Rand() % 3 == 0)
							return RandomString(txAiDrunkText);
					}
					else
						return RandomString(txAiDrunkContestText);
				}
			}
			if(n < 50)
				return RandomString(txAiHumanText);
			else
				hero_text = false;
		}
		else
		{
			if(n < 10)
				return RandomString(txAiHumanText);
			else if(n < 55)
				hero_text = true;
			else
				hero_text = false;
		}
		break;
	case G_BANDITS:
		if(n < 50)
			return RandomString(txAiBanditText);
		else
			hero_text = false;
		break;
	case G_MAGES:
	case G_UNDEAD:
		if(IsSet(u.data->flags, F_MAGE) && n < 50)
			return RandomString(txAiMageText);
		else
			hero_text = false;
		break;
	case G_GOBLINS:
		if(!IsSet(u.data->flags2, F2_NOT_GOBLIN))
			return RandomString(txAiGoblinText);
		hero_text = false;
		break;
	case G_ORCS:
		return RandomString(txAiOrcText);
	case G_ANIMALS:
		return RandomString(txAiWildHunterText);
	default:
		assert(0);
		hero_text = false;
		break;
	}

	if(hero_text)
	{
		if(game_level->location->type == L_CITY)
			return RandomString(txAiHeroCityText);
		else if(game_level->location->outside)
			return RandomString(txAiHeroOutsideText);
		else
			return RandomString(txAiHeroDungeonText);
	}
	else
	{
		n = Rand() % 100;
		if(n < 60)
			return RandomString(txAiDefaultText);
		else if(game_level->location->outside)
			return RandomString(txAiOutsideText);
		else
			return RandomString(txAiInsideText);
	}
}

void Game::OnEnterLevelOrLocation()
{
	game_gui->Clear(false, true);
	game_level->lights_dt = 1.f;
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
}
