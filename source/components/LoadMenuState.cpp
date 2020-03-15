#include "Pch.h"
#include "LoadMenuState.h"

#include <BasicShader.h>
#include <Engine.h>
#include <GlowShader.h>
#include <GrassShader.h>
#include <ParticleShader.h>
#include <PostfxShader.h>
#include <Render.h>
#include <ResourceManager.h>
#include <SkyboxShader.h>
#include <SoundManager.h>
#include <TerrainShader.h>

#include "Arena.h"
#include "CommandParser.h"
#include "Content.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "ItemSlot.h"
#include "Language.h"
#include "Level.h"
#include "LoadScreen.h"
#include "LocationGeneratorFactory.h"
#include "NameHelper.h"
#include "Notifications.h"
#include "QuestManager.h"
#include "ScriptManager.h"
#include "Unit.h"
#include "World.h"

extern string g_system_dir;

//=================================================================================================
bool LoadMenuState::OnEnter()
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
		engine->ShowError(Format("Game: Failed to initialize game: %s", err), Logger::L_FATAL);
		return false;
	}
}

//=================================================================================================
// Preconfigure game vars.
//=================================================================================================
void LoadMenuState::PreconfigureGame()
{
	Info("Game: Preconfiguring game.");

	phy_world = engine->GetPhysicsWorld();
	engine->UnlockCursor(false);

	// set animesh callback
	MeshInstance::Predraw = Unit::Predraw;

	game_gui->PreInit();
	load_screen = game_gui->load_screen;

	PreloadLanguage();
	PreloadData();
	res_mgr->SetProgressCallback(ProgressCallback(this, &Game::OnLoadProgress));
}

//=================================================================================================
// Load language strings showed on load screen.
//=================================================================================================
void LoadMenuState::PreloadLanguage()
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
void LoadMenuState::PreloadData()
{
	res_mgr->AddDir("data/preload");

	GameGui::font = game_gui->gui->CreateFont("Arial", 12, 800, 2);

	// loadscreen textures
	load_screen->LoadData();

	// intro music
	if(!sound_mgr->IsMusicDisabled())
	{
		Ptr<MusicTrack> track;
		track->music = res_mgr->Load<Music>("Intro.ogg");
		track->type = MusicType::Intro;
		MusicTrack::tracks.push_back(track.Pin());
		game->SetMusic(MusicType::Intro);
	}
}

//=================================================================================================
// Load system.
//=================================================================================================
void LoadMenuState::LoadSystem()
{
	Info("Game: Loading system.");
	load_screen->Setup(0.f, 0.33f, 16, txCreatingListOfFiles);

	AddFilesystem();
	game->arena->Init();
	game_gui->Init();
	game_res->Init();
	net->Init();
	quest_mgr->Init();
	script_mgr->Init();
	LoadDatafiles();
	LoadLanguageFiles();
	load_screen->Tick(txLoadingShaders);
	ConfigureGame();
}

//=================================================================================================
// Add filesystem.
//=================================================================================================
void LoadMenuState::AddFilesystem()
{
	Info("Game: Creating list of files.");
	res_mgr->AddDir("data");
	res_mgr->AddPak("data/data.pak", "KrystaliceFire");
}

