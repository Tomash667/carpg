#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "DatatypeManager.h"
#include "Language.h"
#include "Terrain.h"

static cstring CATEGORY = "Game";
extern void HumanPredraw(void* ptr, MATRIX* mat, int n);
extern const int ITEM_IMAGE_SIZE;

//=================================================================================================
// Initialize game and show loadscreen.
//=================================================================================================
bool Game::InitGame()
{
	logger->Info(CATEGORY, "Initializing game.");

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

		logger->Info(CATEGORY, "Game initialized.");
		return true;
	}
	catch(cstring err)
	{
		ShowError(Format("Failed to initialize game: %s", err), CATEGORY, Logger::L_FATAL);
		return false;
	}
}

//=================================================================================================
// Preconfigure game vars.
//=================================================================================================
void Game::PreconfigureGame()
{
	logger->Info(CATEGORY, "Preconfiguring game.");

	// set default render states
	V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	V(device->SetRenderState(D3DRS_ALPHAREF, 200));
	V(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));
	r_alphablend = false;
	r_alphatest = false;
	r_nocull = false;
	r_nozwrite = false;

	// set sound vars
	if(!disabled_sound)
	{
		group_default->setVolume(float(sound_volume) / 100);
		group_music->setVolume(float(music_volume) / 100);
	}
	else
	{
		nosound = true;
		nomusic = true;
	}

	// set animesh callback
	AnimeshInstance::Predraw = HumanPredraw;

	PreinitGui();
	CreateVertexDeclarations();
	PreloadLanguage();
	PreloadData();
	CreatePlaceholderResources();
	resMgr.SetLoadScreen(load_screen);
}

//=================================================================================================
// Create placeholder resources (missing texture).
//=================================================================================================
void Game::CreatePlaceholderResources()
{
	load_errors = 0;

	// create item missing texture
	SURFACE surf;
	D3DLOCKED_RECT rect;

	V(device->CreateTexture(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &missing_texture, nullptr));
	V(missing_texture->GetSurfaceLevel(0, &surf));
	V(surf->LockRect(&rect, nullptr, 0));

	const DWORD col[2] = { COLOR_RGB(255, 0, 255), COLOR_RGB(0, 255, 0) };

	for(int y = 0; y<ITEM_IMAGE_SIZE; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)rect.pBits) + rect.Pitch*y);
		for(int x = 0; x<ITEM_IMAGE_SIZE; ++x)
		{
			*pix = col[(x >= ITEM_IMAGE_SIZE / 2 ? 1 : 0) + (y >= ITEM_IMAGE_SIZE / 2 ? 1 : 0) % 2];
			++pix;
		}
	}

	surf->UnlockRect();
	surf->Release();
}

//=================================================================================================
// Load language strings showed on load screen.
//=================================================================================================
void Game::PreloadLanguage()
{
	LoadLanguageFile("preload.txt");

	txCreateListOfFiles = Str("createListOfFiles");
	txLoadItemsDatafile = Str("loadItemsDatafile");
	txLoadUnitDatafile = Str("loadUnitDatafile");
	txLoadSpellDatafile = Str("loadSpellDatafile");
	txLoadMusicDatafile = Str("loadMusicDatafile");
	txLoadRequires = Str("loadRequires");
	txLoadLanguageFiles = Str("loadLanguageFiles");
	txLoadShaders = Str("loadShaders");
	txConfigureGame = Str("configureGame");
	txLoadDialogs = Str("loadDialogs");
	txLoadingDatafiles = Str("loadingDatafiles");
	txLoadingTextfiles = Str("loadingTextfiles");
}

//=================================================================================================
// Load data used by loadscreen.
//=================================================================================================
void Game::PreloadData()
{
	resMgr.AddDir("data/preload");

	// loadscreen textures
	load_screen->LoadData();

	// gui shader
	eGui = CompileShader("gui.fx");
	GUI.SetShader(eGui);

	// intro music
	if(!nomusic)
	{
		Music* music = new Music;
		music->music = resMgr.GetLoadedMusic("Intro.ogg");
		music->type = MusicType::Intro;
		musics.push_back(music);
		SetMusic(MusicType::Intro);
	}
}

