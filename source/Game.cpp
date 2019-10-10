#include "Pch.h"
#include "GameCore.h"
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
#include "Spell.h"
#include "Team.h"
#include "Action.h"
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
#include "DebugDrawer.h"
#include "LobbyApi.h"
#include "ServerPanel.h"
#include "GameMenu.h"
#include "Language.h"
#include "CommandParser.h"
#include "Controls.h"
#include "GameResources.h"

const float LIMIT_DT = 0.3f;
Game* global::game;
CustomCollisionWorld* global::phy_world;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;

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
Game::Game() : vbParticle(nullptr), quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0), draw_particle_sphere(false),
draw_unit_radius(false), draw_hitbox(false), noai(false), testing(false), game_speed(1.f), devmode(false), force_seed(0), next_seed(0), force_seed_all(false),
dont_wander(false), check_updates(true), skip_tutorial(false), portal_anim(0), music_type(MusicType::None), end_of_game(false), prepared_stream(64 * 1024),
paused(false), draw_flags(0xFFFFFFFF), prev_game_state(GS_LOAD), rt_save(nullptr), rt_item_rot(nullptr), cl_postfx(true), mp_timeout(10.f), cl_normalmap(true),
cl_specularmap(true), dungeon_tex_wrap(true), profiler_mode(ProfilerMode::Disabled), screenshot_format(ImageFormat::JPG), game_state(GS_LOAD),
default_devmode(false), default_player_devmode(false), quickstart_slot(SaveSlot::MAX_SLOTS), clear_color(Color::Black), engine(new Engine)
{
#ifdef _DEBUG
	default_devmode = true;
	default_player_devmode = true;
#endif
	devmode = default_devmode;

	dialog_context.is_local = true;

	ClearPointers();

	uv_mod = Terrain::DEFAULT_UV_MOD;

	SetupConfigVars();
	BeforeInit();
}

//=================================================================================================
Game::~Game()
{
	delete engine;
}

//=================================================================================================
void Game::OnDraw()
{
	if(profiler_mode == ProfilerMode::Rendering)
		Profiler::g_profiler.Start();
	else if(profiler_mode == ProfilerMode::Disabled)
		Profiler::g_profiler.Clear();

	DrawGame(nullptr);

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
	IDirect3DDevice9* device = render->GetDevice();

	if(post_effects.empty() || !ePostFx)
	{
		if(target)
			render->SetTarget(target);
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));
		V(device->BeginScene());

		if(game_state == GS_LEVEL)
		{
			// draw level
			Draw();

			// draw glow
			if(!draw_batch.glow_nodes.empty())
			{
				V(device->EndScene());
				DrawGlowingNodes(false);
				V(device->BeginScene());
			}

			// debug draw
			debug_drawer->Draw();
		}

		// draw gui
		game_gui->Draw(game_level->camera.matViewProj, IsSet(draw_flags, DF_GUI), IsSet(draw_flags, DF_MENU));

		V(device->EndScene());
		if(target)
			render->SetTarget(nullptr);
	}
	else
	{
		// render scene to texture
		SURFACE sPost;
		if(!render->IsMultisamplingEnabled())
			V(tPostEffect[2]->GetSurfaceLevel(0, &sPost));
		else
			sPost = sPostEffect[2];

		V(device->SetRenderTarget(0, sPost));
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));

		if(game_state == GS_LEVEL)
		{
			V(device->BeginScene());
			Draw();
			V(device->EndScene());
			if(!draw_batch.glow_nodes.empty())
				DrawGlowingNodes(true);

			// debug draw
			debug_drawer->Draw();
		}

		PROFILER_BLOCK("PostEffects");

		TEX t;
		if(!render->IsMultisamplingEnabled())
		{
			sPost->Release();
			t = tPostEffect[2];
		}
		else
		{
			SURFACE surf2;
			V(tPostEffect[0]->GetSurfaceLevel(0, &surf2));
			V(device->StretchRect(sPost, nullptr, surf2, nullptr, D3DTEXF_NONE));
			surf2->Release();
			t = tPostEffect[0];
		}

		// post effects
		V(device->SetVertexDeclaration(render->GetVertexDeclaration(VDI_TEX)));
		V(device->SetStreamSource(0, vbFullscreen, 0, sizeof(VTex)));
		render->SetAlphaTest(false);
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
					V(tPostEffect[index_surf]->GetSurfaceLevel(0, &surf));
				else
					surf = sPostEffect[index_surf];
			}

			V(device->SetRenderTarget(0, surf));
			V(device->BeginScene());

			V(ePostFx->SetTechnique(it->tech));
			V(ePostFx->SetTexture(hPostTex, t));
			V(ePostFx->SetFloat(hPostPower, it->power));
			V(ePostFx->SetVector(hPostSkill, (D3DXVECTOR4*)&it->skill));

			V(ePostFx->Begin(&passes, 0));
			V(ePostFx->BeginPass(0));
			V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
			V(ePostFx->EndPass());
			V(ePostFx->End());

			if(it + 1 == end)
				game_gui->Draw(game_level->camera.matViewProj, IsSet(draw_flags, DF_GUI), IsSet(draw_flags, DF_MENU));

			V(device->EndScene());

			if(it + 1 == end)
			{
				if(!target)
					surf->Release();
			}
			else if(!render->IsMultisamplingEnabled())
			{
				surf->Release();
				t = tPostEffect[index_surf];
			}
			else
			{
				SURFACE surf2;
				V(tPostEffect[0]->GetSurfaceLevel(0, &surf2));
				V(device->StretchRect(surf, nullptr, surf2, nullptr, D3DTEXF_NONE));
				surf2->Release();
				t = tPostEffect[0];
			}

			index_surf = (index_surf + 1) % 3;
		}
	}
}

