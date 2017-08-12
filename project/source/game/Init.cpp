#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "Language.h"
#include "Terrain.h"
#include "Content.h"
#include "LoadScreen.h"
#include "CreateCharacterPanel.h"
#include "MainMenu.h"
#include "ServerPanel.h"
#include "PickServerPanel.h"
#include "Spell.h"
#include "Trap.h"

extern void HumanPredraw(void* ptr, Matrix* mat, int n);
extern const int ITEM_IMAGE_SIZE;
extern string g_system_dir;

//=================================================================================================
// Initialize game and show loadscreen.
//=================================================================================================
bool Game::InitGame()
{
	Info("Game: Initializing game.");

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
		ShowError(Format("Game: Failed to initialize game: %s", err), Logger::L_FATAL);
		return false;
	}
}

//=================================================================================================
// Preconfigure game vars.
//=================================================================================================
void Game::PreconfigureGame()
{
	Info("Game: Preconfiguring game.");

	UnlockCursor(false);

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
	MeshInstance::Predraw = HumanPredraw;

	PreinitGui();
	CreateVertexDeclarations();
	PreloadLanguage();
	PreloadData();
	CreatePlaceholderResources();
	ResourceManager::Get().SetLoadScreen(load_screen);
}

//=================================================================================================
// Create placeholder resources (missing texture).
//=================================================================================================
void Game::CreatePlaceholderResources()
{
	// create item missing texture
	SURFACE surf;
	D3DLOCKED_RECT rect;

	V(device->CreateTexture(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &missing_texture, nullptr));
	V(missing_texture->GetSurfaceLevel(0, &surf));
	V(surf->LockRect(&rect, nullptr, 0));

	const DWORD col[2] = { COLOR_RGB(255, 0, 255), COLOR_RGB(0, 255, 0) };

	for(int y = 0; y < ITEM_IMAGE_SIZE; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)rect.pBits) + rect.Pitch*y);
		for(int x = 0; x < ITEM_IMAGE_SIZE; ++x)
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

	txCreatingListOfFiles = Str("creatingListOfFiles");
	txConfiguringGame = Str("configuringGame");
	txLoadingItems = Str("loadingItems");
	txLoadingSpells = Str("loadingSpells");
	txLoadingUnits = Str("loadingUnits");
	txLoadingMusics = Str("loadingMusics");
	txLoadingBuildings = Str("loadingBuildings");
	txLoadingRequires = Str("loadingRequires");
	txLoadingShaders = Str("loadingShaders");
	txLoadingDialogs = Str("loadingDialogs");
	txLoadingLanguageFiles = Str("loadingLanguageFiles");
}