//=================================================================================================
// Load system.
//=================================================================================================
void Game::LoadSystem()
{
	logger->Info(CATEGORY, "Loading system.");
	resMgr.PrepareLoadScreen2(0.1f, xxx, txCreateListOfFiles);

	AddFilesystem();
	LoadDatafiles();
	LoadLanguageFiles();
	resMgr.NextTask(txLoadShaders);
	LoadShaders();
	ConfigureGame();
	resMgr.EndLoadScreenStage();
	resMgr.StartLoadScreen();
}

//=================================================================================================
// Add filesystem.
//=================================================================================================
void Game::AddFilesystem()
{
	logger->Info(CATEGORY, "Creating list of files.");
	resMgr.AddDir("data");
	resMgr.AddPak("data/data.pak", "KrystaliceFire");
}

//=================================================================================================
// Load datafiles (items/units/etc).
//=================================================================================================
void Game::LoadDatafiles()
{
	logger->Info(CATEGORY, "Loading datafiles.");

	// items
	resMgr.NextTask(txLoadItemsDatafile);
	LoadItems(crc_items);
	logger->Info(CATEGORY, Format("Loaded items: %d (crc %p).", g_items.size(), crc_items));

	// spells
	resMgr.NextTask(txLoadSpellDatafile);
	LoadSpells(crc_spells);
	logger->Info(CATEGORY, Format("Loaded spells: %d (crc %p).", spells.size(), crc_spells));

	// dialogs
	resMgr.NextTask(txLoadDialogs);
	uint count = LoadDialogs(crc_dialogs);
	logger->Info(CATEGORY, Format("Loaded dialogs: %d (crc %p).", count, crc_dialogs));

	// units
	resMgr.NextTask(txLoadUnitDatafile);
	LoadUnits(crc_units);
	logger->Info(CATEGORY, Format("Loaded units: %d (crc %p).", unit_datas.size(), crc_units));

	// musics
	resMgr.NextTask(txLoadMusicDatafile);
	LoadMusicDatafile();

	// datatypes
	resMgr.NextTask(txLoadingDatafiles);
	dt_mgr->LoadDatatypesFromText();
	dt_mgr->LogLoadedDatatypes(CATEGORY);

	// required
	resMgr.NextTask(txLoadRequires);
	LoadRequiredStats();
}

//=================================================================================================
// Load language files.
//=================================================================================================
void Game::LoadLanguageFiles()
{
	logger->Info(CATEGORY, "Loading language files.");
	resMgr.NextTask(txLoadLanguageFiles);

	LoadLanguageFile("menu.txt");
	LoadLanguageFile("stats.txt");
	::LoadLanguageFiles();
	LoadDialogTexts();

	resMgr.NextTask(txLoadingTextfiles);
	dt_mgr->LoadStringsFromText();

	GUI.SetText();
	SetGameCommonText();
	SetItemStatsText();
	SetLocationNames();
	SetHeroNames();
	SetGameText();
	SetStatsText();

	txLoadGuiTextures = Str("loadGuiTextures");
	txLoadTerrainTextures = Str("loadTerrainTextures");
	txLoadParticles = Str("loadParticles");
	txLoadPhysicMeshes = Str("loadPhysicMeshes");
	txLoadModels = Str("loadModels");
	txLoadBuildings = Str("loadBuildings");
	txLoadTraps = Str("loadTraps");
	txLoadSpells = Str("loadSpells");
	txLoadObjects = Str("loadObjects");
	txLoadUnits = Str("loadUnits");
	txLoadItems = Str("loadItems");
	txLoadSounds = Str("loadSounds");
	txLoadMusic = Str("loadMusic");
	txGenerateWorld = Str("generateWorld");
	txInitQuests = Str("initQuests");
}