//=================================================================================================
// Load datafiles (items/units/etc).
//=================================================================================================
void LoadMenuState::LoadDatafiles()
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
			load_screen->Tick(txLoadingAbilities);
			break;
		case Content::Id::Buildings:
			load_screen->Tick(txLoadingBuildings);
			break;
		case Content::Id::Classes:
			load_screen->Tick(txLoadingClasses);
			break;
		case Content::Id::Dialogs:
			load_screen->Tick(txLoadingDialogs);
			break;
		case Content::Id::Items:
			load_screen->Tick(txLoadingItems);
			break;
		case Content::Id::Locations:
			load_screen->Tick(txLoadingLocations);
			break;
		case Content::Id::Musics:
			load_screen->Tick(txLoadingMusics);
			break;
		case Content::Id::Objects:
			load_screen->Tick(txLoadingObjects);
			break;
		case Content::Id::Perks:
			load_screen->Tick(txLoadingPerks);
			break;
		case Content::Id::Quests:
			load_screen->Tick(txLoadingQuests);
			break;
		case Content::Id::Required:
			load_screen->Tick(txLoadingRequired);
			break;
		case Content::Id::Units:
			load_screen->Tick(txLoadingUnits);
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
void LoadMenuState::LoadLanguageFiles()
{
	Info("Game: Loading language files.");
	load_screen->Tick(txLoadingLanguageFiles);

	Language::LoadLanguageFiles();

	txHaveErrors = Str("haveErrors");
	SetGameCommonText();
	SetItemStatsText();
	NameHelper::SetHeroNames();
	game->SetGameText();
	WeaponTypeInfo::LoadLanguage();
	game->arena->LoadLanguage();
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
void LoadMenuState::ConfigureGame()
{
	Info("Game: Configuring game.");
	load_screen->Tick(txConfiguringGame);

	game->InitScene();
	cmdp->AddCommands();
	game->settings.ResetGameKeys();
	game->settings.LoadGameKeys(game->cfg);
	load_errors += BaseLocation::SetRoomPointers();

	// shaders
	render->RegisterShader(game->basic_shader = new BasicShader);
	render->RegisterShader(game->grass_shader = new GrassShader);
	render->RegisterShader(game->glow_shader = new GlowShader);
	render->RegisterShader(game->particle_shader = new ParticleShader);
	render->RegisterShader(game->postfx_shader = new PostfxShader);
	render->RegisterShader(game->skybox_shader = new SkyboxShader);
	render->RegisterShader(game->terrain_shader = new TerrainShader);

	game->tMinimap = render->CreateDynamicTexture(Int2(128, 128));
	CreateRenderTargets();
}

//=================================================================================================
// Load game data.
//=================================================================================================
void LoadMenuState::LoadData()
{
	Info("Game: Loading data.");

	res_mgr->PrepareLoadScreen(0.33f);
	load_screen->Tick(txPreloadAssets);
	game_res->LoadData();
	res_mgr->StartLoadScreen();
}

//=================================================================================================
void LoadMenuState::CreateRenderTargets()
{
	game->rt_save = render->CreateRenderTarget(Int2(256, 256));
	game->rt_item_rot = render->CreateRenderTarget(Int2(128, 128));
}

//=================================================================================================
// Configure game after loading content.
//=================================================================================================
void LoadMenuState::PostconfigureGame()
{
	Info("Game: Postconfiguring game.");

	engine->LockCursor();
	game_gui->PostInit();
	game_level->Init();
	game->loc_gen_factory->Init();
	world->Init();
	quest_mgr->InitLists();

	// create save folders
	io::CreateDirectory("saves");
	io::CreateDirectory("saves/single");
	io::CreateDirectory("saves/multi");

	ItemScript::Init();

	// test & validate game data (in debug always check some things)
	if(game->settings.testing)
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
	game->cfg.Add("adapter", render->GetAdapter());
	game->cfg.Add("resolution", engine->GetWindowSize());
	game->cfg.Add("refresh", render->GetRefreshRate());
	game->SaveCfg();

	// end load screen, show menu
	/*clear_color = Color::Black;
	game_state = GS_MAIN_MENU;
	load_screen->visible = false;
	game_gui->main_menu->visible = true;
	if(music_type != MusicType::Intro)
		SetMusic(MusicType::Title);

	// start game mode if selected quickmode
	if(start_game_mode)
		StartGameMode();*/
}

//=================================================================================================
uint LoadMenuState::ValidateGameData(bool major)
{
	Info("Test: Validating game data...");

	uint err = TestGameData(major);

	UnitData::Validate(err);
	Attribute::Validate(err);
	Skill::Validate(err);
	Item::Validate(err);
	Perk::Validate(err);

	if(err == 0)
		Info("Test: Validation succeeded.");
	else
		Error("Test: Validation failed, %u errors found.", err);

	content.warnings += err;
	return err;
}

//=================================================================================================
uint LoadMenuState::TestGameData(bool major)
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