//=================================================================================================
void Game::OnDebugDraw(DebugDrawer* dd)
{
	if(pathfinding->IsDebugDraw())
	{
		dd->SetCamera(game_level->camera);
		pathfinding->Draw(dd);
	}
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
		mat[bone] = Matrix::RotationX(val / 5) *mat[bone];
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

	// lost directx device or window don't have focus
	if(render->IsLostDevice() || !engine->IsActive() || !engine->IsCursorLocked())
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
	}

	// handle panels
	if(gui->HaveDialog() || (game_gui->mp_box->visible && game_gui->mp_box->itb.focus))
		GKey.allow_input = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode && !cutscene)
	{
		OpenPanel open = game_gui->level_gui->GetOpenPanel(),
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
		else if(GKey.PressedRelease(GK_ACTION_PANEL))
			to_open = OpenPanel::Action;
		else if(open == OpenPanel::Trade && input->PressedRelease(Key::Escape))
			to_open = OpenPanel::Trade; // ShowPanel hide when already opened

		if(to_open != OpenPanel::None)
			game_gui->level_gui->ShowPanel(to_open, open);

		switch(open)
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(game_gui->level_gui->use_cursor)
				GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::Inventory:
		case OpenPanel::Team:
		case OpenPanel::Trade:
		case OpenPanel::Action:
		case OpenPanel::Journal:
		case OpenPanel::Book:
			GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		}
	}

	if(!cutscene)
	{
		// quicksave, quickload
		bool console_open = gui->HaveTopDialog("console");
		bool special_key_allowed = (GKey.allow_input == GameKeys::ALLOW_KEYBOARD || GKey.allow_input == GameKeys::ALLOW_INPUT || (!gui->HaveDialog() || console_open));
		if(GKey.KeyPressedReleaseSpecial(GK_QUICKSAVE, special_key_allowed))
			Quicksave(console_open);
		if(GKey.KeyPressedReleaseSpecial(GK_QUICKLOAD, special_key_allowed))
			Quickload(console_open);

		// mp box
		if(game_state == GS_LEVEL && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
			game_gui->mp_box->visible = !game_gui->mp_box->visible;
	}

	// update game_gui
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

	// handle blocking input by game_gui
	if(gui->HaveDialog() || (game_gui->mp_box->visible && game_gui->mp_box->itb.focus))
		GKey.allow_input = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode)
	{
		switch(game_gui->level_gui->GetOpenPanel())
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(game_gui->level_gui->use_cursor)
				GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::Inventory:
		case OpenPanel::Team:
		case OpenPanel::Trade:
		case OpenPanel::Action:
			GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Journal:
			GKey.allow_input = GameKeys::ALLOW_NONE;
			break;
		}
	}
	else
		GKey.allow_input = GameKeys::ALLOW_INPUT;

	// open game menu
	if(GKey.AllowKeyboard() && CanShowMenu() && !cutscene && input->PressedRelease(Key::Escape))
		game_gui->ShowMenu();

	// update game
	if(!end_of_game && !cutscene)
	{
		arena->UpdatePvpRequest(dt);

		if(game_state == GS_LEVEL)
		{
			if(paused)
			{
				UpdateFallback(dt);
				if(Net::IsLocal())
				{
					if(Net::IsOnline())
						net->UpdateWarpData(dt);
					game_level->ProcessUnitWarps();
				}
				SetupCamera(dt);
				if(Net::IsOnline())
					UpdateGameNet(dt);
			}
			else if(gui->HavePauseDialog())
			{
				if(Net::IsOnline())
					UpdateGame(dt);
				else
				{
					UpdateFallback(dt);
					if(Net::IsLocal())
					{
						if(Net::IsOnline())
							net->UpdateWarpData(dt);
						game_level->ProcessUnitWarps();
					}
					SetupCamera(dt);
				}
			}
			else
				UpdateGame(dt);
		}
		else if(game_state == GS_WORLDMAP)
		{
			if(Net::IsOnline())
				UpdateGameNet(dt);
		}

		if(Net::IsSingleplayer() && game_state != GS_MAIN_MENU)
		{
			assert(Net::changes.empty());
		}
	}
	else
	{
		if(cutscene)
			UpdateFallback(dt);
		if(Net::IsOnline())
			UpdateGameNet(dt);
	}

	// open/close mp box
	if(!cutscene && GKey.AllowKeyboard() && game_state == GS_LEVEL && game_gui->mp_box->visible && !game_gui->mp_box->itb.focus && input->PressedRelease(Key::Enter))
	{
		game_gui->mp_box->itb.focus = true;
		game_gui->mp_box->Event(GuiEvent_GainFocus);
		game_gui->mp_box->itb.Event(GuiEvent_GainFocus);
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

	if((game_state != GS_MAIN_MENU && game_state != GS_LOAD) || (game_gui->server && game_gui->server->visible))
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
bool Game::Start()
{
	LocalString s;
	GetTitle(s);
	engine->SetTitle(s.c_str());
	return engine->Start(this);
}

//=================================================================================================
void Game::OnReload()
{
	if(eMesh)
		V(eMesh->OnResetDevice());
	if(eParticle)
		V(eParticle->OnResetDevice());
	if(eSkybox)
		V(eSkybox->OnResetDevice());
	if(eArea)
		V(eArea->OnResetDevice());
	if(ePostFx)
		V(ePostFx->OnResetDevice());
	if(eGlow)
		V(eGlow->OnResetDevice());

	CreateTextures();
	BuildDungeon();
	// rebuild minimap texture
	if(game_state == GS_LEVEL)
		loc_gen_factory->Get(game_level->location)->CreateMinimap();
	if(game_gui && game_gui->inventory)
		game_gui->inventory->OnReload();
}

//=================================================================================================
void Game::OnReset()
{
	if(game_gui && game_gui->inventory)
		game_gui->inventory->OnReset();

	if(eMesh)
		V(eMesh->OnLostDevice());
	if(eParticle)
		V(eParticle->OnLostDevice());
	if(eSkybox)
		V(eSkybox->OnLostDevice());
	if(eArea)
		V(eArea->OnLostDevice());
	if(ePostFx)
		V(ePostFx->OnLostDevice());
	if(eGlow)
		V(eGlow->OnLostDevice());

	SafeRelease(tMinimap.tex);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}
	SafeRelease(vbFullscreen);
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
}

//=================================================================================================
void Game::TakeScreenshot(bool no_gui)
{
	if(no_gui)
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
	}
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
// to mog³o by byæ w konstruktorze ale za du¿o tego
void Game::ClearPointers()
{
	// shadery
	eMesh = nullptr;
	eParticle = nullptr;
	eSkybox = nullptr;
	eArea = nullptr;
	ePostFx = nullptr;
	eGlow = nullptr;

	// bufory wierzcho³ków i indeksy
	vbParticle = nullptr;
	vbDungeon = nullptr;
	ibDungeon = nullptr;
	vbFullscreen = nullptr;

	// tekstury render target, powierzchnie
	for(int i = 0; i < 3; ++i)
	{
		sPostEffect[i] = nullptr;
		tPostEffect[i] = nullptr;
	}

	// vertex data
	vdStairsUp = nullptr;
	vdStairsDown = nullptr;
	vdDoorHole = nullptr;
}

//=================================================================================================
void Game::OnCleanup()
{
	if(game_state != GS_QUIT && game_state != GS_LOAD_MENU)
		ClearGame();

	if(game_gui && game_gui->main_menu)
		game_gui->main_menu->ShutdownThread();

	content.CleanupContent();

	delete arena;
	delete cmdp;
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

	ClearQuadtree();

	// shadery
	ReleaseShaders();

	// bufory wierzcho³ków i indeksy
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
	SafeRelease(vbFullscreen);

	// tekstury render target, powierzchnie
	SafeRelease(tMinimap.tex);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}

	draw_batch.Clear();

	Language::Cleanup();
}

//=================================================================================================
void Game::CreateTextures()
{
	if(tMinimap.tex)
		return;

	IDirect3DDevice9* device = render->GetDevice();
	const Int2& wnd_size = engine->GetWindowSize();

	V(device->CreateTexture(128, 128, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tMinimap.tex, nullptr));
	tMinimap.state = ResourceState::Loaded;

	int ms, msq;
	render->GetMultisampling(ms, msq);
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)ms;
	if(ms != D3DMULTISAMPLE_NONE)
	{
		for(int i = 0; i < 3; ++i)
		{
			V(device->CreateRenderTarget(wnd_size.x, wnd_size.y, D3DFMT_X8R8G8B8, type, msq, FALSE, &sPostEffect[i], nullptr));
			tPostEffect[i] = nullptr;
		}
		V(device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tPostEffect[0], nullptr));
	}
	else
	{
		for(int i = 0; i < 3; ++i)
		{
			V(device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tPostEffect[i], nullptr));
			sPostEffect[i] = nullptr;
		}
	}

	// fullscreen vertexbuffer
	VTex* v;
	V(device->CreateVertexBuffer(sizeof(VTex) * 6, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbFullscreen, nullptr));
	V(vbFullscreen->Lock(0, sizeof(VTex) * 6, (void**)&v, 0));

	// coœ mi siê obi³o o uszy z tym pó³ teksela przy renderowaniu
	// ale szczegó³ów nie znam
	const float u_start = 0.5f / wnd_size.x;
	const float u_end = 1.f + 0.5f / wnd_size.x;
	const float v_start = 0.5f / wnd_size.y;
	const float v_end = 1.f + 0.5f / wnd_size.y;

	v[0] = VTex(-1.f, 1.f, 0.f, u_start, v_start);
	v[1] = VTex(1.f, 1.f, 0.f, u_end, v_start);
	v[2] = VTex(1.f, -1.f, 0.f, u_end, v_end);
	v[3] = VTex(1.f, -1.f, 0.f, u_end, v_end);
	v[4] = VTex(-1.f, -1.f, 0.f, u_start, v_end);
	v[5] = VTex(-1.f, 1.f, 0.f, u_start, v_start);

	V(vbFullscreen->Unlock());
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
	txLoadingLocations = Str("loadingLocations");
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

	TakenPerk::LoadText();
}

//=================================================================================================
void Game::UpdateLights(vector<Light>& lights)
{
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
	{
		Light& s = *it;
		s.t_pos = s.pos + Vec3::Random(Vec3(-0.05f, -0.05f, -0.05f), Vec3(0.05f, 0.05f, 0.05f));
		s.t_color = (s.color + Vec3::Random(Vec3(-0.1f, -0.1f, -0.1f), Vec3(0.1f, 0.1f, 0.1f))).Clamped();
	}
}

