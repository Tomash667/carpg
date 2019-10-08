#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Language.h"
#include "Terrain.h"
#include "Content.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "ServerPanel.h"
#include "PickServerPanel.h"
#include "Spell.h"
#include "Trap.h"
#include "QuestManager.h"
#include "Action.h"
#include "UnitGroup.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "SaveState.h"
#include "World.h"
#include "LocationGeneratorFactory.h"
#include "Pathfinding.h"
#include "Level.h"
#include "SuperShader.h"
#include "Arena.h"
#include "ResourceManager.h"
#include "Building.h"
#include "GameGui.h"
#include "SaveLoadPanel.h"
#include "DebugDrawer.h"
#include "Item.h"
#include "NameHelper.h"
#include "CommandParser.h"
#include "CreateServerPanel.h"
#include "Render.h"
#include "GrassShader.h"
#include "TerrainShader.h"
#include "UnitData.h"
#include "BaseUsable.h"
#include "Engine.h"
#include "Notifications.h"
#include "GameStats.h"
#include "Team.h"
#include "MeshInstance.h"
#include "GameResources.h"

extern void HumanPredraw(void* ptr, Matrix* mat, int n);
extern string g_system_dir;

//=================================================================================================
void Game::BeforeInit()
{
	render->SetShadersDir(Format("%s/shaders", g_system_dir.c_str()));

	arena = new Arena;
	cmdp = new CommandParser;
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
	res_mgr->SetProgressCallback(ProgressCallback(game_gui->load_screen, &LoadScreen::SetProgressCallback));
}

//=================================================================================================
// Load language strings showed on load screen.
//=================================================================================================
void Game::PreloadLanguage()
{
	Language::LoadFile("preload.txt");

	txCreatingListOfFiles = Str("creatingListOfFiles");
	txConfiguringGame = Str("configuringGame");
	txLoadingItems = Str("loadingItems");
	txLoadingObjects = Str("loadingObjects");
	txLoadingSpells = Str("loadingSpells");
	txLoadingClasses = Str("loadingClasses");
	txLoadingUnits = Str("loadingUnits");
	txLoadingMusics = Str("loadingMusics");
	txLoadingBuildings = Str("loadingBuildings");
	txLoadingRequires = Str("loadingRequires");
	txLoadingShaders = Str("loadingShaders");
	txLoadingDialogs = Str("loadingDialogs");
	txLoadingQuests = Str("loadingQuests");
	txLoadingLanguageFiles = Str("loadingLanguageFiles");
	txPreloadAssets = Str("preloadAssets");
}

//=================================================================================================
// Load data used by loadscreen.
//=================================================================================================
void Game::PreloadData()
{
	res_mgr->AddDir("data/preload");

	GameGui::font = game_gui->gui->CreateFont("Arial", 12, 800, 512, 2);

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
	game_gui->load_screen->Setup(0.f, 0.33f, 14, txCreatingListOfFiles);

	AddFilesystem();
	game_res->Init();
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
	LoadShaders();
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
		case Content::Id::Items:
			game_gui->load_screen->Tick(txLoadingItems);
			break;
		case Content::Id::Objects:
			game_gui->load_screen->Tick(txLoadingObjects);
			break;
		case Content::Id::Spells:
			game_gui->load_screen->Tick(txLoadingSpells);
			break;
		case Content::Id::Dialogs:
			game_gui->load_screen->Tick(txLoadingDialogs);
			break;
		case Content::Id::Units:
			game_gui->load_screen->Tick(txLoadingUnits);
			break;
		case Content::Id::Buildings:
			game_gui->load_screen->Tick(txLoadingBuildings);
			break;
		case Content::Id::Musics:
			game_gui->load_screen->Tick(txLoadingMusics);
			break;
		case Content::Id::Quests:
			game_gui->load_screen->Tick(txLoadingQuests);
			break;
		case Content::Id::Classes:
			game_gui->load_screen->Tick(txLoadingClasses);
			break;
		}
	});

	// required
	game_gui->load_screen->Tick(txLoadingRequires);
	LoadRequiredStats(load_errors);
}

