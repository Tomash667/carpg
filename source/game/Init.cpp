#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "DatatypeManager.h"
#include "Language.h"
#include "Terrain.h"

extern void HumanPredraw(void* ptr, MATRIX* mat, int n);
extern const int ITEM_IMAGE_SIZE;

//=================================================================================================
// Initialize game and show loadscreen.
//=================================================================================================
bool Game::InitGame()
{
	INFO("Game: Initializing game.");

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

		INFO("Game: Game initialized.");
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
	INFO("Game: Preconfiguring game.");

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
	InitializeDatatypeManager();
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
	INFO("Game: Loading system.");
	resMgr.PrepareLoadScreen2(0.1f, 12, txCreateListOfFiles);

	AddFilesystem();
	LoadDatafiles();
	LoadLanguageFiles();
	resMgr.NextTask(txLoadShaders);
	LoadShaders();
	ConfigureGame();
	resMgr.EndLoadScreenStage();
}

//=================================================================================================
// Add filesystem.
//=================================================================================================
void Game::AddFilesystem()
{
	INFO("Game: Creating list of files.");
	resMgr.AddDir("data");
	resMgr.AddPak("data/data.pak", "KrystaliceFire");
}

//=================================================================================================
// Load datafiles (items/units/etc).
//=================================================================================================
void Game::LoadDatafiles()
{
	INFO("Game: Loading datafiles.");
	load_errors = 0;
	uint loaded;

	// items
	resMgr.NextTask(txLoadItemsDatafile);
	loaded = LoadItems(crc_items, load_errors);
	INFO(Format("Game: Loaded items: %u (crc %p).", loaded, crc_items));

	// spells
	resMgr.NextTask(txLoadSpellDatafile);
	loaded = LoadSpells(crc_spells, load_errors);
	INFO(Format("Game: Loaded spells: %u (crc %p).", loaded, crc_spells));

	// dialogs
	resMgr.NextTask(txLoadDialogs);
	loaded = LoadDialogs(crc_dialogs, load_errors);
	INFO(Format("Game: Loaded dialogs: %u (crc %p).", loaded, crc_dialogs));

	// units
	resMgr.NextTask(txLoadUnitDatafile);
	loaded = LoadUnits(crc_units, load_errors);
	INFO(Format("Game: Loaded units: %u (crc %p).", loaded, crc_units));

	// musics
	resMgr.NextTask(txLoadMusicDatafile);
	loaded = LoadMusicDatafile(load_errors);
	INFO(Format("Game: Loaded music: %u.", loaded));

	// datatypes
	resMgr.NextTask(txLoadingDatafiles);
	dt_mgr->LoadDatatypesFromText(load_errors);
	dt_mgr->LogLoadedDatatypes();

	// required
	resMgr.NextTask(txLoadRequires);
	required_missing = !LoadRequiredStats(load_errors);
}

//=================================================================================================
// Load language files.
//=================================================================================================
void Game::LoadLanguageFiles()
{
	INFO("Game: Loading language files.");
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

	txHaveErrors = Str("haveErrors");
	txHaveErrorsCritical = Str("haveErrorsCritical");
	txHaveErrorsContinue = Str("haveErrorsContinue");
	txHaveErrorsQuit = Str("haveErrorsQuit");
}

