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
#include "GlobalGui.h"
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

extern void HumanPredraw(void* ptr, Matrix* mat, int n);
extern const int ITEM_IMAGE_SIZE;
extern string g_system_dir;

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
	render = engine->GetRender();
	render->SetShadersDir(Format("%s/shaders", g_system_dir.c_str()));
	sound_mgr = engine->GetSoundManager();

	engine->UnlockCursor(false);
	engine->cam_base = &L.camera;

	// set animesh callback
	MeshInstance::Predraw = HumanPredraw;

	// game components
	pathfinding = new Pathfinding;
	global::pathfinding = pathfinding;
	arena = new Arena;
	loc_gen_factory = new LocationGeneratorFactory;
	gui = new GlobalGui;
	global::gui = gui;
	global::cmdp = new CommandParser;
	components.push_back(&W);
	components.push_back(pathfinding);
	components.push_back(&QM);
	components.push_back(arena);
	components.push_back(loc_gen_factory);
	components.push_back(gui);
	components.push_back(&L);
	components.push_back(&SM);
	components.push_back(global::cmdp);
	components.push_back(&N);
	for(GameComponent* component : components)
		component->Prepare();

	CreateVertexDeclarations();
	PreloadLanguage();
	PreloadData();
	CreatePlaceholderResources();
	ResourceManager::Get().SetProgressCallback(ProgressCallback(gui->load_screen, &LoadScreen::SetProgressCallback));
}

//=================================================================================================
// Create placeholder resources (missing texture).
//=================================================================================================
void Game::CreatePlaceholderResources()
{
	missing_texture = render->CreateTexture(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));
	TextureLock lock(missing_texture);
	const uint col[2] = { Color(255, 0, 255), Color(0, 255, 0) };
	for(int y = 0; y < ITEM_IMAGE_SIZE; ++y)
	{
		uint* pix = lock[y];
		for(int x = 0; x < ITEM_IMAGE_SIZE; ++x)
		{
			*pix = col[(x >= ITEM_IMAGE_SIZE / 2 ? 1 : 0) + (y >= ITEM_IMAGE_SIZE / 2 ? 1 : 0) % 2];
			++pix;
		}
	}
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
	ResourceManager::Get().AddDir("data/preload");

	// loadscreen textures
	gui->load_screen->LoadData();

	// intro music
	if(!sound_mgr->IsMusicDisabled())
	{
		Ptr<Music> music;
		music->music = ResourceManager::Get<Sound>().GetLoadedMusic("Intro.ogg");
		music->type = MusicType::Intro;
		Music::musics.push_back(music.Pin());
		SetMusic(MusicType::Intro);
	}
}

//=================================================================================================
// Load system.
//=================================================================================================
void Game::LoadSystem()
{
	Info("Game: Loading system.");
	gui->load_screen->Setup(0.f, 0.33f, 14, txCreatingListOfFiles);

	for(GameComponent* component : components)
		component->InitOnce();
	gui->main_menu->UpdateCheckVersion();
	AddFilesystem();
	LoadDatafiles();
	LoadLanguageFiles();
	gui->load_screen->Tick(txLoadingShaders);
	LoadShaders();
	ConfigureGame();
}

//=================================================================================================
// Add filesystem.
//=================================================================================================
void Game::AddFilesystem()
{
	Info("Game: Creating list of files.");
	auto& res_mgr = ResourceManager::Get();
	res_mgr.AddDir("data");
	res_mgr.AddPak("data/data.pak", "KrystaliceFire");
}