//=================================================================================================
// Initialize everything needed by game before loading content.
//=================================================================================================
void Game::ConfigureGame()
{
	logger->Info(CATEGORY, "Configuring game.");
	resMgr.NextTask(txConfigureGame);

	InitScene();
	InitSuperShader();
	AddCommands();
	InitGui();
	ResetGameKeys();
	LoadGameKeys();
	SetMeshSpecular();
	LoadSaveSlots();
	SetRoomPointers();

	for(int i = 0; i<SG_MAX; ++i)
	{
		if(g_spawn_groups[i].unit_group_id[0] == 0)
			g_spawn_groups[i].unit_group = nullptr;
		else
			g_spawn_groups[i].unit_group = FindUnitGroup(g_spawn_groups[i].unit_group_id);
	}

	for(ClassInfo& ci : g_classes)
		ci.unit_data = FindUnitData(ci.unit_data_id, false);

	CreateTextures();
}

//=================================================================================================
// Load game data.
//=================================================================================================
void Game::LoadData()
{
	logger->Info(CATEGORY, "Loading data.");

	resMgr.SetMutex(mutex);
	mutex = nullptr;
	resMgr.PrepareLoadScreen();
	AddLoadTasks();
	resMgr.StartLoadScreen();
}

//=================================================================================================
// Configure game after loading content.
//=================================================================================================
void Game::PostconfigureGame()
{
	logger->Info(CATEGORY, "Postconfiguring game.");

	CreateCollisionShapes();
	create_character->Init();

	// init terrain
	terrain = new Terrain;
	TerrainOptions terrain_options;
	terrain_options.n_parts = 8;
	terrain_options.tex_size = 256;
	terrain_options.tile_size = 2.f;
	terrain_options.tiles_per_part = 16;
	terrain->Init(device, terrain_options);
	TEX tex[5] = { tTrawa, tTrawa2, tTrawa3, tZiemia, tDroga };
	terrain->SetTextures(tex);
	terrain->Build();
	terrain->RemoveHeightMap(true);

	// get pointer to gold item
	gold_item_ptr = FindItem("gold");

	// copy first dungeon texture to second
	tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;

	// test & validate game data (in debug always check some things)
	if(testing)
	{
		TestGameData(true);
		ValidateGameData(true);
	}
#ifdef _DEBUG
	else
	{
		TestGameData(false);
		ValidateGameData(false);
	}
#endif

	// show errors notification
	if(load_errors > 0)
		GUI.AddNotification(Format("%d game system loading errors, check logs for details.", load_errors), tWarning, 5.f);
	
	// save config
	cfg.Add("adapter", Format("%d", used_adapter));
	cfg.Add("resolution", Format("%dx%d", wnd_size.x, wnd_size.y));
	cfg.Add("refresh", Format("%d", wnd_hz));
	SaveCfg();

	// end load screen, show menu
	clear_color = BLACK;
	game_state = GS_MAIN_MENU;
	game_state = GS_MAIN_MENU;
	load_screen->visible = false;
	main_menu->visible = true;
	if(music_type != MusicType::Intro)
		SetMusic(MusicType::Title);

	// start game mode if selected quickmode
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
		if(!player_name.empty())
		{
			if(!server_name.empty())
			{
				try
				{
					InitServer();
				}
				catch(cstring err)
				{
					GUI.SimpleDialog(err, nullptr);
					break;
				}

				server_panel->Show();
				Net_OnNewGameServer();
				UpdateServerInfo();

				if(change_title_a)
					ChangeTitle();
			}
			else
				WARN("Quickstart: Can't create server, no server name.");
		}
		else
			WARN("Quickstart: Can't create server, no player nick.");
		break;
	case QUICKSTART_JOIN_LAN:
		if(!player_name.empty())
		{
			pick_autojoin = true;
			pick_server_panel->Show();
		}
		else
			WARN("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_JOIN_IP:
		if(!player_name.empty())
		{
			if(!server_ip.empty())
				QuickJoinIp();
			else
				WARN("Quickstart: Can't join server, no server ip.");
		}
		else
			WARN("Quickstart: Can't join server, no player nick.");
		break;
	default:
		assert(0);
		break;
	}
}