//=================================================================================================
// Initialize everything needed by game before loading content.
//=================================================================================================
void Game::ConfigureGame()
{
	INFO("Game: Configuring game.");
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
	INFO("Game: Loading data.");

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
	INFO("Game: Postconfiguring game.");

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
		ValidateGameData(true);
#ifdef _DEBUG
	else
		ValidateGameData(false);
#endif

	// show errors notification
	bool start_game_mode = true;
	if(load_errors > 0)
	{
		DialogInfo info;
		info.name = "have_errors";
		if(required_missing)
		{
			ERROR(Format("Game: %d loading errors with required missing.", load_errors));
			info.text = Format(txHaveErrorsCritical, load_errors);
			info.type = DIALOG_YESNO;
			info.event = [this](int result)
			{
				if(result == BUTTON_NO)
					StartGameMode();
				else
					Quit();
			};
			info.img = tError;

			cstring names[] = { txHaveErrorsQuit, txHaveErrorsContinue };
			info.custom_names = names;
		}
		else
		{
			WARN(Format("Game: %d loading errors.", load_errors));
			info.text = Format(txHaveErrors, load_errors);
			info.type = DIALOG_OK;
			info.img = tWarning;
		}
		info.parent = main_menu;
		info.order = ORDER_TOPMOST;
		info.pause = false;
		info.auto_wrap = true;
		GUI.ShowDialog(info);
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


//=================================================================================================
void Game::AddLoadTasks()
{
	// gui textures
	resMgr.AddTaskCategory(txLoadGuiTextures);
	LoadGuiData();
	resMgr.GetLoadedTexture("klasa_cecha.png", tKlasaCecha);
	resMgr.GetLoadedTexture("celownik.png", tCelownik);
	resMgr.GetLoadedTexture("bubble.png", tBubble);
	resMgr.GetLoadedTexture("gotowy.png", tGotowy);
	resMgr.GetLoadedTexture("niegotowy.png", tNieGotowy);
	resMgr.GetLoadedTexture("save-16.png", tIcoZapis);
	resMgr.GetLoadedTexture("padlock-16.png", tIcoHaslo);
	resMgr.GetLoadedTexture("emerytura.jpg", tEmerytura);
	resMgr.GetLoadedTexture("equipped.png", tEquipped);
	resMgr.GetLoadedTexture("czern.bmp", tCzern);
	resMgr.GetLoadedTexture("rip.jpg", tRip);
	resMgr.GetLoadedTexture("dialog_up.png", tDialogUp);
	resMgr.GetLoadedTexture("dialog_down.png", tDialogDown);
	resMgr.GetLoadedTexture("mini_unit.png", tMiniunit);
	resMgr.GetLoadedTexture("mini_unit2.png", tMiniunit2);
	resMgr.GetLoadedTexture("schody_dol.png", tSchodyDol);
	resMgr.GetLoadedTexture("schody_gora.png", tSchodyGora);
	resMgr.GetLoadedTexture("czerwono.png", tObwodkaBolu);
	resMgr.GetLoadedTexture("dark_portal.png", tPortal);
	resMgr.GetLoadedTexture("mini_unit3.png", tMiniunit3);
	resMgr.GetLoadedTexture("mini_unit4.png", tMiniunit4);
	resMgr.GetLoadedTexture("mini_unit5.png", tMiniunit5);
	resMgr.GetLoadedTexture("mini_bag.png", tMinibag);
	resMgr.GetLoadedTexture("mini_bag2.png", tMinibag2);
	resMgr.GetLoadedTexture("mini_portal.png", tMiniportal);
	for(ClassInfo& ci : g_classes)
		resMgr.GetLoadedTexture(ci.icon_file, ci.icon);
	resMgr.GetLoadedTexture("warning.png", tWarning);
	resMgr.GetLoadedTexture("error.png", tError);

	// terrain textures
	resMgr.AddTaskCategory(txLoadTerrainTextures);
	resMgr.GetLoadedTexture("trawa.jpg", tTrawa);
	resMgr.GetLoadedTexture("droga.jpg", tDroga);
	resMgr.GetLoadedTexture("ziemia.jpg", tZiemia);
	resMgr.GetLoadedTexture("Grass0157_5_S.jpg", tTrawa2);
	resMgr.GetLoadedTexture("LeavesDead0045_1_S.jpg", tTrawa3);
	resMgr.GetLoadedTexture("pole.jpg", tPole);
	tFloorBase.diffuse = resMgr.GetLoadedTexture("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = resMgr.GetLoadedTexture("sciana.jpg");
	tWallBase.normal = resMgr.GetLoadedTexture("sciana_nrm.jpg");
	tWallBase.specular = resMgr.GetLoadedTexture("sciana_spec.jpg");
	tCeilBase.diffuse = resMgr.GetLoadedTexture("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;

	// particles
	resMgr.AddTaskCategory(txLoadParticles);
	tKrew[BLOOD_RED] = resMgr.GetLoadedTexture("krew.png");
	tKrew[BLOOD_GREEN] = resMgr.GetLoadedTexture("krew2.png");
	tKrew[BLOOD_BLACK] = resMgr.GetLoadedTexture("krew3.png");
	tKrew[BLOOD_BONE] = resMgr.GetLoadedTexture("iskra.png");
	tKrew[BLOOD_ROCK] = resMgr.GetLoadedTexture("kamien.png");
	tKrew[BLOOD_IRON] = resMgr.GetLoadedTexture("iskra.png");
	tKrewSlad[BLOOD_RED] = resMgr.GetLoadedTexture("krew_slad.png");
	tKrewSlad[BLOOD_GREEN] = resMgr.GetLoadedTexture("krew_slad2.png");
	tKrewSlad[BLOOD_BLACK] = resMgr.GetLoadedTexture("krew_slad3.png");
	tKrewSlad[BLOOD_BONE] = nullptr;
	tKrewSlad[BLOOD_ROCK] = nullptr;
	tKrewSlad[BLOOD_IRON] = nullptr;
	tIskra = resMgr.GetLoadedTexture("iskra.png");
	tWoda = resMgr.GetLoadedTexture("water.png");
	tFlare = resMgr.GetLoadedTexture("flare.png");
	tFlare2 = resMgr.GetLoadedTexture("flare2.png");
	resMgr.GetLoadedTexture("lighting_line.png", tLightingLine);

	// physic meshes
	resMgr.AddTaskCategory(txLoadPhysicMeshes);
	resMgr.GetLoadedMeshVertexData("schody_gora_phy.qmsh", vdSchodyGora);
	resMgr.GetLoadedMeshVertexData("schody_dol_phy.qmsh", vdSchodyDol);
	resMgr.GetLoadedMeshVertexData("nadrzwi_phy.qmsh", vdNaDrzwi);

	// models
	resMgr.AddTaskCategory(txLoadModels);
	resMgr.GetLoadedMesh("box.qmsh", aBox);
	resMgr.GetLoadedMesh("cylinder.qmsh", aCylinder);
	resMgr.GetLoadedMesh("sphere.qmsh", aSphere);
	resMgr.GetLoadedMesh("capsule.qmsh", aCapsule);
	resMgr.GetLoadedMesh("strzala.qmsh", aArrow);
	resMgr.GetLoadedMesh("skybox.qmsh", aSkybox);
	resMgr.GetLoadedMesh("worek.qmsh", aWorek);
	resMgr.GetLoadedMesh("skrzynia.qmsh", aSkrzynia);
	resMgr.GetLoadedMesh("kratka.qmsh", aKratka);
	resMgr.GetLoadedMesh("nadrzwi.qmsh", aNaDrzwi);
	resMgr.GetLoadedMesh("nadrzwi2.qmsh", aNaDrzwi2);
	resMgr.GetLoadedMesh("schody_dol.qmsh", aSchodyDol);
	resMgr.GetLoadedMesh("schody_dol2.qmsh", aSchodyDol2);
	resMgr.GetLoadedMesh("schody_gora.qmsh", aSchodyGora);
	resMgr.GetLoadedMesh("beczka.qmsh", aBeczka);
	resMgr.GetLoadedMesh("spellball.qmsh", aSpellball);
	resMgr.GetLoadedMesh("drzwi.qmsh", aDrzwi);
	resMgr.GetLoadedMesh("drzwi2.qmsh", aDrzwi2);
	resMgr.GetLoadedMesh("czlowiek.qmsh", aHumanBase);
	resMgr.GetLoadedMesh("hair1.qmsh", aHair[0]);
	resMgr.GetLoadedMesh("hair2.qmsh", aHair[1]);
	resMgr.GetLoadedMesh("hair3.qmsh", aHair[2]);
	resMgr.GetLoadedMesh("hair4.qmsh", aHair[3]);
	resMgr.GetLoadedMesh("hair5.qmsh", aHair[4]);
	resMgr.GetLoadedMesh("eyebrows.qmsh", aEyebrows);
	resMgr.GetLoadedMesh("mustache1.qmsh", aMustache[0]);
	resMgr.GetLoadedMesh("mustache2.qmsh", aMustache[1]);
	resMgr.GetLoadedMesh("beard1.qmsh", aBeard[0]);
	resMgr.GetLoadedMesh("beard2.qmsh", aBeard[1]);
	resMgr.GetLoadedMesh("beard3.qmsh", aBeard[2]);
	resMgr.GetLoadedMesh("beard4.qmsh", aBeard[3]);
	resMgr.GetLoadedMesh("beardm1.qmsh", aBeard[4]);

	// buildings
	resMgr.AddTaskCategory(txLoadBuildings);
	for(Building* b : content::buildings)
	{
		if(!b->mesh_id.empty())
			resMgr.GetLoadedMesh(b->mesh_id, b->mesh);
		if(!b->inside_mesh_id.empty())
			resMgr.GetLoadedMesh(b->inside_mesh_id, b->inside_mesh);
	}

	// traps
	resMgr.AddTaskCategory(txLoadTraps);
	for(uint i = 0; i<n_traps; ++i)
	{
		BaseTrap& t = g_traps[i];
		if(t.mesh_id)
			resMgr.GetLoadedMesh(t.mesh_id, Task(&t, TaskCallback(this, &Game::SetupTrap)));
		if(t.mesh_id2)
			resMgr.GetLoadedMesh(t.mesh_id2, t.mesh2);
		if(!nosound)
		{
			if(t.sound_id)
				resMgr.GetLoadedSound(t.sound_id, t.sound);
			if(t.sound_id2)
				resMgr.GetLoadedSound(t.sound_id2, t.sound2);
			if(t.sound_id3)
				resMgr.GetLoadedSound(t.sound_id3, t.sound3);
		}
	}

	// spells
	resMgr.AddTaskCategory(txLoadSpells);
	for(Spell* spell_ptr : spells)
	{
		Spell& spell = *spell_ptr;

		if(!nosound)
		{
			if(!spell.sound_cast_id.empty())
				resMgr.GetLoadedSound(spell.sound_cast_id, spell.sound_cast);
			if(!spell.sound_hit_id.empty())
				resMgr.GetLoadedSound(spell.sound_hit_id, spell.sound_hit);
		}
		if(!spell.tex_id.empty())
			spell.tex = resMgr.GetLoadedTexture(spell.tex_id.c_str());
		if(!spell.tex_particle_id.empty())
			spell.tex_particle = resMgr.GetLoadedTexture(spell.tex_particle_id.c_str());
		if(!spell.tex_explode_id.empty())
			spell.tex_explode = resMgr.GetLoadedTexture(spell.tex_explode_id.c_str());
		if(!spell.mesh_id.empty())
			resMgr.GetLoadedMesh(spell.mesh_id, spell.mesh);

		if(spell.type == Spell::Ball || spell.type == Spell::Point)
			spell.shape = new btSphereShape(spell.size);
	}

	// objects
	resMgr.AddTaskCategory(txLoadObjects);
	for(uint i = 0; i<n_objs; ++i)
	{
		Obj& o = g_objs[i];
		if(IS_SET(o.flags2, OBJ2_VARIANT))
		{
			VariantObj& vo = *o.variant;
			if(!vo.loaded)
			{
				for(uint i = 0; i<vo.count; ++i)
					resMgr.GetLoadedMesh(vo.entries[i].mesh_name, vo.entries[i].mesh);
				vo.loaded = true;
			}
			resMgr.AddTask(Task(&o, TaskCallback(this, &Game::SetupObject)));
		}
		else if(o.mesh_id)
		{
			if(IS_SET(o.flags, OBJ_SCALEABLE))
			{
				resMgr.GetLoadedMesh(o.mesh_id, o.mesh);
				o.matrix = nullptr;
			}
			else
			{
				if(o.type == OBJ_CYLINDER)
				{
					resMgr.GetLoadedMesh(o.mesh_id, o.mesh);
					if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
					{
						btCylinderShape* shape = new btCylinderShape(btVector3(o.r, o.h, o.r));
						o.shape = shape;
					}
					o.matrix = nullptr;
				}
				else
					resMgr.GetLoadedMesh(o.mesh_id, Task(&o, TaskCallback(this, &Game::SetupObject)));
			}
		}
		else
		{
			o.mesh = nullptr;
			o.matrix = nullptr;
		}
	}
	for(uint i = 0; i<n_base_usables; ++i)
	{
		BaseUsable& bu = g_base_usables[i];
		bu.obj = FindObject(bu.obj_name);
		if(!nosound && bu.sound_id)
			resMgr.GetLoadedSound(bu.sound_id, bu.sound);
		if(bu.item_id)
			bu.item = FindItem(bu.item_id);
	}

	// units
	resMgr.AddTaskCategory(txLoadUnits);
	for(UnitData* ud_ptr : unit_datas)
	{
		UnitData& ud = *ud_ptr;

		// model
		if(!ud.mesh_id.empty())
			resMgr.GetLoadedMesh(ud.mesh_id, ud.mesh);

		// sounds
		SoundPack& sounds = *ud.sounds;
		if(!nosound && !sounds.inited)
		{
			sounds.inited = true;
			for(int i = 0; i<SOUND_MAX; ++i)
			{
				if(!sounds.filename[i].empty())
					resMgr.GetLoadedSound(sounds.filename[i], sounds.sound[i]);
			}
		}

		// textures
		if(ud.tex && !ud.tex->inited)
		{
			ud.tex->inited = true;
			for(TexId& ti : ud.tex->textures)
			{
				if(!ti.id.empty())
					ti.tex = resMgr.GetLoadedTexture(ti.id.c_str());
			}
		}
	}

	// items
	resMgr.AddTaskCategory(txLoadItems);
	for(Armor* armor : g_armors)
	{
		Armor& a = *armor;
		if(!a.tex_override.empty())
		{
			for(TexId& ti : a.tex_override)
			{
				if(!ti.id.empty())
					ti.tex = resMgr.GetLoadedTexture(ti.id.c_str());
			}
		}
	}
	LoadItemsData();

	// sounds
	if(!nosound)
	{
		resMgr.AddTaskCategory(txLoadSounds);
		resMgr.GetLoadedSound("gulp.mp3", sGulp);
		resMgr.GetLoadedSound("moneta2.mp3", sCoins);
		resMgr.GetLoadedSound("bow1.mp3", sBow[0]);
		resMgr.GetLoadedSound("bow2.mp3", sBow[1]);
		resMgr.GetLoadedSound("drzwi-02.mp3", sDoor[0]);
		resMgr.GetLoadedSound("drzwi-03.mp3", sDoor[1]);
		resMgr.GetLoadedSound("drzwi-04.mp3", sDoor[2]);
		resMgr.GetLoadedSound("104528__skyumori__door-close-sqeuak-02.mp3", sDoorClose);
		resMgr.GetLoadedSound("wont_budge.ogg", sDoorClosed[0]);
		resMgr.GetLoadedSound("wont_budge2.ogg", sDoorClosed[1]);
		resMgr.GetLoadedSound("bottle.wav", sItem[0]); // potion
		resMgr.GetLoadedSound("armor-light.wav", sItem[1]); // light armor
		resMgr.GetLoadedSound("chainmail1.wav", sItem[2]); // heavy armor
		resMgr.GetLoadedSound("metal-ringing.wav", sItem[3]); // crystal
		resMgr.GetLoadedSound("wood-small.wav", sItem[4]); // bow
		resMgr.GetLoadedSound("cloth-heavy.wav", sItem[5]); // shield
		resMgr.GetLoadedSound("sword-unsheathe.wav", sItem[6]); // weapon
		resMgr.GetLoadedSound("interface3.wav", sItem[7]);
		resMgr.GetLoadedSound("hello-3.mp3", sTalk[0]);
		resMgr.GetLoadedSound("hello-4.mp3", sTalk[1]);
		resMgr.GetLoadedSound("hmph.wav", sTalk[2]);
		resMgr.GetLoadedSound("huh-2.mp3", sTalk[3]);
		resMgr.GetLoadedSound("chest_open.mp3", sChestOpen);
		resMgr.GetLoadedSound("chest_close.mp3", sChestClose);
		resMgr.GetLoadedSound("door_budge.mp3", sDoorBudge);
		resMgr.GetLoadedSound("atak_kamien.mp3", sRock);
		resMgr.GetLoadedSound("atak_drewno.mp3", sWood);
		resMgr.GetLoadedSound("atak_krysztal.mp3", sCrystal);
		resMgr.GetLoadedSound("atak_metal.mp3", sMetal);
		resMgr.GetLoadedSound("atak_cialo.mp3", sBody[0]);
		resMgr.GetLoadedSound("atak_cialo2.mp3", sBody[1]);
		resMgr.GetLoadedSound("atak_cialo3.mp3", sBody[2]);
		resMgr.GetLoadedSound("atak_cialo4.mp3", sBody[3]);
		resMgr.GetLoadedSound("atak_cialo5.mp3", sBody[4]);
		resMgr.GetLoadedSound("atak_kosci.mp3", sBone);
		resMgr.GetLoadedSound("atak_skora.mp3", sSkin);
		resMgr.GetLoadedSound("arena_fight.mp3", sArenaFight);
		resMgr.GetLoadedSound("arena_wygrana.mp3", sArenaWin);
		resMgr.GetLoadedSound("arena_porazka.mp3", sArenaLost);
		resMgr.GetLoadedSound("unlock.mp3", sUnlock);
		resMgr.GetLoadedSound("TouchofDeath.ogg", sEvil);
		resMgr.GetLoadedSound("shade8.wav", sXarTalk);
		resMgr.GetLoadedSound("ogre1.wav", sOrcTalk);
		resMgr.GetLoadedSound("goblin-7.wav", sGoblinTalk);
		resMgr.GetLoadedSound("golem_alert.mp3", sGolemTalk);
		resMgr.GetLoadedSound("eat.mp3", sEat);
	}

	// musics
	if(!nomusic)
		LoadMusic(MusicType::Title);
}