//=================================================================================================
// Load language files.
//=================================================================================================
void Game::LoadLanguageFiles()
{
	Info("Game: Loading language files.");
	game_gui->load_screen->Tick(txLoadingLanguageFiles);

	Language::LoadLanguageFiles();

	SetGameCommonText();
	SetItemStatsText();
	NameHelper::SetHeroNames();
	SetGameText();
	SetStatsText();
	arena->LoadLanguage();
	game_gui->LoadLanguage();
	game_level->LoadLanguage();
	net->LoadLanguage();
	quest_mgr->LoadLanguage();
	world->LoadLanguage();

	txLoadGuiTextures = Str("loadGuiTextures");
	txLoadParticles = Str("loadParticles");
	txLoadPhysicMeshes = Str("loadPhysicMeshes");
	txLoadModels = Str("loadModels");
	txLoadSpells = Str("loadSpells");
	txLoadSounds = Str("loadSounds");
	txLoadMusic = Str("loadMusic");
	txGenerateWorld = Str("generateWorld");
	txHaveErrors = Str("haveErrors");

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

	CreateTextures();
	CreateRenderTargets();
}

//=================================================================================================
// Load game data.
//=================================================================================================
void Game::LoadData()
{
	Info("Game: Loading data.");

	res_mgr->PrepareLoadScreen(0.33f);
	AddLoadTasks();
	game_gui->LoadData();
	game_level->LoadData();
	net->LoadData();
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

	// copy first dungeon texture to second
	FIXME;
	/*tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;*/

	ItemScript::Init();

	// shaders
	debug_drawer = new DebugDrawer(render);
	grass_shader = new GrassShader(render);
	terrain_shader = new TerrainShader(render);
	debug_drawer->SetHandler(delegate<void(DebugDrawer*)>(this, &Game::OnDebugDraw));

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
		Texture* img = (load_errors > 0 ? tError : tWarning);
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
void Game::AddLoadTasks()
{
	game_gui->load_screen->Tick(txPreloadAssets);

	game_res->LoadData();

	bool nomusic = sound_mgr->IsMusicDisabled();

	game_res->LoadData();

	// gui textures
	res_mgr->AddTaskCategory(txLoadGuiTextures);
	tEquipped = res_mgr->Load<Texture>("equipped.png");
	tBlack = res_mgr->Load<Texture>("czern.bmp");
	tRip = res_mgr->Load<Texture>("rip.jpg");
	tPortal = res_mgr->Load<Texture>("dark_portal.png");
	tWarning = res_mgr->Load<Texture>("warning.png");
	tError = res_mgr->Load<Texture>("error.png");
	Action::LoadData();

	// preload terrain textures
	tGrass = res_mgr->Load<Texture>("trawa.jpg");
	tGrass2 = res_mgr->Load<Texture>("Grass0157_5_S.jpg");
	tGrass3 = res_mgr->Load<Texture>("LeavesDead0045_1_S.jpg");
	tRoad = res_mgr->Load<Texture>("droga.jpg");
	tFootpath = res_mgr->Load<Texture>("ziemia.jpg");
	tField = res_mgr->Load<Texture>("pole.jpg");
	FIXME;
	/*tFloorBase.diffuse = res_mgr->Load<Texture>("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = res_mgr->Load<Texture>("sciana.jpg");
	tWallBase.normal = res_mgr->Load<Texture>("sciana_nrm.jpg");
	tWallBase.specular = res_mgr->Load<Texture>("sciana_spec.jpg");
	tCeilBase.diffuse = res_mgr->Load<Texture>("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;*/
	BaseLocation::PreloadTextures();

	// particles
	res_mgr->AddTaskCategory(txLoadParticles);
	tBlood[BLOOD_RED] = res_mgr->Load<Texture>("krew.png");
	tBlood[BLOOD_GREEN] = res_mgr->Load<Texture>("krew2.png");
	tBlood[BLOOD_BLACK] = res_mgr->Load<Texture>("krew3.png");
	tBlood[BLOOD_BONE] = res_mgr->Load<Texture>("iskra.png");
	tBlood[BLOOD_ROCK] = res_mgr->Load<Texture>("kamien.png");
	tBlood[BLOOD_IRON] = res_mgr->Load<Texture>("iskra.png");
	tBloodSplat[BLOOD_RED] = res_mgr->Load<Texture>("krew_slad.png");
	tBloodSplat[BLOOD_GREEN] = res_mgr->Load<Texture>("krew_slad2.png");
	tBloodSplat[BLOOD_BLACK] = res_mgr->Load<Texture>("krew_slad3.png");
	tBloodSplat[BLOOD_BONE] = nullptr;
	tBloodSplat[BLOOD_ROCK] = nullptr;
	tBloodSplat[BLOOD_IRON] = nullptr;
	tSpark = res_mgr->Load<Texture>("iskra.png");
	tSpawn = res_mgr->Load<Texture>("spawn_fog.png");
	tLightingLine = res_mgr->Load<Texture>("lighting_line.png");

	// physic meshes
	res_mgr->AddTaskCategory(txLoadPhysicMeshes);
	vdStairsUp = res_mgr->Load<VertexData>("schody_gora.phy");
	vdStairsDown = res_mgr->Load<VertexData>("schody_dol.phy");
	vdDoorHole = res_mgr->Load<VertexData>("nadrzwi.phy");

	// models
	res_mgr->AddTaskCategory(txLoadModels);
	aBox = res_mgr->Load<Mesh>("box.qmsh");
	aCylinder = res_mgr->Load<Mesh>("cylinder.qmsh");
	aSphere = res_mgr->Load<Mesh>("sphere.qmsh");
	aCapsule = res_mgr->Load<Mesh>("capsule.qmsh");
	aArrow = res_mgr->Load<Mesh>("strzala.qmsh");
	aSkybox = res_mgr->Load<Mesh>("skybox.qmsh");
	aBag = res_mgr->Load<Mesh>("worek.qmsh");
	aChest = res_mgr->Load<Mesh>("skrzynia.qmsh");
	aGrating = res_mgr->Load<Mesh>("kratka.qmsh");
	aDoorWall = res_mgr->Load<Mesh>("nadrzwi.qmsh");
	aDoorWall2 = res_mgr->Load<Mesh>("nadrzwi2.qmsh");
	aStairsDown = res_mgr->Load<Mesh>("schody_dol.qmsh");
	aStairsDown2 = res_mgr->Load<Mesh>("schody_dol2.qmsh");
	aStairsUp = res_mgr->Load<Mesh>("schody_gora.qmsh");
	aSpellball = res_mgr->Load<Mesh>("spellball.qmsh");
	aDoor = res_mgr->Load<Mesh>("drzwi.qmsh");
	aDoor2 = res_mgr->Load<Mesh>("drzwi2.qmsh");
	aHair[0] = res_mgr->Load<Mesh>("hair1.qmsh");
	aHair[1] = res_mgr->Load<Mesh>("hair2.qmsh");
	aHair[2] = res_mgr->Load<Mesh>("hair3.qmsh");
	aHair[3] = res_mgr->Load<Mesh>("hair4.qmsh");
	aHair[4] = res_mgr->Load<Mesh>("hair5.qmsh");
	aEyebrows = res_mgr->Load<Mesh>("eyebrows.qmsh");
	aMustache[0] = res_mgr->Load<Mesh>("mustache1.qmsh");
	aMustache[1] = res_mgr->Load<Mesh>("mustache2.qmsh");
	aBeard[0] = res_mgr->Load<Mesh>("beard1.qmsh");
	aBeard[1] = res_mgr->Load<Mesh>("beard2.qmsh");
	aBeard[2] = res_mgr->Load<Mesh>("beard3.qmsh");
	aBeard[3] = res_mgr->Load<Mesh>("beard4.qmsh");
	aBeard[4] = res_mgr->Load<Mesh>("beardm1.qmsh");
	aStun = res_mgr->Load<Mesh>("stunned.qmsh");

	// preload buildings
	for(Building* b : Building::buildings)
	{
		if(b->mesh)
			res_mgr->LoadMeshMetadata(b->mesh);
		if(b->inside_mesh)
			res_mgr->LoadMeshMetadata(b->inside_mesh);
	}

	// preload traps
	for(uint i = 0; i < BaseTrap::n_traps; ++i)
	{
		BaseTrap& t = BaseTrap::traps[i];
		if(t.mesh_id)
		{
			t.mesh = res_mgr->Get<Mesh>(t.mesh_id);
			res_mgr->LoadMeshMetadata(t.mesh);

			Mesh::Point* pt = t.mesh->FindPoint("hitbox");
			assert(pt);
			if(pt->type == Mesh::Point::BOX)
			{
				t.rw = pt->size.x;
				t.h = pt->size.z;
			}
			else
				t.h = t.rw = pt->size.x;
		}
		if(t.mesh_id2)
			t.mesh2 = res_mgr->Get<Mesh>(t.mesh_id2);
		if(t.sound_id)
			t.sound = res_mgr->Get<Sound>(t.sound_id);
		if(t.sound_id2)
			t.sound2 = res_mgr->Get<Sound>(t.sound_id2);
		if(t.sound_id3)
			t.sound3 = res_mgr->Get<Sound>(t.sound_id3);
	}

	// spells
	res_mgr->AddTaskCategory(txLoadSpells);
	for(Spell* spell_ptr : Spell::spells)
	{
		Spell& spell = *spell_ptr;

		if(spell.sound_cast)
			res_mgr->Load(spell.sound_cast);
		if(spell.sound_hit)
			res_mgr->Load(spell.sound_hit);
		if(spell.tex)
			res_mgr->Load(spell.tex);
		if(spell.tex_particle)
			res_mgr->Load(spell.tex_particle);
		if(spell.tex_explode)
			res_mgr->Load(spell.tex_explode);
		if(spell.mesh)
			res_mgr->Load(spell.mesh);

		if(spell.type == Spell::Ball || spell.type == Spell::Point)
			spell.shape = new btSphereShape(spell.size);
	}

	// preload objects
	for(BaseObject* p_obj : BaseObject::objs)
	{
		BaseObject& obj = *p_obj;
		if(obj.variants)
			SetupObject(obj);
		else if(obj.mesh)
		{
			if(!IsSet(obj.flags, OBJ_SCALEABLE | OBJ_NO_PHYSICS) && obj.type == OBJ_CYLINDER)
				obj.shape = new btCylinderShape(btVector3(obj.r, obj.h, obj.r));
			SetupObject(obj);
		}
	}

	// preload items
	LoadItemsData();

	// sounds
	res_mgr->AddTaskCategory(txLoadSounds);
	sGulp = res_mgr->Load<Sound>("gulp.mp3");
	sCoins = res_mgr->Load<Sound>("moneta2.mp3");
	sBow[0] = res_mgr->Load<Sound>("bow1.mp3");
	sBow[1] = res_mgr->Load<Sound>("bow2.mp3");
	sDoor[0] = res_mgr->Load<Sound>("drzwi-02.mp3");
	sDoor[1] = res_mgr->Load<Sound>("drzwi-03.mp3");
	sDoor[2] = res_mgr->Load<Sound>("drzwi-04.mp3");
	sDoorClose = res_mgr->Load<Sound>("104528__skyumori__door-close-sqeuak-02.mp3");
	sDoorClosed[0] = res_mgr->Load<Sound>("wont_budge.mp3");
	sDoorClosed[1] = res_mgr->Load<Sound>("wont_budge2.mp3");
	sItem[0] = res_mgr->Load<Sound>("bottle.wav"); // potion
	sItem[1] = res_mgr->Load<Sound>("armor-light.wav"); // light armor
	sItem[2] = res_mgr->Load<Sound>("chainmail1.wav"); // heavy armor
	sItem[3] = res_mgr->Load<Sound>("metal-ringing.wav"); // crystal
	sItem[4] = res_mgr->Load<Sound>("wood-small.wav"); // bow
	sItem[5] = res_mgr->Load<Sound>("cloth-heavy.wav"); // shield
	sItem[6] = res_mgr->Load<Sound>("sword-unsheathe.wav"); // weapon
	sItem[7] = res_mgr->Load<Sound>("interface3.wav"); // other
	sItem[8] = res_mgr->Load<Sound>("amulet.mp3"); // amulet
	sItem[9] = res_mgr->Load<Sound>("ring.mp3"); // ring
	sChestOpen = res_mgr->Load<Sound>("chest_open.mp3");
	sChestClose = res_mgr->Load<Sound>("chest_close.mp3");
	sDoorBudge = res_mgr->Load<Sound>("door_budge.mp3");
	sRock = res_mgr->Load<Sound>("atak_kamien.mp3");
	sWood = res_mgr->Load<Sound>("atak_drewno.mp3");
	sCrystal = res_mgr->Load<Sound>("atak_krysztal.mp3");
	sMetal = res_mgr->Load<Sound>("atak_metal.mp3");
	sBody[0] = res_mgr->Load<Sound>("atak_cialo.mp3");
	sBody[1] = res_mgr->Load<Sound>("atak_cialo2.mp3");
	sBody[2] = res_mgr->Load<Sound>("atak_cialo3.mp3");
	sBody[3] = res_mgr->Load<Sound>("atak_cialo4.mp3");
	sBody[4] = res_mgr->Load<Sound>("atak_cialo5.mp3");
	sBone = res_mgr->Load<Sound>("atak_kosci.mp3");
	sSkin = res_mgr->Load<Sound>("atak_skora.mp3");
	sArenaFight = res_mgr->Load<Sound>("arena_fight.mp3");
	sArenaWin = res_mgr->Load<Sound>("arena_wygrana.mp3");
	sArenaLost = res_mgr->Load<Sound>("arena_porazka.mp3");
	sUnlock = res_mgr->Load<Sound>("unlock.mp3");
	sEvil = res_mgr->Load<Sound>("TouchofDeath.mp3");
	sEat = res_mgr->Load<Sound>("eat.mp3");
	sSummon = res_mgr->Load<Sound>("whooshy-puff.wav");
	sZap = res_mgr->Load<Sound>("zap.mp3");
	sCancel = res_mgr->Load<Sound>("cancel.mp3");

	// musics
	if(!nomusic)
		LoadMusic(MusicType::Title);
}

//=================================================================================================
void Game::SetupObject(BaseObject& obj)
{
	Mesh::Point* point;

	if(IsSet(obj.flags, OBJ_PRELOAD))
	{
		if(obj.variants)
		{
			for(Mesh* mesh : obj.variants->meshes)
				res_mgr->Load(mesh);
		}
		else if(obj.mesh)
			res_mgr->Load(obj.mesh);
	}

	if(obj.variants)
	{
		assert(!IsSet(obj.flags, OBJ_DOUBLE_PHYSICS | OBJ_MULTI_PHYSICS)); // not supported for variant mesh yet
		Mesh* mesh = obj.variants->meshes[0];
		res_mgr->LoadMeshMetadata(mesh);
		point = mesh->FindPoint("hit");
	}
	else
	{
		res_mgr->LoadMeshMetadata(obj.mesh);
		point = obj.mesh->FindPoint("hit");
	}

	if(!point || !point->IsBox() || IsSet(obj.flags, OBJ_BUILDING | OBJ_SCALEABLE) || obj.type == OBJ_CYLINDER)
	{
		obj.size = Vec2::Zero;
		obj.matrix = nullptr;
		return;
	}

	assert(point->size.x >= 0 && point->size.y >= 0 && point->size.z >= 0);
	obj.matrix = &point->mat;
	obj.size = point->size.XZ();

	if(!IsSet(obj.flags, OBJ_NO_PHYSICS))
		obj.shape = new btBoxShape(ToVector3(point->size));

	if(IsSet(obj.flags, OBJ_PHY_ROT))
		obj.type = OBJ_HITBOX_ROT;

	if(IsSet(obj.flags, OBJ_MULTI_PHYSICS))
	{
		LocalVector2<Mesh::Point*> points;
		Mesh::Point* prev_point = point;

		while(true)
		{
			Mesh::Point* new_point = obj.mesh->FindNextPoint("hit", prev_point);
			if(new_point)
			{
				assert(new_point->IsBox() && new_point->size.x >= 0 && new_point->size.y >= 0 && new_point->size.z >= 0);
				points.push_back(new_point);
				prev_point = new_point;
			}
			else
				break;
		}

		assert(points.size() > 1u);
		obj.next_obj = new BaseObject[points.size() + 1];
		for(uint i = 0, size = points.size(); i < size; ++i)
		{
			BaseObject& o2 = obj.next_obj[i];
			o2.shape = new btBoxShape(ToVector3(points[i]->size));
			if(IsSet(obj.flags, OBJ_PHY_BLOCKS_CAM))
				o2.flags = OBJ_PHY_BLOCKS_CAM;
			o2.matrix = &points[i]->mat;
			o2.size = points[i]->size.XZ();
			o2.type = obj.type;
		}
		obj.next_obj[points.size()].shape = nullptr;
	}
	else if(IsSet(obj.flags, OBJ_DOUBLE_PHYSICS))
	{
		Mesh::Point* point2 = obj.mesh->FindNextPoint("hit", point);
		if(point2 && point2->IsBox())
		{
			assert(point2->size.x >= 0 && point2->size.y >= 0 && point2->size.z >= 0);
			obj.next_obj = new BaseObject;
			if(!IsSet(obj.flags, OBJ_NO_PHYSICS))
			{
				btBoxShape* shape = new btBoxShape(ToVector3(point2->size));
				obj.next_obj->shape = shape;
				if(IsSet(obj.flags, OBJ_PHY_BLOCKS_CAM))
					obj.next_obj->flags = OBJ_PHY_BLOCKS_CAM;
			}
			else
				obj.next_obj->shape = nullptr;
			obj.next_obj->matrix = &point2->mat;
			obj.next_obj->size = point2->size.XZ();
			obj.next_obj->type = obj.type;
		}
	}
}

//=================================================================================================
void Game::LoadItemsData()
{
	// preload hardcoded items
	PreloadItem(Item::Get("beer"));
	PreloadItem(Item::Get("vodka"));
	PreloadItem(Item::Get("spirit"));
	PreloadItem(Item::Get("p_hp"));
	PreloadItem(Item::Get("p_hp2"));
	PreloadItem(Item::Get("p_hp3"));
	PreloadItem(Item::Get("gold"));
	ItemListResult list = ItemList::Get("normal_food");
	for(const Item* item : list.lis->items)
		PreloadItem(item);
	list = ItemList::Get("orc_food");
	for(const Item* item : list.lis->items)
		PreloadItem(item);
}