//=================================================================================================
// Load datafiles (items/units/etc).
//=================================================================================================
void Game::LoadDatafiles()
{
	Info("Game: Loading system.");
	auto& res_mgr = ResourceManager::Get();
	load_errors = 0;
	load_warnings = 0;

	// content
	content.system_dir = g_system_dir;
	content.LoadContent([this, &res_mgr](Content::Id id)
	{
		switch(id)
		{
		case Content::Id::Items:
			gui->load_screen->Tick(txLoadingItems);
			break;
		case Content::Id::Objects:
			gui->load_screen->Tick(txLoadingObjects);
			break;
		case Content::Id::Spells:
			gui->load_screen->Tick(txLoadingSpells);
			break;
		case Content::Id::Dialogs:
			gui->load_screen->Tick(txLoadingDialogs);
			break;
		case Content::Id::Units:
			gui->load_screen->Tick(txLoadingUnits);
			break;
		case Content::Id::Buildings:
			gui->load_screen->Tick(txLoadingBuildings);
			break;
		case Content::Id::Musics:
			gui->load_screen->Tick(txLoadingMusics);
			break;
		case Content::Id::Quests:
			gui->load_screen->Tick(txLoadingQuests);
			break;
		}
	});

	// required
	gui->load_screen->Tick(txLoadingRequires);
	LoadRequiredStats(load_errors);
}

//=================================================================================================
// Load language files.
//=================================================================================================
void Game::LoadLanguageFiles()
{
	Info("Game: Loading language files.");
	gui->load_screen->Tick(txLoadingLanguageFiles);

	Language::LoadLanguageFiles();

	SetGameCommonText();
	SetItemStatsText();
	NameHelper::SetHeroNames();
	SetGameText();
	SetStatsText();
	N.LoadLanguage();

	txLoadGuiTextures = Str("loadGuiTextures");
	txLoadParticles = Str("loadParticles");
	txLoadPhysicMeshes = Str("loadPhysicMeshes");
	txLoadModels = Str("loadModels");
	txLoadSpells = Str("loadSpells");
	txLoadSounds = Str("loadSounds");
	txLoadMusic = Str("loadMusic");
	txGenerateWorld = Str("generateWorld");
	txHaveErrors = Str("haveErrors");

	for(GameComponent* component : components)
		component->LoadLanguage();
}

//=================================================================================================
// Initialize everything needed by game before loading content.
//=================================================================================================
void Game::ConfigureGame()
{
	Info("Game: Configuring game.");
	gui->load_screen->Tick(txConfiguringGame);

	InitScene();
	global::cmdp->AddCommands();
	settings.ResetGameKeys();
	settings.LoadGameKeys(cfg);
	load_errors += BaseLocation::SetRoomPointers();

	for (ClassInfo& ci : ClassInfo::classes)
	{
		ci.unit_data = UnitData::TryGet(ci.unit_data_id);
		if (ci.action_id)
			ci.action = Action::Find(string(ci.action_id));
	}

	CreateTextures();
	CreateRenderTargets();
}

//=================================================================================================
// Load game data.
//=================================================================================================
void Game::LoadData()
{
	Info("Game: Loading data.");
	auto& res_mgr = ResourceManager::Get();

	res_mgr.PrepareLoadScreen(0.33f);
	AddLoadTasks();
	for(GameComponent* component : components)
		component->LoadData();
	res_mgr.StartLoadScreen();
}