//=================================================================================================
// Load data used by loadscreen.
//=================================================================================================
void Game::PreloadData()
{
	ResourceManager::Get().AddDir("data/preload");

	// loadscreen textures
	load_screen->LoadData();

	// gui shader
	eGui = CompileShader("gui.fx");
	GUI.SetShader(eGui);

	// intro music
	if(!nomusic)
	{
		Music* music = new Music;
		music->music = ResourceManager::Get<Sound>().GetLoadedMusic("Intro.ogg");
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
	Info("Game: Loading system.");
	load_screen->Setup(0.f, 0.1f, 11, txCreatingListOfFiles);

	AddFilesystem();
	LoadDatafiles();
	LoadLanguageFiles();
	load_screen->Tick(txLoadingShaders);
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
	uint loaded;

	// items
	load_screen->Tick(txLoadingItems);
	loaded = LoadItems(crc_items, load_errors);
	Info("Game: Loaded items: %u (crc %p).", loaded, crc_items);

	// spells
	load_screen->Tick(txLoadingSpells);
	loaded = LoadSpells(crc_spells, load_errors);
	Info("Game: Loaded spells: %u (crc %p).", loaded, crc_spells);

	// dialogs
	load_screen->Tick(txLoadingDialogs);
	loaded = LoadDialogs(crc_dialogs, load_errors);
	Info("Game: Loaded dialogs: %u (crc %p).", loaded, crc_dialogs);

	// units
	load_screen->Tick(txLoadingUnits);
	loaded = LoadUnits(crc_units, load_errors);
	Info("Game: Loaded units: %u (crc %p).", loaded, crc_units);

	// musics
	load_screen->Tick(txLoadingMusics);
	loaded = LoadMusicDatafile(load_errors);
	Info("Game: Loaded music: %u.", loaded);

	// content
	content::system_dir = g_system_dir;
	content::LoadContent([this, &res_mgr](content::Id id)
	{
		switch(id)
		{
		case content::Id::Buildings:
			load_screen->Tick(txLoadingBuildings);
			break;
		}
	});

	// required
	load_screen->Tick(txLoadingRequires);
	LoadRequiredStats(load_errors);
}

//=================================================================================================
// Load language files.
//=================================================================================================
void Game::LoadLanguageFiles()
{
	Info("Game: Loading language files.");
	load_screen->Tick(txLoadingLanguageFiles);

	LoadLanguageFile("menu.txt");
	LoadLanguageFile("stats.txt");
	::LoadLanguageFiles();
	LoadDialogTexts();

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

	txHaveErrors = Str("haveErrors");
}

//=================================================================================================
// Initialize everything needed by game before loading content.
//=================================================================================================
void Game::ConfigureGame()
{
	Info("Game: Configuring game.");
	load_screen->Tick(txConfiguringGame);

	InitScene();
	InitSuperShader();
	AddCommands();
	InitGui();
	ResetGameKeys();
	LoadGameKeys();
	SetMeshSpecular();
	LoadSaveSlots();
	SetRoomPointers();

	for(int i = 0; i < SG_MAX; ++i)
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
	Info("Game: Loading data.");
	auto& res_mgr = ResourceManager::Get();

	res_mgr.SetMutex(mutex);
	mutex = nullptr;
	res_mgr.PrepareLoadScreen(0.1f);
	AddLoadTasks();
	res_mgr.StartLoadScreen();
}

//=================================================================================================
// Configure game after loading content.
//=================================================================================================
void Game::PostconfigureGame()
{
	Info("Game: Postconfiguring game.");

	LockCursor();
	CreateCollisionShapes();
	create_character->Init();

	// load gui textures that require instant loading
	GUI.GetLayout()->LoadDefault();

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
		ValidateGameData(true);
#ifdef _DEBUG
	else
		ValidateGameData(false);
#endif

	// show errors notification
	bool start_game_mode = true;
	load_errors += content::errors;
	load_warnings += content::warnings;
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
		info.parent = main_menu;
		info.order = ORDER_TOPMOST;
		info.pause = false;
		info.auto_wrap = true;
		GUI.ShowDialog(info);
		start_game_mode = false;
#endif
	}

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
				Warn("Quickstart: Can't create server, no server name.");
		}
		else
			Warn("Quickstart: Can't create server, no player nick.");
		break;
	case QUICKSTART_JOIN_LAN:
		if(!player_name.empty())
		{
			pick_autojoin = true;
			pick_server_panel->Show();
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	case QUICKSTART_JOIN_IP:
		if(!player_name.empty())
		{
			if(!server_ip.empty())
				QuickJoinIp();
			else
				Warn("Quickstart: Can't join server, no server ip.");
		}
		else
			Warn("Quickstart: Can't join server, no player nick.");
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void Game::AddLoadTasks()
{
	auto& res_mgr = ResourceManager::Get();
	auto& tex_mgr = ResourceManager::Get<Texture>();
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	auto& vd_mgr = ResourceManager::Get<VertexData>();
	auto& sound_mgr = ResourceManager::Get<Sound>();

	// gui textures
	res_mgr.AddTaskCategory(txLoadGuiTextures);
	LoadGuiData();
	tex_mgr.AddLoadTask("klasa_cecha.png", tKlasaCecha);
	tex_mgr.AddLoadTask("celownik.png", tCelownik);
	tex_mgr.AddLoadTask("bubble.png", tBubble);
	tex_mgr.AddLoadTask("gotowy.png", tGotowy);
	tex_mgr.AddLoadTask("niegotowy.png", tNieGotowy);
	tex_mgr.AddLoadTask("save-16.png", tIcoZapis);
	tex_mgr.AddLoadTask("padlock-16.png", tIcoHaslo);
	tex_mgr.AddLoadTask("emerytura.jpg", tEmerytura);
	tex_mgr.AddLoadTask("equipped.png", tEquipped);
	tex_mgr.AddLoadTask("czern.bmp", tCzern);
	tex_mgr.AddLoadTask("rip.jpg", tRip);
	tex_mgr.AddLoadTask("dialog_up.png", tDialogUp);
	tex_mgr.AddLoadTask("dialog_down.png", tDialogDown);
	tex_mgr.AddLoadTask("mini_unit.png", tMiniunit);
	tex_mgr.AddLoadTask("mini_unit2.png", tMiniunit2);
	tex_mgr.AddLoadTask("schody_dol.png", tSchodyDol);
	tex_mgr.AddLoadTask("schody_gora.png", tSchodyGora);
	tex_mgr.AddLoadTask("czerwono.png", tObwodkaBolu);
	tex_mgr.AddLoadTask("dark_portal.png", tPortal);
	tex_mgr.AddLoadTask("mini_unit3.png", tMiniunit3);
	tex_mgr.AddLoadTask("mini_unit4.png", tMiniunit4);
	tex_mgr.AddLoadTask("mini_unit5.png", tMiniunit5);
	tex_mgr.AddLoadTask("mini_bag.png", tMinibag);
	tex_mgr.AddLoadTask("mini_bag2.png", tMinibag2);
	tex_mgr.AddLoadTask("mini_portal.png", tMiniportal);
	for(ClassInfo& ci : g_classes)
		tex_mgr.AddLoadTask(ci.icon_file, ci.icon);
	tex_mgr.AddLoadTask("warning.png", tWarning);
	tex_mgr.AddLoadTask("error.png", tError);

	// terrain textures
	res_mgr.AddTaskCategory(txLoadTerrainTextures);
	tex_mgr.AddLoadTask("trawa.jpg", tTrawa);
	tex_mgr.AddLoadTask("droga.jpg", tDroga);
	tex_mgr.AddLoadTask("ziemia.jpg", tZiemia);
	tex_mgr.AddLoadTask("Grass0157_5_S.jpg", tTrawa2);
	tex_mgr.AddLoadTask("LeavesDead0045_1_S.jpg", tTrawa3);
	tex_mgr.AddLoadTask("pole.jpg", tPole);
	tFloorBase.diffuse = tex_mgr.AddLoadTask("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = tex_mgr.AddLoadTask("sciana.jpg");
	tWallBase.normal = tex_mgr.AddLoadTask("sciana_nrm.jpg");
	tWallBase.specular = tex_mgr.AddLoadTask("sciana_spec.jpg");
	tCeilBase.diffuse = tex_mgr.AddLoadTask("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;

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
	tWoda = tex_mgr.AddLoadTask("water.png");
	tFlare = tex_mgr.AddLoadTask("flare.png");
	tFlare2 = tex_mgr.AddLoadTask("flare2.png");
	tex_mgr.AddLoadTask("lighting_line.png", tLightingLine);

	// physic meshes
	res_mgr.AddTaskCategory(txLoadPhysicMeshes);
	vdSchodyGora = vd_mgr.AddLoadTask("schody_gora.phy");
	vdSchodyDol = vd_mgr.AddLoadTask("schody_dol.phy");
	vdNaDrzwi = vd_mgr.AddLoadTask("nadrzwi.phy");

	// models
	res_mgr.AddTaskCategory(txLoadModels);
	aBox = mesh_mgr.AddLoadTask("box.qmsh");
	aCylinder = mesh_mgr.AddLoadTask("cylinder.qmsh");
	aSphere = mesh_mgr.AddLoadTask("sphere.qmsh");
	aCapsule = mesh_mgr.AddLoadTask("capsule.qmsh");
	aArrow = mesh_mgr.AddLoadTask("strzala.qmsh");
	aSkybox = mesh_mgr.AddLoadTask("skybox.qmsh");
	aWorek = mesh_mgr.AddLoadTask("worek.qmsh");
	aSkrzynia = mesh_mgr.AddLoadTask("skrzynia.qmsh");
	aKratka = mesh_mgr.AddLoadTask("kratka.qmsh");
	aNaDrzwi = mesh_mgr.AddLoadTask("nadrzwi.qmsh");
	aNaDrzwi2 = mesh_mgr.AddLoadTask("nadrzwi2.qmsh");
	aSchodyDol = mesh_mgr.AddLoadTask("schody_dol.qmsh");
	aSchodyDol2 = mesh_mgr.AddLoadTask("schody_dol2.qmsh");
	aSchodyGora = mesh_mgr.AddLoadTask("schody_gora.qmsh");
	aSpellball = mesh_mgr.AddLoadTask("spellball.qmsh");
	aDrzwi = mesh_mgr.AddLoadTask("drzwi.qmsh");
	aDrzwi2 = mesh_mgr.AddLoadTask("drzwi2.qmsh");
	aHumanBase = mesh_mgr.AddLoadTask("czlowiek.qmsh");
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

	// buildings
	res_mgr.AddTaskCategory(txLoadBuildings);
	for(Building* b : content::buildings)
	{
		if(!b->mesh_id.empty())
			b->mesh = mesh_mgr.AddLoadTask(b->mesh_id);
		if(!b->inside_mesh_id.empty())
			b->inside_mesh = mesh_mgr.AddLoadTask(b->inside_mesh_id);
	}

	// traps
	res_mgr.AddTaskCategory(txLoadTraps);
	for(uint i = 0; i < n_traps; ++i)
	{
		BaseTrap& t = g_traps[i];
		if(t.mesh_id)
			 mesh_mgr.AddLoadTask(t.mesh_id, &t, TaskCallback(this, &Game::SetupTrap));
		if(t.mesh_id2)
			t.mesh2 = mesh_mgr.AddLoadTask(t.mesh_id2);
		if(!nosound)
		{
			if(t.sound_id)
				sound_mgr.AddLoadTask(t.sound_id, t.sound);
			if(t.sound_id2)
				sound_mgr.AddLoadTask(t.sound_id2, t.sound2);
			if(t.sound_id3)
				sound_mgr.AddLoadTask(t.sound_id3, t.sound3);
		}
	}

	// spells
	res_mgr.AddTaskCategory(txLoadSpells);
	for(Spell* spell_ptr : spells)
	{
		Spell& spell = *spell_ptr;

		if(!nosound)
		{
			if(!spell.sound_cast_id.empty())
				sound_mgr.AddLoadTask(spell.sound_cast_id, spell.sound_cast);
			if(!spell.sound_hit_id.empty())
				sound_mgr.AddLoadTask(spell.sound_hit_id, spell.sound_hit);
		}
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

	// objects
	res_mgr.AddTaskCategory(txLoadObjects);
	for(uint i = 0; i < n_objs; ++i)
	{
		Obj& o = g_objs[i];
		if(IS_SET(o.flags2, OBJ2_VARIANT))
		{
			VariantObj& vo = *o.variant;
			if(!vo.loaded)
			{
				for(uint i = 0; i < vo.count; ++i)
					vo.entries[i].mesh = mesh_mgr.AddLoadTask(vo.entries[i].mesh_name);
				vo.loaded = true;
			}
			res_mgr.AddTask(&o, TaskCallback(this, &Game::SetupObject));
		}
		else if(o.mesh_id)
		{
			if(IS_SET(o.flags, OBJ_SCALEABLE))
			{
				o.mesh = mesh_mgr.AddLoadTask(o.mesh_id);
				o.matrix = nullptr;
			}
			else
			{
				if(o.type == OBJ_CYLINDER)
				{
					o.mesh = mesh_mgr.AddLoadTask(o.mesh_id);
					if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
					{
						btCylinderShape* shape = new btCylinderShape(btVector3(o.r, o.h, o.r));
						o.shape = shape;
					}
					o.matrix = nullptr;
				}
				else
					mesh_mgr.AddLoadTask(o.mesh_id, &o, TaskCallback(this, &Game::SetupObject));
			}
		}
		else
		{
			o.mesh = nullptr;
			o.matrix = nullptr;
		}
	}

	// useable objects
	for(uint i = 0; i < n_base_usables; ++i)
	{
		BaseUsable& bu = g_base_usables[i];
		bu.obj = FindObject(bu.obj_name);
		if(!nosound && bu.sound_id)
			sound_mgr.AddLoadTask(bu.sound_id, bu.sound);
		if(bu.item_id)
			bu.item = FindItem(bu.item_id);
	}

	// units
	res_mgr.AddTaskCategory(txLoadUnits);
	for(UnitData* ud_ptr : unit_datas)
	{
		UnitData& ud = *ud_ptr;

		// model
		if(!ud.mesh_id.empty())
			ud.mesh = mesh_mgr.AddLoadTask(ud.mesh_id);

		// sounds
		SoundPack& sounds = *ud.sounds;
		if(!nosound && !sounds.inited)
		{
			sounds.inited = true;
			for(int i = 0; i < SOUND_MAX; ++i)
			{
				if(!sounds.filename[i].empty())
					sound_mgr.AddLoadTask(sounds.filename[i], sounds.sound[i]);
			}
		}

		// textures
		if(ud.tex && !ud.tex->inited)
		{
			ud.tex->inited = true;
			for(TexId& ti : ud.tex->textures)
			{
				if(!ti.id.empty())
					ti.tex = tex_mgr.AddLoadTask(ti.id);
			}
		}
	}

	// items
	res_mgr.AddTaskCategory(txLoadItems);
	for(Armor* armor : g_armors)
	{
		Armor& a = *armor;
		if(!a.tex_override.empty())
		{
			for(TexId& ti : a.tex_override)
			{
				if(!ti.id.empty())
					ti.tex = tex_mgr.AddLoadTask(ti.id);
			}
		}
	}
	LoadItemsData();

	// sounds
	if(!nosound)
	{
		res_mgr.AddTaskCategory(txLoadSounds);
		sound_mgr.AddLoadTask("gulp.mp3", sGulp);
		sound_mgr.AddLoadTask("moneta2.mp3", sCoins);
		sound_mgr.AddLoadTask("bow1.mp3", sBow[0]);
		sound_mgr.AddLoadTask("bow2.mp3", sBow[1]);
		sound_mgr.AddLoadTask("drzwi-02.mp3", sDoor[0]);
		sound_mgr.AddLoadTask("drzwi-03.mp3", sDoor[1]);
		sound_mgr.AddLoadTask("drzwi-04.mp3", sDoor[2]);
		sound_mgr.AddLoadTask("104528__skyumori__door-close-sqeuak-02.mp3", sDoorClose);
		sound_mgr.AddLoadTask("wont_budge.ogg", sDoorClosed[0]);
		sound_mgr.AddLoadTask("wont_budge2.ogg", sDoorClosed[1]);
		sound_mgr.AddLoadTask("bottle.wav", sItem[0]); // potion
		sound_mgr.AddLoadTask("armor-light.wav", sItem[1]); // light armor
		sound_mgr.AddLoadTask("chainmail1.wav", sItem[2]); // heavy armor
		sound_mgr.AddLoadTask("metal-ringing.wav", sItem[3]); // crystal
		sound_mgr.AddLoadTask("wood-small.wav", sItem[4]); // bow
		sound_mgr.AddLoadTask("cloth-heavy.wav", sItem[5]); // shield
		sound_mgr.AddLoadTask("sword-unsheathe.wav", sItem[6]); // weapon
		sound_mgr.AddLoadTask("interface3.wav", sItem[7]);
		sound_mgr.AddLoadTask("hello-3.mp3", sTalk[0]);
		sound_mgr.AddLoadTask("hello-4.mp3", sTalk[1]);
		sound_mgr.AddLoadTask("hmph.wav", sTalk[2]);
		sound_mgr.AddLoadTask("huh-2.mp3", sTalk[3]);
		sound_mgr.AddLoadTask("chest_open.mp3", sChestOpen);
		sound_mgr.AddLoadTask("chest_close.mp3", sChestClose);
		sound_mgr.AddLoadTask("door_budge.mp3", sDoorBudge);
		sound_mgr.AddLoadTask("atak_kamien.mp3", sRock);
		sound_mgr.AddLoadTask("atak_drewno.mp3", sWood);
		sound_mgr.AddLoadTask("atak_krysztal.mp3", sCrystal);
		sound_mgr.AddLoadTask("atak_metal.mp3", sMetal);
		sound_mgr.AddLoadTask("atak_cialo.mp3", sBody[0]);
		sound_mgr.AddLoadTask("atak_cialo2.mp3", sBody[1]);
		sound_mgr.AddLoadTask("atak_cialo3.mp3", sBody[2]);
		sound_mgr.AddLoadTask("atak_cialo4.mp3", sBody[3]);
		sound_mgr.AddLoadTask("atak_cialo5.mp3", sBody[4]);
		sound_mgr.AddLoadTask("atak_kosci.mp3", sBone);
		sound_mgr.AddLoadTask("atak_skora.mp3", sSkin);
		sound_mgr.AddLoadTask("arena_fight.mp3", sArenaFight);
		sound_mgr.AddLoadTask("arena_wygrana.mp3", sArenaWin);
		sound_mgr.AddLoadTask("arena_porazka.mp3", sArenaLost);
		sound_mgr.AddLoadTask("unlock.mp3", sUnlock);
		sound_mgr.AddLoadTask("TouchofDeath.ogg", sEvil);
		sound_mgr.AddLoadTask("shade8.wav", sXarTalk);
		sound_mgr.AddLoadTask("ogre1.wav", sOrcTalk);
		sound_mgr.AddLoadTask("goblin-7.wav", sGoblinTalk);
		sound_mgr.AddLoadTask("golem_alert.mp3", sGolemTalk);
		sound_mgr.AddLoadTask("eat.mp3", sEat);
	}

	// musics
	if(!nomusic)
		LoadMusic(MusicType::Title);
}