//=================================================================================================
void Game::UpdatePostEffects(float dt)
{
	post_effects.clear();
	if(!cl_postfx || game_state != GS_LEVEL)
		return;

	// szarzenie
	if(pc->unit->IsAlive())
		grayout = max(grayout - dt, 0.f);
	else
		grayout = min(grayout + dt, 1.f);
	if(grayout > 0.f)
	{
		PostEffect& e = Add1(post_effects);
		e.tech = ePostFx->GetTechniqueByName("Monochrome");
		e.power = grayout;
	}

	// upicie
	float drunk = pc->unit->alcohol / pc->unit->hpmax;
	if(drunk > 0.1f)
	{
		PostEffect* e, *e2;
		post_effects.resize(post_effects.size() + 2);
		e = &*(post_effects.end() - 2);
		e2 = &*(post_effects.end() - 1);

		e->id = e2->id = 0;
		e->tech = ePostFx->GetTechniqueByName("BlurX");
		e2->tech = ePostFx->GetTechniqueByName("BlurY");
		// 0.1-0.5 - 1
		// 1 - 2
		float mod;
		if(drunk < 0.5f)
			mod = 1.f;
		else
			mod = 1.f + (drunk - 0.5f) * 2;
		e->skill = e2->skill = Vec4(1.f / engine->GetWindowSize().x*mod, 1.f / engine->GetWindowSize().y*mod, 0, 0);
		// 0.1-0
		// 1-1
		e->power = e2->power = (drunk - 0.1f) / 0.9f;
	}
}

//=================================================================================================
void Game::ReloadShaders()
{
	Info("Reloading shaders...");

	ReleaseShaders();

	try
	{
		eMesh = render->CompileShader("mesh.fx");
		eParticle = render->CompileShader("particle.fx");
		eSkybox = render->CompileShader("skybox.fx");
		eArea = render->CompileShader("area.fx");
		ePostFx = render->CompileShader("post.fx");
		eGlow = render->CompileShader("glow.fx");

		for(ShaderHandler* shader : render->GetShaders())
			shader->OnInit();
	}
	catch(cstring err)
	{
		engine->FatalError(Format("Failed to reload shaders.\n%s", err));
		return;
	}

	SetupShaders();
}