//=================================================================================================
// Configure game after loading content.
//=================================================================================================
void Game::PostconfigureGame()
{
	Info("Game: Postconfiguring game.");

	engine->LockCursor();
	for(GameComponent* component : components)
		component->PostInit();

	// create save folders
	io::CreateDirectory("saves");
	io::CreateDirectory("saves/single");
	io::CreateDirectory("saves/multi");

	// copy first dungeon texture to second
	tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;

	ItemScript::Init();

	// shaders
	debug_drawer.reset(new DebugDrawer(render));
	grass_shader.reset(new GrassShader(render));
	super_shader.reset(new SuperShader(render));
	terrain_shader.reset(new TerrainShader(render));
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
		TEX img = (load_errors > 0 ? tError : tWarning);
		cstring text = Format(txHaveErrors, load_errors, load_warnings);
#ifdef _DEBUG
		GUI.AddNotification(text, img, 5.f);
#else
		DialogInfo info;
		info.name = "have_errors";
		info.text = text;
		info.type = DIALOG_OK;
		info.img = img;
		info.event = [this](int result) { StartGameMode(); };
		info.parent = gui->main_menu;
		info.order = ORDER_TOPMOST;
		info.pause = false;
		info.auto_wrap = true;
		GUI.ShowDialog(info);
		start_game_mode = false;
#endif
	}

	// save config
	cfg.Add("adapter", render->GetAdapter());
	cfg.Add("resolution", Format("%dx%d", engine->GetWindowSize().x, engine->GetWindowSize().y));
	cfg.Add("refresh", render->GetRefreshRate());
	SaveCfg();

	// end load screen, show menu
	clear_color = Color::Black;
	game_state = GS_MAIN_MENU;
	gui->load_screen->visible = false;
	gui->main_menu->visible = true;
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
		if(N.server_name.empty())
		{
			Warn("Quickstart: Can't create server, no server name.");
			break;
		}

		if(quickstart == QUICKSTART_LOAD_MP)
		{
			N.mp_load = true;
			N.mp_quickload = false;
			if(TryLoadGame(quickstart_slot, false, false))
			{
				gui->create_server->CloseDialog();
				gui->server->autoready = true;
			}
			else
			{
				Error("Multiplayer quickload failed.");
				break;
			}
		}

		try
		{
			N.InitServer();
		}
		catch(cstring err)
		{
			GUI.SimpleDialog(err, nullptr);
			break;
		}

		Net_OnNewGameServer();
		break;
	case QUICKSTART_JOIN_LAN:
		if(!player_name.empty())
		{
			gui->server->autoready = true;
			gui->pick_server->Show(true);
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_JOIN_IP:
		if(!player_name.empty())
		{
			if(!server_ip.empty())
			{
				gui->server->autoready = true;
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
	gui->load_screen->Tick(txPreloadAssets);

	auto& res_mgr = ResourceManager::Get();
	auto& tex_mgr = ResourceManager::Get<Texture>();
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	auto& vd_mgr = ResourceManager::Get<VertexData>();
	auto& sound_mgr = ResourceManager::Get<Sound>();
	bool nomusic = this->sound_mgr->IsMusicDisabled();

	// gui textures
	res_mgr.AddTaskCategory(txLoadGuiTextures);
	tex_mgr.AddLoadTask("equipped.png", tEquipped);
	tex_mgr.AddLoadTask("czern.bmp", tCzern);
	tex_mgr.AddLoadTask("rip.jpg", tRip);
	tex_mgr.AddLoadTask("dark_portal.png", tPortal);
	for(ClassInfo& ci : ClassInfo::classes)
		tex_mgr.AddLoadTask(ci.icon_file, ci.icon);
	tex_mgr.AddLoadTask("warning.png", tWarning);
	tex_mgr.AddLoadTask("error.png", tError);
	Action::LoadData();

	// preload terrain textures
	tTrawa = tex_mgr.Get("trawa.jpg");
	tTrawa2 = tex_mgr.Get("Grass0157_5_S.jpg");
	tTrawa3 = tex_mgr.Get("LeavesDead0045_1_S.jpg");
	tDroga = tex_mgr.Get("droga.jpg");
	tZiemia = tex_mgr.Get("ziemia.jpg");
	tPole = tex_mgr.Get("pole.jpg");
	tFloorBase.diffuse = tex_mgr.Get("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = tex_mgr.Get("sciana.jpg");
	tWallBase.normal = tex_mgr.Get("sciana_nrm.jpg");
	tWallBase.specular = tex_mgr.Get("sciana_spec.jpg");
	tCeilBase.diffuse = tex_mgr.Get("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;
	BaseLocation::PreloadTextures();

	// particles
	res_mgr.AddTaskCategory(txLoadParticles);
	tKrew[BLOOD_RED] = tex_mgr.AddLoadTask("krew.png");
	tKrew[BLOOD_GREEN] = tex_mgr.AddLoadTask("krew2.png");
	tKrew[BLOOD_BLACK] = tex_mgr.AddLoadTask("krew3.png");
	tKrew[BLOOD_BONE] = tex_mgr.AddLoadTask("iskra.png");
	tKrew[BLOOD_ROCK] = tex_mgr.AddLoadTask("kamien.png");
	tKrew[BLOOD_IRON] = tex_mgr.AddLoadTask("iskra.png");
	tKrewSlad[BLOOD_RED] = tex_mgr.AddLoadTask("krew_slad.png");
	tKrewSlad[BLOOD_GREEN] = tex_mgr.AddLoadTask("krew_slad2.png");
	tKrewSlad[BLOOD_BLACK] = tex_mgr.AddLoadTask("krew_slad3.png");
	tKrewSlad[BLOOD_BONE] = nullptr;
	tKrewSlad[BLOOD_ROCK] = nullptr;
	tKrewSlad[BLOOD_IRON] = nullptr;
	tIskra = tex_mgr.AddLoadTask("iskra.png");
	tSpawn = tex_mgr.AddLoadTask("spawn_fog.png");
	tex_mgr.AddLoadTask("lighting_line.png", tLightingLine);

	// physic meshes
	res_mgr.AddTaskCategory(txLoadPhysicMeshes);
	vdStairsUp = vd_mgr.AddLoadTask("schody_gora.phy");
	vdStairsDown = vd_mgr.AddLoadTask("schody_dol.phy");
	vdDoorHole = vd_mgr.AddLoadTask("nadrzwi.phy");

	// models
	res_mgr.AddTaskCategory(txLoadModels);
	aBox = mesh_mgr.AddLoadTask("box.qmsh");
	aCylinder = mesh_mgr.AddLoadTask("cylinder.qmsh");
	aSphere = mesh_mgr.AddLoadTask("sphere.qmsh");
	aCapsule = mesh_mgr.AddLoadTask("capsule.qmsh");
	aArrow = mesh_mgr.AddLoadTask("strzala.qmsh");
	aSkybox = mesh_mgr.AddLoadTask("skybox.qmsh");
	aBag = mesh_mgr.AddLoadTask("worek.qmsh");
	aChest = mesh_mgr.AddLoadTask("skrzynia.qmsh");
	aGrating = mesh_mgr.AddLoadTask("kratka.qmsh");
	aDoorWall = mesh_mgr.AddLoadTask("nadrzwi.qmsh");
	aDoorWall2 = mesh_mgr.AddLoadTask("nadrzwi2.qmsh");
	aStairsDown = mesh_mgr.AddLoadTask("schody_dol.qmsh");
	aStairsDown2 = mesh_mgr.AddLoadTask("schody_dol2.qmsh");
	aStairsUp = mesh_mgr.AddLoadTask("schody_gora.qmsh");
	aSpellball = mesh_mgr.AddLoadTask("spellball.qmsh");
	aDoor = mesh_mgr.AddLoadTask("drzwi.qmsh");
	aDoor2 = mesh_mgr.AddLoadTask("drzwi2.qmsh");
	aHumanBase = mesh_mgr.AddLoadTask("human.qmsh");
	aHair[0] = mesh_mgr.AddLoadTask("hair1.qmsh");
	aHair[1] = mesh_mgr.AddLoadTask("hair2.qmsh");
	aHair[2] = mesh_mgr.AddLoadTask("hair3.qmsh");
	aHair[3] = mesh_mgr.AddLoadTask("hair4.qmsh");
	aHair[4] = mesh_mgr.AddLoadTask("hair5.qmsh");
	aEyebrows = mesh_mgr.AddLoadTask("eyebrows.qmsh");
	aMustache[0] = mesh_mgr.AddLoadTask("mustache1.qmsh");
	aMustache[1] = mesh_mgr.AddLoadTask("mustache2.qmsh");
	aBeard[0] = mesh_mgr.AddLoadTask("beard1.qmsh");
	aBeard[1] = mesh_mgr.AddLoadTask("beard2.qmsh");
	aBeard[2] = mesh_mgr.AddLoadTask("beard3.qmsh");
	aBeard[3] = mesh_mgr.AddLoadTask("beard4.qmsh");
	aBeard[4] = mesh_mgr.AddLoadTask("beardm1.qmsh");
	aStun = mesh_mgr.AddLoadTask("stunned.qmsh");

	// preload buildings
	for(Building* b : Building::buildings)
	{
		if(!b->mesh_id.empty())
		{
			b->mesh = mesh_mgr.Get(b->mesh_id);
			mesh_mgr.LoadMetadata(b->mesh);
		}
		if(!b->inside_mesh_id.empty())
		{
			b->inside_mesh = mesh_mgr.Get(b->inside_mesh_id);
			mesh_mgr.LoadMetadata(b->inside_mesh);
		}
	}

	// preload traps
	for(uint i = 0; i < BaseTrap::n_traps; ++i)
	{
		BaseTrap& t = BaseTrap::traps[i];
		if(t.mesh_id)
		{
			t.mesh = mesh_mgr.Get(t.mesh_id);
			mesh_mgr.LoadMetadata(t.mesh);

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
			t.mesh2 = mesh_mgr.Get(t.mesh_id2);
		if(t.sound_id)
			t.sound = sound_mgr.Get(t.sound_id);
		if(t.sound_id2)
			t.sound2 = sound_mgr.Get(t.sound_id2);
		if(t.sound_id3)
			t.sound3 = sound_mgr.Get(t.sound_id3);
	}

	// spells
	res_mgr.AddTaskCategory(txLoadSpells);
	for(Spell* spell_ptr : Spell::spells)
	{
		Spell& spell = *spell_ptr;

		if(!spell.sound_cast_id.empty())
			spell.sound_cast = sound_mgr.AddLoadTask(spell.sound_cast_id);
		if(!spell.sound_hit_id.empty())
			spell.sound_hit = sound_mgr.AddLoadTask(spell.sound_hit_id);
		if(!spell.tex_id.empty())
			spell.tex = tex_mgr.AddLoadTask(spell.tex_id);
		if(!spell.tex_particle_id.empty())
			spell.tex_particle = tex_mgr.AddLoadTask(spell.tex_particle_id);
		if(!spell.tex_explode_id.empty())
			spell.tex_explode = tex_mgr.AddLoadTask(spell.tex_explode_id);
		if(!spell.mesh_id.empty())
			spell.mesh = mesh_mgr.AddLoadTask(spell.mesh_id);

		if(spell.type == Spell::Ball || spell.type == Spell::Point)
			spell.shape = new btSphereShape(spell.size);
	}

	// preload objects
	for(BaseObject* p_obj : BaseObject::objs)
	{
		auto& obj = *p_obj;
		if(obj.variants)
		{
			VariantObject& vo = *obj.variants;
			if(!vo.loaded)
			{
				for(uint i = 0; i < vo.entries.size(); ++i)
					vo.entries[i].mesh = mesh_mgr.Get(vo.entries[i].mesh_id);
				vo.loaded = true;
			}
			SetupObject(obj);
		}
		else if(!obj.mesh_id.empty())
		{
			obj.mesh = mesh_mgr.Get(obj.mesh_id);
			if(!IS_SET(obj.flags, OBJ_SCALEABLE | OBJ_NO_PHYSICS) && obj.type == OBJ_CYLINDER)
				obj.shape = new btCylinderShape(btVector3(obj.r, obj.h, obj.r));
			SetupObject(obj);
		}
		else
		{
			obj.mesh = nullptr;
			obj.matrix = nullptr;
		}

		if(obj.IsUsable())
		{
			BaseUsable& bu = *(BaseUsable*)p_obj;
			if(!bu.sound_id.empty())
				bu.sound = sound_mgr.Get(bu.sound_id);
			if(!bu.item_id.empty())
				bu.item = Item::Get(bu.item_id);
		}
	}

	// preload units
	for(UnitData* ud_ptr : UnitData::units)
	{
		UnitData& ud = *ud_ptr;

		// model
		if(!ud.mesh_id.empty())
			ud.mesh = mesh_mgr.Get(ud.mesh_id);
		else
			ud.mesh = aHumanBase;

		// textures
		if(ud.tex && !ud.tex->inited)
		{
			ud.tex->inited = true;
			for(TexId& ti : ud.tex->textures)
			{
				if(!ti.id.empty())
					ti.tex = tex_mgr.Get(ti.id);
			}
		}
	}

	// preload items
	LoadItemsData();

	// sounds
	res_mgr.AddTaskCategory(txLoadSounds);
	sGulp = sound_mgr.AddLoadTask("gulp.mp3");
	sCoins = sound_mgr.AddLoadTask("moneta2.mp3");
	sBow[0] = sound_mgr.AddLoadTask("bow1.mp3");
	sBow[1] = sound_mgr.AddLoadTask("bow2.mp3");
	sDoor[0] = sound_mgr.AddLoadTask("drzwi-02.mp3");
	sDoor[1] = sound_mgr.AddLoadTask("drzwi-03.mp3");
	sDoor[2] = sound_mgr.AddLoadTask("drzwi-04.mp3");
	sDoorClose = sound_mgr.AddLoadTask("104528__skyumori__door-close-sqeuak-02.mp3");
	sDoorClosed[0] = sound_mgr.AddLoadTask("wont_budge.ogg");
	sDoorClosed[1] = sound_mgr.AddLoadTask("wont_budge2.ogg");
	sItem[0] = sound_mgr.AddLoadTask("bottle.wav"); // potion
	sItem[1] = sound_mgr.AddLoadTask("armor-light.wav"); // light armor
	sItem[2] = sound_mgr.AddLoadTask("chainmail1.wav"); // heavy armor
	sItem[3] = sound_mgr.AddLoadTask("metal-ringing.wav"); // crystal
	sItem[4] = sound_mgr.AddLoadTask("wood-small.wav"); // bow
	sItem[5] = sound_mgr.AddLoadTask("cloth-heavy.wav"); // shield
	sItem[6] = sound_mgr.AddLoadTask("sword-unsheathe.wav"); // weapon
	sItem[7] = sound_mgr.AddLoadTask("interface3.wav"); // other
	sItem[8] = sound_mgr.AddLoadTask("amulet.mp3"); // amulet
	sItem[9] = sound_mgr.AddLoadTask("ring.mp3"); // ring
	sChestOpen = sound_mgr.AddLoadTask("chest_open.mp3");
	sChestClose = sound_mgr.AddLoadTask("chest_close.mp3");
	sDoorBudge = sound_mgr.AddLoadTask("door_budge.mp3");
	sRock = sound_mgr.AddLoadTask("atak_kamien.mp3");
	sWood = sound_mgr.AddLoadTask("atak_drewno.mp3");
	sCrystal = sound_mgr.AddLoadTask("atak_krysztal.mp3");
	sMetal = sound_mgr.AddLoadTask("atak_metal.mp3");
	sBody[0] = sound_mgr.AddLoadTask("atak_cialo.mp3");
	sBody[1] = sound_mgr.AddLoadTask("atak_cialo2.mp3");
	sBody[2] = sound_mgr.AddLoadTask("atak_cialo3.mp3");
	sBody[3] = sound_mgr.AddLoadTask("atak_cialo4.mp3");
	sBody[4] = sound_mgr.AddLoadTask("atak_cialo5.mp3");
	sBone = sound_mgr.AddLoadTask("atak_kosci.mp3");
	sSkin = sound_mgr.AddLoadTask("atak_skora.mp3");
	sArenaFight = sound_mgr.AddLoadTask("arena_fight.mp3");
	sArenaWin = sound_mgr.AddLoadTask("arena_wygrana.mp3");
	sArenaLost = sound_mgr.AddLoadTask("arena_porazka.mp3");
	sUnlock = sound_mgr.AddLoadTask("unlock.mp3");
	sEvil = sound_mgr.AddLoadTask("TouchofDeath.ogg");
	sEat = sound_mgr.AddLoadTask("eat.mp3");
	sSummon = sound_mgr.AddLoadTask("whooshy-puff.wav");
	sZap = sound_mgr.AddLoadTask("zap.mp3");

	// musics
	if(!nomusic)
		LoadMusic(MusicType::Title);
}

//=================================================================================================
void Game::SetupObject(BaseObject& obj)
{
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	Mesh::Point* point;

	if(IS_SET(obj.flags, OBJ_PRELOAD))
	{
		if(obj.variants)
		{
			VariantObject& vo = *obj.variants;
			for(uint i = 0; i < vo.entries.size(); ++i)
				mesh_mgr.Load(vo.entries[i].mesh);
		}
		else if(!obj.mesh_id.empty())
			mesh_mgr.Load(obj.mesh);
	}

	if(obj.variants)
	{
		assert(!IS_SET(obj.flags, OBJ_DOUBLE_PHYSICS | OBJ_MULTI_PHYSICS)); // not supported for variant mesh yet
		mesh_mgr.LoadMetadata(obj.variants->entries[0].mesh);
		point = obj.variants->entries[0].mesh->FindPoint("hit");
	}
	else
	{
		mesh_mgr.LoadMetadata(obj.mesh);
		point = obj.mesh->FindPoint("hit");
	}

	if(!point || !point->IsBox() || IS_SET(obj.flags, OBJ_BUILDING | OBJ_SCALEABLE) || obj.type == OBJ_CYLINDER)
	{
		obj.size = Vec2::Zero;
		obj.matrix = nullptr;
		return;
	}

	assert(point->size.x >= 0 && point->size.y >= 0 && point->size.z >= 0);
	obj.matrix = &point->mat;
	obj.size = point->size.XZ();

	if(!IS_SET(obj.flags, OBJ_NO_PHYSICS))
		obj.shape = new btBoxShape(ToVector3(point->size));

	if(IS_SET(obj.flags, OBJ_PHY_ROT))
		obj.type = OBJ_HITBOX_ROT;

	if(IS_SET(obj.flags, OBJ_MULTI_PHYSICS))
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
			if(IS_SET(obj.flags, OBJ_PHY_BLOCKS_CAM))
				o2.flags = OBJ_PHY_BLOCKS_CAM;
			o2.matrix = &points[i]->mat;
			o2.size = points[i]->size.XZ();
			o2.type = obj.type;
		}
		obj.next_obj[points.size()].shape = nullptr;
	}
	else if(IS_SET(obj.flags, OBJ_DOUBLE_PHYSICS))
	{
		Mesh::Point* point2 = obj.mesh->FindNextPoint("hit", point);
		if(point2 && point2->IsBox())
		{
			assert(point2->size.x >= 0 && point2->size.y >= 0 && point2->size.z >= 0);
			obj.next_obj = new BaseObject;
			if(!IS_SET(obj.flags, OBJ_NO_PHYSICS))
			{
				btBoxShape* shape = new btBoxShape(ToVector3(point2->size));
				obj.next_obj->shape = shape;
				if(IS_SET(obj.flags, OBJ_PHY_BLOCKS_CAM))
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
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	auto& tex_mgr = ResourceManager::Get<Texture>();

	for(Armor* armor : Armor::armors)
	{
		Armor& a = *armor;
		if(!a.tex_override.empty())
		{
			for(TexId& ti : a.tex_override)
			{
				if(!ti.id.empty())
					ti.tex = tex_mgr.Get(ti.id);
			}
		}
	}

	for(auto it : Item::items)
	{
		Item& item = *it.second;

		if(IS_SET(item.flags, ITEM_TEX_ONLY))
		{
			item.tex = tex_mgr.TryGet(item.mesh_id);
			if(!item.tex)
			{
				item.icon = missing_texture;
				Warn("Missing item texture '%s'.", item.mesh_id.c_str());
				++load_errors;
			}
		}
		else
		{
			item.mesh = mesh_mgr.TryGet(item.mesh_id);
			if(!item.mesh)
			{
				item.icon = missing_texture;
				item.flags &= ~ITEM_GROUND_MESH;
				Warn("Missing item mesh '%s'.", item.mesh_id.c_str());
				++load_errors;
			}
		}
	}

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