//=================================================================================================
void Game::ReleaseShaders()
{
	SafeRelease(eMesh);
	SafeRelease(eParticle);
	SafeRelease(eSkybox);
	SafeRelease(eArea);
	SafeRelease(ePostFx);
	SafeRelease(eGlow);

	for(ShaderHandler* shader : render->GetShaders())
		shader->OnRelease();
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
	PerkInfo::Validate(err);
	RoomType::Validate(err);

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
		return Format("%s (%s, %s)", action, game_gui->controls->key_text[(int)k[0]], game_gui->controls->key_text[(int)k[1]]);
	else if(first_key || second_key)
	{
		Key used = k[first_key ? 0 : 1];
		return Format("%s (%s)", action, game_gui->controls->key_text[(int)used]);
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

	game_gui->world_map->Hide();
	game_gui->level_gui->Reset();
	game_gui->level_gui->visible = true;

	const bool reenter = game_level->is_open;
	game_level->is_open = true;
	game_level->reenter = reenter;
	if(world->GetState() != World::State::INSIDE_ENCOUNTER)
		world->SetState(World::State::INSIDE_LOCATION);
	if(from_portal != -1)
		game_level->enter_from = ENTER_FROM_PORTAL + from_portal;
	else
		game_level->enter_from = ENTER_FROM_OUTSIDE;
	if(!reenter)
		game_level->light_angle = Random(PI * 2);

	game_level->dungeon_level = level;
	game_level->event_handler = nullptr;
	pc->data.before_player = BP_NONE;
	arena->Reset();
	game_gui->inventory->lock = nullptr;

	bool first = false;

	if(l.last_visit == -1)
		first = true;

	if(!reenter)
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
	LocationGenerator* loc_gen = loc_gen_factory->Get(&l, first, reenter);
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
		if(!reenter)
		{
			SetTerrainTextures();
			CalculateQuadtree();
		}
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
void Game::OnFocus(bool focus, const Int2& activation_point)
{
	game_gui->OnFocus(focus, activation_point);
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
void Game::SetupCamera(float dt)
{
	Unit* target = pc->unit;
	LevelArea& area = *target->area;

	float rotX;
	if(game_level->camera.free_rot)
		rotX = game_level->camera.real_rot.x;
	else
		rotX = target->rot;

	game_level->camera.UpdateRot(dt, Vec2(rotX, game_level->camera.real_rot.y));

	Matrix mat, matProj, matView;
	const Vec3 cam_h(0, target->GetUnitHeight() + 0.2f, 0);
	Vec3 dist(0, -game_level->camera.tmp_dist, 0);

	mat = Matrix::Rotation(game_level->camera.rot.y, game_level->camera.rot.x, 0);
	dist = Vec3::Transform(dist, mat);

	// !!! to => from !!!
	// kamera idzie od g³owy do ty³u
	Vec3 to = target->pos + cam_h;
	float tout, min_tout = 2.f;

	int tx = int(target->pos.x / 2),
		tz = int(target->pos.z / 2);

	if(area.area_type == LevelArea::Type::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)game_level->location;

		// terrain
		tout = game_level->terrain->Raytest(to, to + dist);
		if(tout < min_tout && tout > 0.f)
			min_tout = tout;

		// buildings
		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(OutsideLocation::size - 1, tx + 3),
			maxz = min(OutsideLocation::size - 1, tz + 3);
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				if(outside->tiles[x + z * OutsideLocation::size].IsBlocking())
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 8.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
			}
		}
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		InsideLocation* inside = (InsideLocation*)game_level->location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(lvl.w - 1, tx + 3),
			maxz = min(lvl.h - 1, tz + 3);

		// ceil
		const Plane sufit(0, -1, 0, 4);
		if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
		{
			//tmpvar2 = 1;
			min_tout = tout;
		}

		// floor
		const Plane podloga(0, 1, 0, 0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// dungeon
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				Tile& p = lvl.map[x + z * lvl.w];
				if(IsBlocking(p.type))
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				else if(IsSet(p.flags, Tile::F_LOW_CEILING))
				{
					const Box box(float(x) * 2, 3.f, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				if(p.type == STAIRS_UP)
				{
					if(vdStairsUp->RayToMesh(to, dist, PtToPos(lvl.staircase_up), DirToRot(lvl.staircase_up_dir), tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == STAIRS_DOWN)
				{
					if(!lvl.staircase_down_in_wall
						&& vdStairsDown->RayToMesh(to, dist, PtToPos(lvl.staircase_down), DirToRot(lvl.staircase_down_dir), tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == DOORS || p.type == HOLE_FOR_DOORS)
				{
					Vec3 pos(float(x * 2) + 1, 0, float(z * 2) + 1);
					float rot;

					if(IsBlocking(lvl.map[x - 1 + z * lvl.w].type))
					{
						rot = 0;
						int mov = 0;
						if(lvl.rooms[lvl.map[x + (z - 1)*lvl.w].room]->IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + (z + 1)*lvl.w].room]->IsCorridor())
							--mov;
						if(mov == 1)
							pos.z += 0.8229f;
						else if(mov == -1)
							pos.z -= 0.8229f;
					}
					else
					{
						rot = PI / 2;
						int mov = 0;
						if(lvl.rooms[lvl.map[x - 1 + z * lvl.w].room]->IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + 1 + z * lvl.w].room]->IsCorridor())
							--mov;
						if(mov == 1)
							pos.x += 0.8229f;
						else if(mov == -1)
							pos.x -= 0.8229f;
					}

					if(vdDoorHole->RayToMesh(to, dist, pos, rot, tout) && tout < min_tout)
						min_tout = tout;

					Door* door = area.FindDoor(Int2(x, z));
					if(door && door->IsBlocking())
					{
						Box box(pos.x, 0.f, pos.z);
						box.v2.y = Door::HEIGHT * 2;
						if(rot == 0.f)
						{
							box.v1.x -= Door::WIDTH;
							box.v2.x += Door::WIDTH;
							box.v1.z -= Door::THICKNESS;
							box.v2.z += Door::THICKNESS;
						}
						else
						{
							box.v1.z -= Door::WIDTH;
							box.v2.z += Door::WIDTH;
							box.v1.x -= Door::THICKNESS;
							box.v2.x += Door::THICKNESS;
						}

						if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
							min_tout = tout;
					}
				}
			}
		}
	}
	else
	{
		// building
		InsideBuilding& building = static_cast<InsideBuilding&>(area);

		// ceil
		if(building.top > 0.f)
		{
			const Plane sufit(0, -1, 0, 4);
			if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}

		// floor
		const Plane podloga(0, 1, 0, 0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// xsphere
		if(building.xsphere_radius > 0.f)
		{
			Vec3 from = to + dist;
			if(RayToSphere(from, -dist, building.xsphere_pos, building.xsphere_radius, tout) && tout > 0.f)
			{
				tout = 1.f - tout;
				if(tout < min_tout)
					min_tout = tout;
			}
		}

		// doors
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box box(door.pos);
				box.v2.y = Door::HEIGHT * 2;
				if(door.rot == 0.f)
				{
					box.v1.x -= Door::WIDTH;
					box.v2.x += Door::WIDTH;
					box.v1.z -= Door::THICKNESS;
					box.v2.z += Door::THICKNESS;
				}
				else
				{
					box.v1.z -= Door::WIDTH;
					box.v2.z += Door::WIDTH;
					box.v1.x -= Door::THICKNESS;
					box.v2.x += Door::THICKNESS;
				}

				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}

			if(vdDoorHole->RayToMesh(to, dist, door.pos, door.rot, tout) && tout < min_tout)
				min_tout = tout;
		}
	}

	// objects
	for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
	{
		if(!it->cam_collider)
			continue;

		if(it->type == CollisionObject::SPHERE)
		{
			if(RayToCylinder(to, to + dist, Vec3(it->pt.x, 0, it->pt.y), Vec3(it->pt.x, 32.f, it->pt.y), it->radius, tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
		else if(it->type == CollisionObject::RECTANGLE)
		{
			Box box(it->pt.x - it->w, 0.f, it->pt.y - it->h, it->pt.x + it->w, 32.f, it->pt.y + it->h);
			if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
		else
		{
			float w, h;
			if(Equal(it->rot, PI / 2) || Equal(it->rot, PI * 3 / 2))
			{
				w = it->h;
				h = it->w;
			}
			else
			{
				w = it->w;
				h = it->h;
			}

			Box box(it->pt.x - w, 0.f, it->pt.y - h, it->pt.x + w, 32.f, it->pt.y + h);
			if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
	}

	// camera colliders
	for(vector<CameraCollider>::iterator it = game_level->cam_colliders.begin(), end = game_level->cam_colliders.end(); it != end; ++it)
	{
		if(RayToBox(to, dist, it->box, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;
	}

	// uwzglêdnienie znear
	if(min_tout > 1.f || pc->noclip)
		min_tout = 1.f;
	else if(min_tout < 0.1f)
		min_tout = 0.1f;

	float real_dist = dist.Length() * min_tout - 0.1f;
	if(real_dist < 0.01f)
		real_dist = 0.01f;
	Vec3 from = to + dist.Normalize() * real_dist;

	game_level->camera.Update(dt, from, to);

	float drunk = pc->unit->alcohol / pc->unit->hpmax;
	float drunk_mod = (drunk > 0.1f ? (drunk - 0.1f) / 0.9f : 0.f);

	matView = Matrix::CreateLookAt(game_level->camera.from, game_level->camera.to);
	matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4 + sin(drunk_anim)*(PI / 16)*drunk_mod,
		engine->GetWindowAspect() * (1.f + sin(drunk_anim) / 10 * drunk_mod), 0.1f, game_level->camera.draw_range);
	game_level->camera.matViewProj = matView * matProj;
	game_level->camera.matViewInv = matView.Inverse();
	game_level->camera.center = game_level->camera.from;
	game_level->camera.frustum.Set(game_level->camera.matViewProj);

	// centrum dŸwiêku 3d
	sound_mgr->SetListenerPosition(target->GetHeadSoundPos(), Vec3(sin(target->rot + PI), 0, cos(target->rot + PI)));
}

//=================================================================================================
void Game::LoadShaders()
{
	Info("Loading shaders.");

	eMesh = render->CompileShader("mesh.fx");
	eParticle = render->CompileShader("particle.fx");
	eSkybox = render->CompileShader("skybox.fx");
	eArea = render->CompileShader("area.fx");
	ePostFx = render->CompileShader("post.fx");
	eGlow = render->CompileShader("glow.fx");

	SetupShaders();
}

//=================================================================================================
void Game::SetupShaders()
{
	techMesh = eMesh->GetTechniqueByName("mesh");
	techMeshDir = eMesh->GetTechniqueByName("mesh_dir");
	techMeshSimple = eMesh->GetTechniqueByName("mesh_simple");
	techMeshSimple2 = eMesh->GetTechniqueByName("mesh_simple2");
	techMeshExplo = eMesh->GetTechniqueByName("mesh_explo");
	techParticle = eParticle->GetTechniqueByName("particle");
	techTrail = eParticle->GetTechniqueByName("trail");
	techSkybox = eSkybox->GetTechniqueByName("skybox");
	techArea = eArea->GetTechniqueByName("area");
	techGlowMesh = eGlow->GetTechniqueByName("mesh");
	techGlowAni = eGlow->GetTechniqueByName("ani");
	assert(techMesh && techMeshDir && techMeshSimple && techMeshSimple2 && techMeshExplo && techParticle && techTrail && techSkybox && techArea
		&& techGlowMesh && techGlowAni);

	hMeshCombined = eMesh->GetParameterByName(nullptr, "matCombined");
	hMeshWorld = eMesh->GetParameterByName(nullptr, "matWorld");
	hMeshTex = eMesh->GetParameterByName(nullptr, "texDiffuse");
	hMeshFogColor = eMesh->GetParameterByName(nullptr, "fogColor");
	hMeshFogParam = eMesh->GetParameterByName(nullptr, "fogParam");
	hMeshTint = eMesh->GetParameterByName(nullptr, "tint");
	hMeshAmbientColor = eMesh->GetParameterByName(nullptr, "ambientColor");
	hMeshLightDir = eMesh->GetParameterByName(nullptr, "lightDir");
	hMeshLightColor = eMesh->GetParameterByName(nullptr, "lightColor");
	hMeshLights = eMesh->GetParameterByName(nullptr, "lights");
	assert(hMeshCombined && hMeshWorld && hMeshTex && hMeshFogColor && hMeshFogParam && hMeshTint && hMeshAmbientColor && hMeshLightDir && hMeshLightColor
		&& hMeshLights);

	hParticleCombined = eParticle->GetParameterByName(nullptr, "matCombined");
	hParticleTex = eParticle->GetParameterByName(nullptr, "tex0");
	assert(hParticleCombined && hParticleTex);

	hSkyboxCombined = eSkybox->GetParameterByName(nullptr, "matCombined");
	hSkyboxTex = eSkybox->GetParameterByName(nullptr, "tex0");
	assert(hSkyboxCombined && hSkyboxTex);

	hAreaCombined = eArea->GetParameterByName(nullptr, "matCombined");
	hAreaColor = eArea->GetParameterByName(nullptr, "color");
	hAreaPlayerPos = eArea->GetParameterByName(nullptr, "playerPos");
	hAreaRange = eArea->GetParameterByName(nullptr, "range");
	assert(hAreaCombined && hAreaColor && hAreaPlayerPos && hAreaRange);

	hPostTex = ePostFx->GetParameterByName(nullptr, "t0");
	hPostPower = ePostFx->GetParameterByName(nullptr, "power");
	hPostSkill = ePostFx->GetParameterByName(nullptr, "skill");
	assert(hPostTex && hPostPower && hPostSkill);

	hGlowCombined = eGlow->GetParameterByName(nullptr, "matCombined");
	hGlowBones = eGlow->GetParameterByName(nullptr, "matBones");
	hGlowColor = eGlow->GetParameterByName(nullptr, "color");
	hGlowTex = eGlow->GetParameterByName(nullptr, "texDiffuse");
	assert(hGlowCombined && hGlowBones && hGlowColor && hGlowTex);
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

	drunk_anim = Clip(drunk_anim + dt);
	UpdatePostEffects(dt);

	portal_anim += dt;
	if(portal_anim >= 1.f)
		portal_anim -= 1.f;
	game_level->light_angle = Clip(game_level->light_angle + dt / 100);

	UpdateFallback(dt);
	if(!game_level->location)
		return;

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
			text = txWin;

		gui->SimpleDialog(Format(text, pc->kills, game_stats->total_kills - pc->kills), nullptr);
	}

	// licznik otrzymanych obra¿eñ
	pc->last_dmg = 0.f;
	if(Net::IsLocal())
		pc->last_dmg_poison = 0.f;

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
						game_level->WarpUnit(*pc->unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
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
						game_level->WarpUnit(*pc->unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
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

	// obracanie kamery góra/dó³
	if(!Net::IsLocal() || team->IsAnyoneAlive())
	{
		if(dialog_context.dialog_mode && game_gui->inventory->mode <= I_INVENTORY)
		{
			game_level->camera.free_rot = false;
			if(game_level->camera.real_rot.y > 4.2875104f)
			{
				game_level->camera.real_rot.y -= dt;
				if(game_level->camera.real_rot.y < 4.2875104f)
					game_level->camera.real_rot.y = 4.2875104f;
			}
			else if(game_level->camera.real_rot.y < 4.2875104f)
			{
				game_level->camera.real_rot.y += dt;
				if(game_level->camera.real_rot.y > 4.2875104f)
					game_level->camera.real_rot.y = 4.2875104f;
			}
		}
		else
		{
			if(Any(GKey.allow_input, GameKeys::ALLOW_INPUT, GameKeys::ALLOW_MOUSE))
			{
				const float c_cam_angle_min = PI + 0.1f;
				const float c_cam_angle_max = PI * 1.8f - 0.1f;

				int div = (pc->unit->action == A_SHOOT ? 800 : 400);
				game_level->camera.real_rot.y += -float(input->GetMouseDif().y) * settings.mouse_sensitivity_f / div;
				if(game_level->camera.real_rot.y > c_cam_angle_max)
					game_level->camera.real_rot.y = c_cam_angle_max;
				if(game_level->camera.real_rot.y < c_cam_angle_min)
					game_level->camera.real_rot.y = c_cam_angle_min;

				if(!pc->unit->IsStanding())
				{
					game_level->camera.real_rot.x = Clip(game_level->camera.real_rot.x + float(input->GetMouseDif().x) * settings.mouse_sensitivity_f / 400);
					game_level->camera.free_rot = true;
					game_level->camera.free_rot_key = Key::None;
				}
				else if(!game_level->camera.free_rot)
				{
					game_level->camera.free_rot_key = GKey.KeyDoReturn(GK_ROTATE_CAMERA, &Input::Pressed);
					if(game_level->camera.free_rot_key != Key::None)
					{
						game_level->camera.real_rot.x = Clip(pc->unit->rot + PI);
						game_level->camera.free_rot = true;
					}
				}
				else
				{
					if(game_level->camera.free_rot_key == Key::None || GKey.KeyUpAllowed(game_level->camera.free_rot_key))
						game_level->camera.free_rot = false;
					else
						game_level->camera.real_rot.x = Clip(game_level->camera.real_rot.x + float(input->GetMouseDif().x) * settings.mouse_sensitivity_f / 400);
				}
			}
			else
				game_level->camera.free_rot = false;
		}
	}
	else
	{
		game_level->camera.free_rot = false;
		if(game_level->camera.real_rot.y > PI + 0.1f)
		{
			game_level->camera.real_rot.y -= dt;
			if(game_level->camera.real_rot.y < PI + 0.1f)
				game_level->camera.real_rot.y = PI + 0.1f;
		}
		else if(game_level->camera.real_rot.y < PI + 0.1f)
		{
			game_level->camera.real_rot.y += dt;
			if(game_level->camera.real_rot.y > PI + 0.1f)
				game_level->camera.real_rot.y = PI + 0.1f;
		}
	}

	// przybli¿anie/oddalanie kamery
	if(GKey.AllowMouse())
	{
		if(!dialog_context.dialog_mode || !dialog_context.show_choices || !game_gui->level_gui->IsMouseInsideDialog())
		{
			game_level->camera.dist -= input->GetMouseWheel();
			game_level->camera.dist = Clamp(game_level->camera.dist, 0.5f, 6.f);
		}

		if(input->PressedRelease(Key::MiddleButton))
			game_level->camera.dist = 3.5f;
	}

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
	if(Net::IsLocal())
	{
		if(Net::IsOnline())
			net->UpdateWarpData(dt);
		game_level->ProcessUnitWarps();
	}

	// usuñ jednostki
	game_level->ProcessRemoveUnits(false);

	if(Net::IsOnline())
	{
		UpdateGameNet(dt);
		if(!Net::IsOnline() || game_state != GS_LEVEL)
			return;
	}

	// aktualizuj kamerê
	SetupCamera(dt);
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
						pc->AddPerk((Perk)fallback_2, -1);
						game_gui->messages->AddGameMsg3(GMS_LEARNED_PERK);
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
				game_level->WarpUnit(pc->unit, fallback_1);
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
			if(ud.spells && !mesh.GetPoint(NAMES::point_cast))
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
Unit* Game::CreateUnit(UnitData& base, int level, Human* human_data, Unit* test_unit, bool create_physics, bool custom)
{
	Unit* u;
	if(test_unit)
		u = test_unit;
	else
	{
		u = new Unit;
		u->Register();
	}

	// unit data
	u->data = &base;
	u->human_data = nullptr;
	u->pos = Vec3(0, 0, 0);
	u->rot = 0.f;
	u->used_item = nullptr;
	u->live_state = Unit::ALIVE;
	for(int i = 0; i < SLOT_MAX; ++i)
		u->slots[i] = nullptr;
	u->action = A_NONE;
	u->weapon_taken = W_NONE;
	u->weapon_hiding = W_NONE;
	u->weapon_state = WS_HIDDEN;
	if(level == -2)
		u->level = base.level.Random();
	else if(level == -3)
		u->level = base.level.Clamp(game_level->location->st);
	else
		u->level = base.level.Clamp(level);
	u->player = nullptr;
	u->ai = nullptr;
	u->speed = u->prev_speed = 0.f;
	u->hurt_timer = 0.f;
	u->talking = false;
	u->usable = nullptr;
	u->frozen = FROZEN::NO;
	u->in_arena = -1;
	u->run_attack = false;
	u->event_handler = nullptr;
	u->to_remove = false;
	u->temporary = false;
	u->quest_id = -1;
	u->bubble = nullptr;
	u->busy = Unit::Busy_No;
	u->interp = nullptr;
	u->dont_attack = false;
	u->assist = false;
	u->attack_team = false;
	u->last_bash = 0.f;
	u->alcohol = 0.f;
	u->moved = false;
	u->running = false;

	u->fake_unit = true; // to prevent sending hp changed message set temporary as fake unit
	if(base.group == G_PLAYER)
	{
		u->stats = new UnitStats;
		u->stats->fixed = false;
		u->stats->subprofile.value = 0;
		u->stats->Set(base.GetStatProfile());
	}
	else
		u->stats = base.GetStats(u->level);
	u->CalculateStats();
	u->hp = u->hpmax = u->CalculateMaxHp();
	u->mp = u->mpmax = u->CalculateMaxMp();
	u->stamina = u->stamina_max = u->CalculateMaxStamina();
	u->stamina_timer = 0;
	u->fake_unit = false;

	// items
	u->weight = 0;
	u->CalculateLoad();
	if(!custom && base.item_script)
	{
		ItemScript* script = base.item_script;
		if(base.stat_profile && !base.stat_profile->subprofiles.empty() && base.stat_profile->subprofiles[u->stats->subprofile.index]->item_script)
			script = base.stat_profile->subprofiles[u->stats->subprofile.index]->item_script;
		script->Parse(*u);
		SortItems(u->items);
		u->RecalculateWeight();
		if(!res_mgr->IsLoadScreen())
		{
			for(auto slot : u->slots)
			{
				if(slot)
					PreloadItem(slot);
			}
			for(auto& slot : u->items)
				PreloadItem(slot.item);
		}
	}
	if(base.trader && !test_unit)
	{
		u->stock = new TraderStock;
		u->stock->date = world->GetWorldtime();
		base.trader->stock->Parse(u->stock->items);
		if(!game_level->entering)
		{
			for(ItemSlot& slot : u->stock->items)
				PreloadItem(slot.item);
		}
	}

	// gold
	float t;
	if(base.level.x == base.level.y)
		t = 1.f;
	else
		t = float(u->level - base.level.x) / (base.level.y - base.level.x);
	u->gold = Int2::Lerp(base.gold, base.gold2, t).Random();

	if(!test_unit)
	{
		// mesh, human details
		if(base.type == UNIT_TYPE::HUMAN)
		{
			if(human_data)
				u->human_data = human_data;
			else
			{
				u->human_data = new Human;
				u->human_data->beard = Rand() % MAX_BEARD - 1;
				u->human_data->hair = Rand() % MAX_HAIR - 1;
				u->human_data->mustache = Rand() % MAX_MUSTACHE - 1;
				u->human_data->height = Random(0.9f, 1.1f);
				if(IsSet(base.flags2, F2_OLD))
					u->human_data->hair_color = Color::Hex(0xDED5D0);
				else if(IsSet(base.flags, F_CRAZY))
					u->human_data->hair_color = Vec4(RandomPart(8), RandomPart(8), RandomPart(8), 1.f);
				else if(IsSet(base.flags, F_GRAY_HAIR))
					u->human_data->hair_color = g_hair_colors[Rand() % 4];
				else if(IsSet(base.flags, F_TOMASHU))
				{
					u->human_data->beard = 4;
					u->human_data->mustache = -1;
					u->human_data->hair = 0;
					u->human_data->hair_color = g_hair_colors[0];
					u->human_data->height = 1.1f;
				}
				else
					u->human_data->hair_color = g_hair_colors[Rand() % n_hair_colors];
#undef HEX
			}
		}

		u->CreateMesh(Unit::CREATE_MESH::NORMAL);

		// hero data
		if(IsSet(base.flags, F_HERO))
		{
			u->hero = new HeroData;
			u->hero->Init(*u);
		}
		else
			u->hero = nullptr;

		// boss music
		if(IsSet(u->data->flags2, F2_BOSS))
			world->AddBossLevel(Int2(game_level->location_index, game_level->dungeon_level));

		// physics
		if(create_physics)
			u->CreatePhysics();
		else
			u->cobj = nullptr;
	}

	if(Net::IsServer() && !game_level->entering)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = u;
	}

	return u;
}

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
		hitbox = unit.mesh_inst->mesh->GetPoint(Format("hitbox%d", unit.attack_id + 1));
		if(!hitbox)
			hitbox = unit.mesh_inst->mesh->FindPoint("hitbox");
	}

	assert(hitbox);

	return CheckForHit(area, unit, hitted, *hitbox, point, hitpoint);
}

// Sprawdza czy jest kolizja hitboxa z jak¹œ postaci¹
// S¹ dwie opcje:
//  - bone to punkt "bron", hitbox to hitbox z bronii
//  - bone jest nullptr, hitbox jest na modelu postaci
// Na drodze testów ustali³em ¿e najlepiej dzia³a gdy stosuje siê sam¹ funkcjê OrientedBoxToOrientedBox
bool Game::CheckForHit(LevelArea& area, Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint)
{
	assert(hitted && hitbox.IsBox());

	// ustaw koœci
	if(unit.human_data)
		unit.mesh_inst->SetupBones(&unit.human_data->mat_scale[0]);
	else
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
				unit.hitted = true;

				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = tSpark;
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
				pe->op_size = POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = POP_LINEAR_SHRINK;
				pe->mode = 0;
				pe->Init();
				area.tmp->pes.push_back(pe);

				sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, HIT_SOUND_DIST);

				if(Net::IsLocal() && unit.IsPlayer())
				{
					if(quest_mgr->quest_tutorial->in_tutorial)
						quest_mgr->quest_tutorial->HandleMeleeAttackCollision();
					unit.player->Train(TrainWhat::AttackNoDamage, 0.f, 1);
				}

				return false;
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
		if(tpe->Update(dt, nullptr, nullptr))
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

	float power;
	if(unit.data->frames->extra)
		power = 1.f;
	else
		power = unit.data->frames->attack_power[unit.attack_id];
	return DoGenericAttack(area, unit, *hitted, hitpoint, unit.CalculateAttack()*unit.attack_power*power, unit.GetDmgType(), false);
}

void Game::GiveDmg(Unit& taker, float dmg, Unit* giver, const Vec3* hitpoint, int dmg_flags)
{
	// blood particles
	if(!IsSet(dmg_flags, DMG_NO_BLOOD))
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = tBlood[taker.data->blood];
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
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 0.9f;
		pe->op_alpha = POP_LINEAR_SHRINK;
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

	if(!IsSet(hitted->data->flags, F_DONT_SUFFER) && hitted->last_bash <= 0.f)
	{
		hitted->last_bash = 1.f + float(hitted->Get(AttributeId::END)) / 50.f;

		hitted->BreakAction();

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->animation_state = 1;

		if(hitted->mesh_inst->mesh->head.n_groups == 2)
		{
			hitted->mesh_inst->frame_end_info2 = false;
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
			hitted->mesh_inst->groups[1].speed = 1.f;
		}
		else
		{
			hitted->mesh_inst->frame_end_info = false;
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
			hitted->mesh_inst->groups[0].speed = 1.f;
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
		it->pos += Vec3(sin(it->rot.y)*it->speed, it->yspeed, cos(it->rot.y)*it->speed) * dt;
		if(it->spell && it->spell->type == Spell::Ball)
			it->yspeed -= 10.f*dt;

		// update particles
		if(it->pe)
			it->pe->pos = it->pos;
		if(it->trail)
		{
			Vec3 pt1 = it->pos;
			pt1.y += 0.05f;
			Vec3 pt2 = it->pos;
			pt2.y -= 0.05f;
			it->trail->Update(0, &pt1, &pt2);

			pt1 = it->pos;
			pt1.x += sin(it->rot.y + PI / 2)*0.05f;
			pt1.z += cos(it->rot.y + PI / 2)*0.05f;
			pt2 = it->pos;
			pt2.x -= sin(it->rot.y + PI / 2)*0.05f;
			pt2.z -= cos(it->rot.y + PI / 2)*0.05f;
			it->trail2->Update(0, &pt1, &pt2);
		}

		// remove bullet on timeout
		if((it->timer -= dt) <= 0.f)
		{
			// timeout, delete bullet
			it->remove = true;
			deletions = true;
			if(it->trail)
			{
				it->trail->destroy = true;
				it->trail2->destroy = true;
			}
			if(it->pe)
				it->pe->destroy = true;
			continue;
		}

		// do contact test
		btCollisionShape* shape;
		if(!it->spell)
			shape = game_level->shape_arrow;
		else
			shape = it->spell->shape;
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
		{
			it->trail->destroy = true;
			it->trail2->destroy = true;
		}
		if(it->pe)
			it->pe->destroy = true;

		if(hitted)
		{
			if(!Net::IsLocal())
				continue;

			if(!it->spell)
			{
				if(it->owner && it->owner->IsFriend(*hitted, true) || it->attack < -50.f)
				{
					// friendly fire
					if(hitted->IsBlocking() && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
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
					sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
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
								hitted->animation_state = 1;

							if(hitted->mesh_inst->mesh->head.n_groups == 2)
							{
								hitted->mesh_inst->frame_end_info2 = false;
								hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
							}
							else
							{
								hitted->mesh_inst->frame_end_info = false;
								hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
								hitted->mesh_inst->groups[0].speed = 1.f;
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
						sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
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
					dmg += it->owner->level * it->spell->dmg_bonus;
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

				GiveDmg(*hitted, dmg, it->owner, &hitpoint, !IsSet(it->spell->flags, Spell::Poison) ? DMG_MAGICAL : 0);

				// apply poison
				if(IsSet(it->spell->flags, Spell::Poison))
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
			if(!it->spell)
			{
				sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, ARROW_HIT_SOUND_DIST);

				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = tSpark;
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
				pe->op_size = POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = POP_LINEAR_SHRINK;
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

Unit* Game::CreateUnitWithAI(LevelArea& area, UnitData& unit, int level, Human* human_data, const Vec3* pos, const float* rot, AIController** ai)
{
	Unit* u = CreateUnit(unit, level, human_data);
	u->area = &area;
	area.units.push_back(u);

	if(pos)
	{
		if(area.area_type == LevelArea::Type::Outside)
		{
			Vec3 pt = *pos;
			game_level->terrain->SetH(pt);
			u->pos = pt;
		}
		else
			u->pos = *pos;
		u->UpdatePhysics();
		u->visual_pos = u->pos;
	}

	if(rot)
		u->rot = *rot;

	AIController* a = new AIController;
	a->Init(u);
	ais.push_back(a);
	if(ai)
		*ai = a;

	return u;
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
	if(game_level->is_open && game_level->location->type == L_ENCOUNTER)
		LeaveLocation();

	net->ClearFastTravel();
	world->ExitToMap();
	SetMusic(MusicType::Travel);

	if(Net::IsServer())
		Net::PushChange(NetChange::EXIT_TO_MAP);

	game_gui->world_map->Show();
	game_gui->level_gui->visible = false;
}

Sound* Game::GetMaterialSound(MATERIAL_TYPE atakuje, MATERIAL_TYPE trafiony)
{
	switch(trafiony)
	{
	case MAT_BODY:
		return sBody[Rand() % 5];
	case MAT_BONE:
		return sBone;
	case MAT_CLOTH:
	case MAT_SKIN:
		return sSkin;
	case MAT_IRON:
		return sMetal;
	case MAT_WOOD:
		return sWood;
	case MAT_ROCK:
		return sRock;
	case MAT_CRYSTAL:
		return sCrystal;
	default:
		assert(0);
		return nullptr;
	}
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
		sound_mgr->PlaySound3d(GetMaterialSound(weapon_mat, hitted_mat), hitpoint, HIT_SOUND_DIST);

		// train blocking
		if(hitted.IsPlayer())
			hitted.player->Train(TrainWhat::BlockAttack, attack / hitted.hpmax, attacker.level);

		// reduce damage
		float block = hitted.CalculateBlock() * hitted.GetBlockMod();
		float stamina = min(attack, block);
		if(IsSet(attacker.data->flags2, F2_IGNORE_BLOCK))
			block *= 2.f / 3;
		if(attacker.attack_power >= 1.9f)
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
					hitted.animation_state = 1;

				if(hitted.mesh_inst->mesh->head.n_groups == 2)
				{
					hitted.mesh_inst->frame_end_info2 = false;
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
				}
				else
				{
					hitted.mesh_inst->frame_end_info = false;
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
					hitted.mesh_inst->groups[0].speed = 1.f;
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
	Spell& spell = *bullet.spell;

	// dŸwiêk
	if(spell.sound_hit)
		sound_mgr->PlaySound3d(spell.sound_hit, pos, spell.sound_hit_dist);

	// cz¹steczki
	if(spell.tex_particle && spell.type == Spell::Ball)
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = spell.tex_particle;
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
		pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
		pe->pos_max = Vec3(spell.size, spell.size, spell.size);
		pe->size = spell.size / 2;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 1.f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 1;
		pe->Init();
		area.tmp->pes.push_back(pe);
	}

	// wybuch
	if(Net::IsLocal() && spell.tex_explode && IsSet(spell.flags, Spell::Explode))
	{
		Explo* explo = new Explo;
		explo->dmg = (float)spell.dmg;
		if(bullet.owner)
			explo->dmg += float((bullet.owner->level + bullet.owner->CalculateMagicPower()) * spell.dmg_bonus);
		explo->size = 0.f;
		explo->sizemax = spell.explode_range;
		explo->pos = pos;
		explo->tex = spell.tex_explode;
		explo->owner = bullet.owner;
		if(hitted)
			explo->hitted.push_back(hitted);
		area.tmp->explos.push_back(explo);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CREATE_EXPLOSION;
			c.spell = &spell;
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
		explo.size += explo.sizemax*dt;
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

				trap.obj2.pos.y = trap.obj.pos.y - 2.f + 2.f*(trap.time / 0.27f);

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
								sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, unit->GetBodyMaterial()), unit->pos + Vec3(0, 1.f, 0), HIT_SOUND_DIST);

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
						b.mesh = aArrow;
						b.pos = Vec3(2.f*trap.tile.x + trap.pos.x - float(int(trap.pos.x / 2) * 2) + Random(-trap.base->rw, trap.base->rw) - 1.2f*DirToPos(trap.dir).x,
							Random(0.5f, 1.5f),
							2.f*trap.tile.y + trap.pos.z - float(int(trap.pos.z / 2) * 2) + Random(-trap.base->h, trap.base->h) - 1.2f*DirToPos(trap.dir).y);
						b.start_pos = b.pos;
						b.rot = Vec3(0, DirToRot(trap.dir), 0);
						b.owner = nullptr;
						b.pe = nullptr;
						b.remove = false;
						b.speed = TRAP_ARROW_SPEED;
						b.spell = nullptr;
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

						TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
						tpe2->fade = 0.3f;
						tpe2->color1 = Vec4(1, 1, 1, 0.5f);
						tpe2->color2 = Vec4(1, 1, 1, 0);
						tpe2->Init(50);
						area.tmp->tpes.push_back(tpe2);
						b.trail2 = tpe2;

						sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, SHOOT_SOUND_DIST);

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
					Spell* fireball = Spell::TryGet("fireball");

					Explo* explo = new Explo;
					explo->pos = trap.pos;
					explo->pos.y += 0.2f;
					explo->size = 0.f;
					explo->sizemax = 2.f;
					explo->dmg = float(trap.base->attack);
					explo->tex = fireball->tex_explode;

					sound_mgr->PlaySound3d(fireball->sound_hit, explo->pos, fireball->sound_hit_dist);

					area.tmp->explos.push_back(explo);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CREATE_EXPLOSION;
						c.spell = fireball;
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

		for(Electro::Line& line : electro.lines)
			line.t += dt;

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

				// deal damage
				if(!owner->IsFriend(*hitted, true))
				{
					if(hitted->IsAI() && owner->IsAlive())
						hitted->ai->HitReaction(electro.start_pos);
					GiveDmg(*hitted, electro.dmg, owner, nullptr, DMG_NO_BLOOD | DMG_MAGICAL);
				}

				// play sound
				if(electro.spell->sound_hit)
					sound_mgr->PlaySound3d(electro.spell->sound_hit, electro.lines.back().pts.back(), electro.spell->sound_hit_dist);

				// add particles
				if(electro.spell->tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = electro.spell->tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 8;
					pe->spawn_max = 12;
					pe->max_particles = 12;
					pe->pos = electro.lines.back().pts.back();
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-electro.spell->size, -electro.spell->size, -electro.spell->size);
					pe->pos_max = Vec3(electro.spell->size, electro.spell->size, electro.spell->size);
					pe->size = electro.spell->size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					area.tmp->pes.push_back(pe);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::ELECTRO_HIT;
					c.pos = electro.lines.back().pts.back();
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
							if(game_level->RayTest(electro.lines.back().pts.back(), it2->first->GetCenter(), hitted, hitpoint, new_hitted))
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
							Vec3 from = electro.lines.back().pts.back();
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
				electro.hitsome = false;

				if(electro.spell->sound_hit)
					sound_mgr->PlaySound3d(electro.spell->sound_hit, electro.lines.back().pts.back(), electro.spell->sound_hit_dist);

				// cz¹steczki
				if(electro.spell->tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = electro.spell->tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 8;
					pe->spawn_max = 12;
					pe->max_particles = 12;
					pe->pos = electro.lines.back().pts.back();
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-electro.spell->size, -electro.spell->size, -electro.spell->size);
					pe->pos_max = Vec3(electro.spell->size, electro.spell->size, electro.spell->size);
					pe->size = electro.spell->size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					area.tmp->pes.push_back(pe);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::ELECTRO_HIT;
					c.pos = electro.lines.back().pts.back();
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

		Vec3 center = drain.to->GetCenter();
		for(Particle& p : drain.pe->particles)
			p.pos = Vec3::Lerp(p.pos, center, drain.t / 1.5f);

		return false;
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
	game_gui->world_map->dialog_enc = nullptr;
	game_gui->level_gui->visible = false;
	game_gui->inventory->lock = nullptr;
	game_gui->world_map->picked_location = -1;
	post_effects.clear();
	grayout = 0.f;
	game_level->camera.Reset();
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
		game_level->cl_fog = true;
		game_level->cl_lighting = true;
		draw_particle_sphere = false;
		draw_unit_radius = false;
		draw_hitbox = false;
		noai = false;
		draw_phy = false;
		draw_col = false;
		game_level->camera.real_rot = Vec2(0, 4.2875104f);
		game_level->camera.dist = 3.5f;
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
		drunk_anim = 0.f;
		game_level->light_angle = Random(PI * 2);
		game_level->camera.Reset();
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
	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP) && !game_level->is_open && (Net::IsClient() || net->was_client))
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

Sound* Game::GetItemSound(const Item* item)
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return sItem[6];
	case IT_BOW:
		return sItem[4];
	case IT_SHIELD:
		return sItem[5];
	case IT_ARMOR:
		if(item->ToArmor().armor_type != AT_LIGHT)
			return sItem[2];
		else
			return sItem[1];
	case IT_AMULET:
		return sItem[8];
	case IT_RING:
		return sItem[9];
	case IT_CONSUMABLE:
		if(Any(item->ToConsumable().cons_type, ConsumableType::Food, ConsumableType::Herb))
			return sItem[7];
		else
			return sItem[0];
	case IT_OTHER:
		if(IsSet(item->flags, ITEM_CRYSTAL_SOUND))
			return sItem[3];
		else
			return sItem[7];
	case IT_GOLD:
		return sCoins;
	default:
		return sItem[7];
	}
}

void ApplyTexturePackToSubmesh(Mesh::Submesh& sub, TexturePack& tp)
{
	sub.tex = tp.diffuse;
	sub.tex_normal = tp.normal;
	sub.tex_specular = tp.specular;
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

void Game::ApplyLocationTexturePack(TexturePack& floor, TexturePack& wall, TexturePack& ceil, LocationTexturePack& tex)
{
	ApplyLocationTexturePack(floor, tex.floor, tFloorBase);
	ApplyLocationTexturePack(wall, tex.wall, tWallBase);
	ApplyLocationTexturePack(ceil, tex.ceil, tCeilBase);
}

void Game::ApplyLocationTexturePack(TexturePack& pack, LocationTexturePack::Entry& e, TexturePack& pack_def)
{
	if(e.tex)
	{
		pack.diffuse = e.tex;
		pack.normal = e.tex_normal;
		pack.specular = e.tex_specular;
	}
	else
		pack = pack_def;

	res_mgr->Load(pack.diffuse);
	if(pack.normal)
		res_mgr->Load(pack.normal);
	if(pack.specular)
		res_mgr->Load(pack.specular);
}

void Game::SetDungeonParamsAndTextures(BaseLocation& base)
{
	// ustawienia t³a
	game_level->camera.draw_range = base.draw_range;
	game_level->fog_params = Vec4(base.fog_range.x, base.fog_range.y, base.fog_range.y - base.fog_range.x, 0);
	game_level->fog_color = Vec4(base.fog_color, 1);
	game_level->ambient_color = Vec4(base.ambient_color, 1);
	clear_color_next = Color(int(game_level->fog_color.x * 255), int(game_level->fog_color.y * 255), int(game_level->fog_color.z * 255));

	// tekstury podziemi
	ApplyLocationTexturePack(tFloor[0], tWall[0], tCeil[0], base.tex);

	// druga tekstura
	if(base.tex2 != -1)
	{
		BaseLocation& base2 = g_base_locations[base.tex2];
		ApplyLocationTexturePack(tFloor[1], tWall[1], tCeil[1], base2.tex);
	}
	else
	{
		tFloor[1] = tFloor[0];
		tCeil[1] = tCeil[0];
		tWall[1] = tWall[0];
	}

	// ustawienia uv podziemi
	bool new_tex_wrap = !IsSet(base.options, BLO_NO_TEX_WRAP);
	if(new_tex_wrap != dungeon_tex_wrap)
	{
		dungeon_tex_wrap = new_tex_wrap;
		ChangeDungeonTexWrap();
	}
}

void Game::SetDungeonParamsToMeshes()
{
	// tekstury schodów / pu³apek
	ApplyTexturePackToSubmesh(aStairsDown->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aStairsDown->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aStairsDown2->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aStairsDown2->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aStairsUp->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aStairsUp->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aDoorWall->subs[0], tWall[0]);
	ApplyDungeonLightToMesh(*aStairsDown);
	ApplyDungeonLightToMesh(*aStairsDown2);
	ApplyDungeonLightToMesh(*aStairsUp);
	ApplyDungeonLightToMesh(*aDoorWall);
	ApplyDungeonLightToMesh(*aDoorWall2);

	// apply texture/lighting to trap to make it same texture as dungeon
	if(BaseTrap::traps[TRAP_ARROW].mesh->state == ResourceState::Loaded)
	{
		ApplyTexturePackToSubmesh(BaseTrap::traps[TRAP_ARROW].mesh->subs[0], tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_ARROW].mesh);
	}
	if(BaseTrap::traps[TRAP_POISON].mesh->state == ResourceState::Loaded)
	{
		ApplyTexturePackToSubmesh(BaseTrap::traps[TRAP_POISON].mesh->subs[0], tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_POISON].mesh);
	}

	// druga tekstura
	ApplyTexturePackToSubmesh(aDoorWall2->subs[0], tWall[1]);
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
	}

	ais.clear();
	game_level->RemoveColliders();
	StopAllSounds();

	ClearQuadtree();

	game_gui->CloseAllPanels();

	game_level->camera.Reset();
	pc->data.rot_buf = 0.f;
	dialog_context.dialog_mode = false;
	game_gui->inventory->mode = I_NONE;
	pc->data.before_player = BP_NONE;
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
					if(!unit.IsAlive())
					{
						unit.hp = 1.f;
						unit.live_state = Unit::ALIVE;
					}
					if(unit.GetOrder() != ORDER_FOLLOW)
						unit.OrderFollow(team->GetLeader());
					unit.mesh_inst->need_update = true;
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
				door.state = Door::Open;
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
			if(door.state == Door::Opening)
			{
				if(door.mesh_inst->frame_end_info || door.mesh_inst->GetProgress() >= 0.25f)
				{
					door.state = Door::Opening2;
					btVector3& pos = door.phy->getWorldTransform().getOrigin();
					pos.setY(pos.y() - 100.f);
				}
			}
			if(door.mesh_inst->frame_end_info)
				door.state = Door::Open;
		}
		else if(door.state == Door::Closing || door.state == Door::Closing2)
		{
			if(door.state == Door::Closing)
			{
				if(door.mesh_inst->frame_end_info || door.mesh_inst->GetProgress() <= 0.25f)
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
			if(door.mesh_inst->frame_end_info)
			{
				if(door.state == Door::Closing2)
					door.state = Door::Closed;
				else
				{
					// nie mo¿na zamknaæ drzwi bo coœ blokuje
					door.state = Door::Opening2;
					door.mesh_inst->Play(&door.mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_NO_BLEND | PLAY_STOP_AT_END, 0);
					door.mesh_inst->frame_end_info = false;
					// mo¿na by daæ lepszy punkt dŸwiêku
					sound_mgr->PlaySound3d(sDoorBudge, door.pos, Door::BLOCKED_SOUND_DIST);
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
	sound_mgr->PlaySound3d(GetMaterialSound(mat2, mat), hitpoint, range);
	if(mat != MAT_BODY && dmg)
		sound_mgr->PlaySound3d(GetMaterialSound(mat2, MAT_BODY), hitpoint, range);

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
		res_mgr->StartLoadScreen(txLoadingResources);

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
					items_load.insert(ground_item->item);
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
			if(!sound_mgr->IsMusicDisabled())
				LoadMusic(game_level->GetLocationMusic(), false);

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
		PreloadItem(item);
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
		items_load.insert(slot.item);
}

void Game::PreloadItem(const Item* p_item)
{
	Item& item = *(Item*)p_item;
	if(item.state == ResourceState::Loaded)
		return;

	if(res_mgr->IsLoadScreen())
	{
		if(item.state != ResourceState::Loading)
		{
			if(item.type == IT_ARMOR)
			{
				Armor& armor = item.ToArmor();
				if(!armor.tex_override.empty())
				{
					for(TexOverride& tex_o : armor.tex_override)
					{
						if(tex_o.diffuse)
							res_mgr->Load(tex_o.diffuse);
					}
				}
			}
			else if(item.type == IT_BOOK)
			{
				Book& book = item.ToBook();
				res_mgr->Load(book.scheme->tex);
			}

			if(item.mesh)
				res_mgr->Load(item.mesh);
			else if(item.tex)
				res_mgr->Load(item.tex);
			res_mgr->AddTask(&item, TaskCallback(game_res, &GameResources::GenerateItemIconTask));

			item.state = ResourceState::Loading;
		}
	}
	else
	{
		// instant loading
		if(item.type == IT_ARMOR)
		{
			Armor& armor = item.ToArmor();
			if(!armor.tex_override.empty())
			{
				for(TexOverride& tex_o : armor.tex_override)
				{
					if(tex_o.diffuse)
						res_mgr->Load(tex_o.diffuse);
				}
			}
		}
		else if(item.type == IT_BOOK)
		{
			Book& book = item.ToBook();
			res_mgr->Load(book.scheme->tex);
		}

		if(item.mesh)
			res_mgr->Load(item.mesh);
		else if(item.tex)
			res_mgr->Load(item.tex);
		game_res->GenerateItemIcon(item);

		item.state = ResourceState::Loaded;
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
						attacked.ai->escape_room = nullptr;
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
	if(dialog_context.talker == talker)
		return &dialog_context;
	if(Net::IsOnline())
	{
		for(PlayerInfo& info : net->players)
		{
			if(info.pc->dialog_ctx->talker == talker)
				return info.pc->dialog_ctx;
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
			event.location = game_level->location;
			e.quest->FireEvent(event);
		}
	}
}
