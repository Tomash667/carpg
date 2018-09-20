#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "GameStats.h"
#include "Terrain.h"
#include "ParticleSystem.h"
#include "Language.h"
#include "Version.h"
#include "Quest_Mages.h"
#include "Content.h"
#include "OutsideLocation.h"
#include "InsideLocation.h"
#include "GameGui.h"
#include "Console.h"
#include "MpBox.h"
#include "GameMenu.h"
#include "ServerPanel.h"
#include "WorldMapGui.h"
#include "MainMenu.h"
#include "Controls.h"
#include "AIController.h"
#include "Spell.h"
#include "Team.h"
#include "RoomType.h"
#include "StartupOptions.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "Inventory.h"
#include "Profiler.h"
#include "World.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Crazies.h"
#include "Quest_Orcs.h"
#include "Quest_Tournament.h"
#include "LocationGeneratorFactory.h"
#include "SuperShader.h"
#include "Arena.h"
#include "DirectX.h"
#include "ResourceManager.h"
#include "BitStreamFunc.h"
#include "City.h"
#include "MultiInsideLocation.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "Portal.h"
#include "GlobalGui.h"
#include "DebugDrawer.h"

// limit fps
#define LIMIT_DT 0.3f

// symulacja lagów
#define TESTUJ_LAG
#ifndef _DEBUG
#undef TESTUJ_LAG
#endif
#ifdef TESTUJ_LAG
#define MY_PRIORITY HIGH_PRIORITY
#else
#define MY_PRIORITY IMMEDIATE_PRIORITY
#endif

const float bazowa_wysokosc = 1.74f;
Game* Game::game;
cstring Game::txGoldPlus, Game::txQuestCompletedGold;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;

//=================================================================================================
Game::Game() : have_console(false), vbParticle(nullptr), quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0), cl_fog(true),
cl_lighting(true), draw_particle_sphere(false), draw_unit_radius(false), draw_hitbox(false), noai(false), testing(false), game_speed(1.f), devmode(false),
draw_phy(false), draw_col(false), force_seed(0), next_seed(0), force_seed_all(false), alpha_test_state(-1), debug_info(false), dont_wander(false),
check_updates(true), skip_tutorial(false), portal_anim(0), debug_info2(false), music_type(MusicType::None),
koniec_gry(false), prepared_stream(64 * 1024),
paused(false), pick_autojoin(false), draw_flags(0xFFFFFFFF), tMiniSave(nullptr), prev_game_state(GS_LOAD), tSave(nullptr), sItemRegion(nullptr),
sItemRegionRot(nullptr), sChar(nullptr), sSave(nullptr), in_tutorial(false), mp_load(false), was_client(false), sCustom(nullptr),
cl_postfx(true), mp_timeout(10.f), cl_normalmap(true), cl_specularmap(true), dungeon_tex_wrap(true), profiler_mode(0),
grass_range(40.f), vbInstancing(nullptr), vb_instancing_max(0), screenshot_format(ImageFormat::JPG), quickstart_class(Class::RANDOM),
autopick_class(Class::INVALID), game_state(GS_LOAD), default_devmode(false), default_player_devmode(false), finished_tutorial(false),
quickstart_slot(MAX_SAVE_SLOTS), autoready(false), super_shader(new SuperShader)
{
#ifdef _DEBUG
	default_devmode = true;
	default_player_devmode = true;
#endif
	devmode = default_devmode;

	obj_alpha.id = "tmp_alpha";
	obj_alpha.alpha = 1;

	game = this;
	Quest::game = this;

	dialog_context.is_local = true;

	ClearPointers();

	uv_mod = Terrain::DEFAULT_UV_MOD;
	cam.draw_range = 80.f;

	SetupConfigVars();
}

//=================================================================================================
Game::~Game()
{
	delete super_shader;
}

//=================================================================================================
// Rysowanie gry
//=================================================================================================
void Game::OnDraw()
{
	OnDraw(true);
}

//=================================================================================================
void Game::OnDraw(bool normal)
{
	if(normal)
	{
		if(profiler_mode == 2)
			Profiler::g_profiler.Start();
		else if(profiler_mode == 0)
			Profiler::g_profiler.Clear();
	}

	if(post_effects.empty() || !ePostFx)
	{
		if(sCustom)
			V(device->SetRenderTarget(0, sCustom));

		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));
		V(device->BeginScene());

		if(game_state == GS_LEVEL)
		{
			// draw level
			Draw();

			// draw glow
			if(pc_data.before_player != BP_NONE && !draw_batch.glow_nodes.empty())
			{
				V(device->EndScene());
				DrawGlowingNodes(false);
				V(device->BeginScene());
			}

			// debug draw
			GetDebugDrawer()->Draw();
		}

		// draw gui
		GUI.mViewProj = cam.matViewProj;
		GUI.Draw(IS_SET(draw_flags, DF_GUI), IS_SET(draw_flags, DF_MENU));

		V(device->EndScene());
	}
	else
	{
		// renderuj scenê do tekstury
		SURFACE sPost;
		if(!IsMultisamplingEnabled())
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
			if(pc_data.before_player != BP_NONE && !draw_batch.glow_nodes.empty())
				DrawGlowingNodes(true);
		}

		PROFILER_BLOCK("PostEffects");

		TEX t;
		if(!IsMultisamplingEnabled())
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
		V(device->SetVertexDeclaration(vertex_decl[VDI_TEX]));
		V(device->SetStreamSource(0, vbFullscreen, 0, sizeof(VTex)));
		SetAlphaTest(false);
		SetAlphaBlend(false);
		SetNoCulling(false);
		SetNoZWrite(true);

		UINT passes;
		int index_surf = 1;
		for(vector<PostEffect>::iterator it = post_effects.begin(), end = post_effects.end(); it != end; ++it)
		{
			SURFACE surf;
			if(it + 1 == end)
			{
				// ostatni pass
				if(sCustom)
					surf = sCustom;
				else
					V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf));
			}
			else
			{
				// jest nastêpny
				if(!IsMultisamplingEnabled())
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
			{
				GUI.mViewProj = cam.matViewProj;
				GUI.Draw(IS_SET(draw_flags, DF_GUI), IS_SET(draw_flags, DF_MENU));
			}

			V(device->EndScene());

			if(it + 1 == end)
			{
				if(!sCustom)
					surf->Release();
			}
			else if(!IsMultisamplingEnabled())
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

	Profiler::g_profiler.End();
}

//=================================================================================================
void HumanPredraw(void* ptr, Matrix* mat, int n)
{
	if(n != 0)
		return;

	Unit* u = (Unit*)ptr;

	if(u->data->type == UNIT_TYPE::HUMAN)
	{
		int bone = u->mesh_inst->mesh->GetBone("usta")->id;
		static Matrix mat2;
		float val = u->talking ? sin(u->talk_timer * 6) : 0.f;
		mat[bone] = Matrix::RotationX(val / 5) *mat[bone];
	}
}

//=================================================================================================
// Aktualizacja gry, g³ównie multiplayer
//=================================================================================================
void Game::OnTick(float dt)
{
	// sprawdzanie pamiêci
	assert(_CrtCheckMemory());

	// limit czasu ramki
	if(dt > LIMIT_DT)
		dt = LIMIT_DT;

	if(profiler_mode == 1)
		Profiler::g_profiler.Start();
	else if(profiler_mode == 0)
		Profiler::g_profiler.Clear();

	UpdateMusic();

	if(Net::IsSingleplayer() || !paused)
	{
		// aktualizacja czasu spêdzonego w grze
		if(game_state != GS_MAIN_MENU && game_state != GS_LOAD)
			GameStats::Get().Update(dt);
	}

	GKey.allow_input = GameKeys::ALLOW_INPUT;

	// utracono urz¹dzenie directx lub okno nie aktywne
	if(IsLostDevice() || !IsActive() || !IsCursorLocked())
	{
		Key.SetFocus(false);
		if(Net::IsSingleplayer() && !inactive_update)
			return;
	}
	else
		Key.SetFocus(true);

	if(devmode)
	{
		if(Key.PressedRelease(VK_F3))
			debug_info = !debug_info;
	}
	if(Key.PressedRelease(VK_F2))
		debug_info2 = !debug_info2;

	// szybkie wyjœcie z gry (alt+f4)
	if(Key.Focus() && Key.Down(VK_MENU) && Key.Down(VK_F4) && !GUI.HaveTopDialog("dialog_alt_f4"))
		ShowQuitDialog();

	if(koniec_gry)
	{
		death_fade += dt;
		if(death_fade >= 1.f && GKey.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE))
		{
			ExitToMenu();
			koniec_gry = false;
		}
		GKey.allow_input = GameKeys::ALLOW_NONE;
	}

	// globalna obs³uga klawiszy
	if(GKey.allow_input == GameKeys::ALLOW_INPUT)
	{
		// konsola
		if(!GUI.HaveTopDialog("dialog_alt_f4") && !GUI.HaveDialog("console") && GKey.KeyDownUpAllowed(GK_CONSOLE))
			GUI.ShowDialog(gui->console);

		// uwolnienie myszki
		if(!IsFullscreen() && IsActive() && IsCursorLocked() && Key.Shortcut(KEY_CONTROL, 'U'))
			UnlockCursor();

		// zmiana trybu okna
		if(Key.Shortcut(KEY_ALT, VK_RETURN))
			ChangeMode(!IsFullscreen());

		// screenshot
		if(Key.PressedRelease(VK_SNAPSHOT))
			TakeScreenshot(Key.Down(VK_SHIFT));

		// zatrzymywanie/wznawianie gry
		if(GKey.KeyPressedReleaseAllowed(GK_PAUSE) && !Net::IsClient())
			PauseGame();
	}

	// obs³uga paneli
	if(GUI.HaveDialog() || (gui->mp_box->visible && gui->mp_box->itb.focus))
		GKey.allow_input = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode)
	{
		OpenPanel open = gui->game_gui->GetOpenPanel(),
			to_open = OpenPanel::None;

		if (GKey.PressedRelease(GK_STATS))
			to_open = OpenPanel::Stats;
		else if (GKey.PressedRelease(GK_INVENTORY))
			to_open = OpenPanel::InventoryPanel;
		else if (GKey.PressedRelease(GK_TEAM_PANEL))
			to_open = OpenPanel::Team;
		else if (GKey.PressedRelease(GK_JOURNAL))
			to_open = OpenPanel::Journal;
		else if (GKey.PressedRelease(GK_MINIMAP))
			to_open = OpenPanel::Minimap;
		else if (GKey.PressedRelease(GK_ACTION_PANEL))
			to_open = OpenPanel::Action;
		else if(open == OpenPanel::Trade && Key.PressedRelease(VK_ESCAPE))
			to_open = OpenPanel::Trade; // ShowPanel hide when already opened

		if(to_open != OpenPanel::None)
			gui->game_gui->ShowPanel(to_open, open);

		switch(open)
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(gui->game_gui->use_cursor)
				GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::InventoryPanel:
		case OpenPanel::Team:
		case OpenPanel::Trade:
		case OpenPanel::Action:
		case OpenPanel::Journal:
			GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		}
	}

	// quicksave, quickload
	bool console_open = GUI.HaveTopDialog("console");
	bool special_key_allowed = (GKey.allow_input == GameKeys::ALLOW_KEYBOARD || GKey.allow_input == GameKeys::ALLOW_INPUT || (!GUI.HaveDialog() || console_open));
	if(GKey.KeyPressedReleaseSpecial(GK_QUICKSAVE, special_key_allowed))
		Quicksave(console_open);
	if(GKey.KeyPressedReleaseSpecial(GK_QUICKLOAD, special_key_allowed))
		Quickload(console_open);

	// mp box
	if((game_state == GS_LEVEL || game_state == GS_WORLDMAP) && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
		gui->mp_box->visible = !gui->mp_box->visible;

	// update gui
	gui->Update(dt);
	if(game_state == GS_EXIT_TO_MENU)
	{
		ExitToMenu();
		return;
	}
	else if(game_state == GS_QUIT)
	{
		ClearGame();
		EngineShutdown();
		return;
	}

	// handle blocking input by gui
	if(GUI.HaveDialog() || (gui->mp_box->visible && gui->mp_box->itb.focus))
		GKey.allow_input = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode)
	{
		switch(gui->game_gui->GetOpenPanel())
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(gui->game_gui->use_cursor)
				GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::InventoryPanel:
		case OpenPanel::Team:
		case OpenPanel::Trade:
		case OpenPanel::Action:
		case OpenPanel::Journal:
			GKey.allow_input = GameKeys::ALLOW_KEYBOARD;
			break;
		}
	}
	else
		GKey.allow_input = GameKeys::ALLOW_INPUT;

	// otwórz menu
	if(GKey.AllowKeyboard() && CanShowMenu() && Key.PressedRelease(VK_ESCAPE))
		ShowMenu();

	if(gui->game_menu->visible)
		gui->game_menu->Set(CanSaveGame(), CanLoadGame(), hardcore_mode);

	arena->UpdatePvpRequest(dt);

	// aktualizacja gry
	if(!koniec_gry)
	{
		if(game_state == GS_LEVEL)
		{
			if(paused)
			{
				UpdateFallback(dt);
				if (Net::IsLocal())
				{
					if (Net::IsOnline())
						UpdateWarpData(dt);
					L.ProcessUnitWarps();
				}
				SetupCamera(dt);
				if(Net::IsOnline())
					UpdateGameNet(dt);
			}
			else if(GUI.HavePauseDialog())
			{
				if (Net::IsOnline())
					UpdateGame(dt);
				else
				{
					UpdateFallback(dt);
					if (Net::IsLocal())
					{
						if (Net::IsOnline())
							UpdateWarpData(dt);
						L.ProcessUnitWarps();
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
	else if(Net::IsOnline())
		UpdateGameNet(dt);

	// aktywacja mp_box
	if(GKey.AllowKeyboard() && game_state == GS_LEVEL && gui->mp_box->visible && !gui->mp_box->itb.focus && Key.PressedRelease(VK_RETURN))
	{
		gui->mp_box->itb.focus = true;
		gui->mp_box->Event(GuiEvent_GainFocus);
		gui->mp_box->itb.Event(GuiEvent_GainFocus);
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
	s += " -  DEBUG";
#endif

	if(change_title_a && ((game_state != GS_MAIN_MENU && game_state != GS_LOAD) || (gui->server && gui->server->visible)))
	{
		if(Net::IsOnline())
		{
			if(Net::IsServer())
			{
				if(none)
					s += " - SERVER";
				else
					s += ", SERVER";
			}
			else
			{
				if(none)
					s += " - CLIENT";
				else
					s += ", CLIENT";
			}
		}
		else
		{
			if(none)
				s += " - SINGLE";
			else
				s += ", SINGLE";
		}
	}

	s += Format(" [%d]", GetCurrentProcessId());
}

//=================================================================================================
void Game::ChangeTitle()
{
	LocalString s;
	GetTitle(s);
	SetConsoleTitle(s->c_str());
	SetTitle(s->c_str());
}

//=================================================================================================
bool Game::Start0(StartupOptions& options)
{
	LocalString s;
	GetTitle(s);
	options.title = s.c_str();
	return Start(options);
}

//=================================================================================================
void Game::OnReload()
{
	GUI.OnReload();

	if(eMesh)
		V(eMesh->OnResetDevice());
	if(eParticle)
		V(eParticle->OnResetDevice());
	if(eTerrain)
		V(eTerrain->OnResetDevice());
	if(eSkybox)
		V(eSkybox->OnResetDevice());
	if(eArea)
		V(eArea->OnResetDevice());
	if(eGui)
		V(eGui->OnResetDevice());
	if(ePostFx)
		V(ePostFx->OnResetDevice());
	if(eGlow)
		V(eGlow->OnResetDevice());
	if(eGrass)
		V(eGrass->OnResetDevice());

	super_shader->OnReload();

	CreateTextures();
	BuildDungeon();
	// rebuild minimap texture
	if(game_state == GS_LEVEL)
		loc_gen_factory->Get(L.location)->CreateMinimap();
	gui->inventory->OnReload();
}

//=================================================================================================
void Game::OnReset()
{
	GUI.OnReset();
	gui->inventory->OnReset();

	if(eMesh)
		V(eMesh->OnLostDevice());
	if(eParticle)
		V(eParticle->OnLostDevice());
	if(eTerrain)
		V(eTerrain->OnLostDevice());
	if(eSkybox)
		V(eSkybox->OnLostDevice());
	if(eArea)
		V(eArea->OnLostDevice());
	if(eGui)
		V(eGui->OnLostDevice());
	if(ePostFx)
		V(ePostFx->OnLostDevice());
	if(eGlow)
		V(eGlow->OnLostDevice());
	if(eGrass)
		V(eGrass->OnLostDevice());

	super_shader->OnReset();

	SafeRelease(tItemRegion);
	SafeRelease(tItemRegionRot);
	SafeRelease(tMinimap);
	SafeRelease(tChar);
	SafeRelease(tSave);
	SafeRelease(sItemRegion);
	SafeRelease(sItemRegionRot);
	SafeRelease(sChar);
	SafeRelease(sSave);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}
	SafeRelease(vbFullscreen);
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
	SafeRelease(vbInstancing);
	vb_instancing_max = 0;
}

//=================================================================================================
void Game::OnChar(char c)
{
	if((c != VK_BACK && c != VK_RETURN && byte(c) < 0x20) || c == '`')
		return;

	GUI.OnChar(c);
}

//=================================================================================================
void Game::TakeScreenshot(bool no_gui)
{
	if(no_gui)
	{
		int old_flags = draw_flags;
		draw_flags = (0xFFFF & ~DF_GUI);
		Render(true);
		draw_flags = old_flags;
	}
	else
		Render(true);

	SURFACE back_buffer;
	HRESULT hr = device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	if(FAILED(hr))
	{
		cstring msg = Format("Failed to get front buffer data to save screenshot (%d)!", hr);
		AddConsoleMsg(msg);
		Error(msg);
	}
	else
	{
		CreateDirectory("screenshots", nullptr);

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
		case D3DXIFF_BMP:
			ext = "bmp";
			format = D3DXIFF_BMP;
			break;
		default:
		case D3DXIFF_JPG:
			ext = "jpg";
			format = D3DXIFF_JPG;
			break;
		case D3DXIFF_TGA:
			ext = "tga";
			format = D3DXIFF_TGA;
			break;
		case D3DXIFF_PNG:
			ext = "png";
			format = D3DXIFF_PNG;
			break;
		}

		cstring path = Format("screenshots\\%04d%02d%02d_%02d%02d%02d_%02d.%s", lt.tm_year + 1900, lt.tm_mon + 1,
			lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, screenshot_count, ext);

		D3DXSaveSurfaceToFileA(path, format, back_buffer, nullptr, nullptr);

		cstring msg = Format("Screenshot saved to '%s'.", path);
		AddConsoleMsg(msg);
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

	auto& res_mgr = ResourceManager::Get();
	if(res_mgr.IsLoadScreen())
		res_mgr.CancelLoadScreen(true);

	game_state = GS_MAIN_MENU;
	paused = false;
	mp_load = false;
	was_client = false;

	SetMusic(MusicType::Title);
	koniec_gry = false;

	CloseAllPanels();
	GUI.CloseDialogs();
	gui->game_menu->visible = false;
	gui->game_gui->visible = false;
	gui->world_map->visible = false;
	gui->main_menu->visible = true;
	units_mesh_load.clear();

	if(change_title_a)
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
	eTerrain = nullptr;
	eSkybox = nullptr;
	eArea = nullptr;
	eGui = nullptr;
	ePostFx = nullptr;
	eGlow = nullptr;
	eGrass = nullptr;

	// bufory wierzcho³ków i indeksy
	vbParticle = nullptr;
	vbDungeon = nullptr;
	ibDungeon = nullptr;
	vbFullscreen = nullptr;

	// tekstury render target, powierzchnie
	tItemRegion = nullptr;
	tItemRegionRot = nullptr;
	tMinimap = nullptr;
	tChar = nullptr;
	sItemRegion = nullptr;
	sItemRegionRot = nullptr;
	sChar = nullptr;
	sSave = nullptr;
	for(int i = 0; i < 3; ++i)
	{
		sPostEffect[i] = nullptr;
		tPostEffect[i] = nullptr;
	}

	// vertex data
	vdSchodyGora = nullptr;
	vdSchodyDol = nullptr;
	vdNaDrzwi = nullptr;

	// teren
	terrain = nullptr;
	terrain_shape = nullptr;

	// fizyka
	shape_wall = nullptr;
	shape_low_ceiling = nullptr;
	shape_ceiling = nullptr;
	shape_floor = nullptr;
	shape_door = nullptr;
	shape_block = nullptr;
	shape_schody_c[0] = nullptr;
	shape_schody_c[1] = nullptr;
	shape_schody = nullptr;
	shape_summon = nullptr;
	shape_barrier = nullptr;
	shape_arrow = nullptr;
	dungeon_shape = nullptr;
	dungeon_shape_data = nullptr;

	// vertex declarations
	for(int i = 0; i < VDI_MAX; ++i)
		vertex_decl[i] = nullptr;
}

//=================================================================================================
void Game::OnCleanup()
{
	if(game_state != GS_QUIT)
		ClearGame();

	for(std::pair<GameComponent*, bool>& component : components)
	{
		component.first->Cleanup();
		if(component.second)
			delete component.first;
	}

	GUI.OnClean();
	CleanScene();
	DeleteElements(bow_instances);
	ClearQuadtree();
	CleanupDialogs();
	Language::Cleanup();

	// shadery
	ReleaseShaders();

	// bufory wierzcho³ków i indeksy
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
	SafeRelease(vbFullscreen);
	SafeRelease(vbInstancing);

	// tekstury render target, powierzchnie
	SafeRelease(tItemRegion);
	SafeRelease(sItemRegionRot);
	SafeRelease(tMinimap);
	SafeRelease(tChar);
	SafeRelease(tSave);
	SafeRelease(sItemRegion);
	SafeRelease(sItemRegionRot);
	SafeRelease(sChar);
	SafeRelease(sSave);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}

	// item textures
	for(auto& it : item_texture_map)
		SafeRelease(it.second);

	content::CleanupContent();

	// teren
	delete terrain;
	delete terrain_shape;

	// fizyka
	delete shape_wall;
	delete shape_low_ceiling;
	delete shape_ceiling;
	delete shape_floor;
	delete shape_door;
	delete shape_block;
	delete shape_schody_c[0];
	delete shape_schody_c[1];
	delete shape_summon;
	delete shape_barrier;
	delete shape_schody;
	delete shape_arrow;
	delete dungeon_shape;
	delete dungeon_shape_data;

	draw_batch.Clear();
	DeleteElements(old_players);
	SM.Cleanup();
	N.Cleanup();
}

//=================================================================================================
void Game::CreateTextures()
{
	auto& wnd_size = GetWindowSize();

	V(device->CreateTexture(64, 64, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tItemRegion, nullptr));
	V(device->CreateTexture(128, 128, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tItemRegionRot, nullptr));
	V(device->CreateTexture(128, 128, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tMinimap, nullptr));
	V(device->CreateTexture(128, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tChar, nullptr));
	V(device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tSave, nullptr));

	int ms, msq;
	GetMultisampling(ms, msq);
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)ms;
	if(ms != D3DMULTISAMPLE_NONE)
	{
		V(device->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, type, msq, FALSE, &sItemRegion, nullptr));
		V(device->CreateRenderTarget(128, 128, D3DFMT_A8R8G8B8, type, msq, FALSE, &sItemRegionRot, nullptr));
		V(device->CreateRenderTarget(128, 256, D3DFMT_A8R8G8B8, type, msq, FALSE, &sChar, nullptr));
		V(device->CreateRenderTarget(256, 256, D3DFMT_X8R8G8B8, type, msq, FALSE, &sSave, nullptr));
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
void Game::InitGameKeys()
{
	GKey[GK_MOVE_FORWARD].id = "keyMoveForward";
	GKey[GK_MOVE_BACK].id = "keyMoveBack";
	GKey[GK_MOVE_LEFT].id = "keyMoveLeft";
	GKey[GK_MOVE_RIGHT].id = "keyMoveRight";
	GKey[GK_WALK].id = "keyWalk";
	GKey[GK_ROTATE_LEFT].id = "keyRotateLeft";
	GKey[GK_ROTATE_RIGHT].id = "keyRotateRight";
	GKey[GK_TAKE_WEAPON].id = "keyTakeWeapon";
	GKey[GK_ATTACK_USE].id = "keyAttackUse";
	GKey[GK_USE].id = "keyUse";
	GKey[GK_BLOCK].id = "keyBlock";
	GKey[GK_STATS].id = "keyStats";
	GKey[GK_INVENTORY].id = "keyInventory";
	GKey[GK_TEAM_PANEL].id = "keyTeam";
	GKey[GK_ACTION_PANEL].id = "keyActions";
	GKey[GK_JOURNAL].id = "keyGameJournal";
	GKey[GK_MINIMAP].id = "keyMinimap";
	GKey[GK_QUICKSAVE].id = "keyQuicksave";
	GKey[GK_QUICKLOAD].id = "keyQuickload";
	GKey[GK_POTION].id = "keyPotion";
	GKey[GK_MELEE_WEAPON].id = "keyMeleeWeapon";
	GKey[GK_RANGED_WEAPON].id = "keyRangedWeapon";
	GKey[GK_ACTION].id = "keyAction";
	GKey[GK_TAKE_ALL].id = "keyTakeAll";
	GKey[GK_SELECT_DIALOG].id = "keySelectDialog";
	GKey[GK_SKIP_DIALOG].id = "keySkipDialog";
	GKey[GK_TALK_BOX].id = "keyTalkBox";
	GKey[GK_PAUSE].id = "keyPause";
	GKey[GK_YELL].id = "keyYell";
	GKey[GK_CONSOLE].id = "keyConsole";
	GKey[GK_ROTATE_CAMERA].id = "keyRotateCamera";
	GKey[GK_AUTOWALK].id = "keyAutowalk";
	GKey[GK_TOGGLE_WALK].id = "keyToggleWalk";

	for(int i = 0; i < GK_MAX; ++i)
		GKey[i].text = Str(GKey[i].id);
}

//=================================================================================================
void Game::ResetGameKeys()
{
	GKey[GK_MOVE_FORWARD].Set('W', VK_UP);
	GKey[GK_MOVE_BACK].Set('S', VK_DOWN);
	GKey[GK_MOVE_LEFT].Set('A', VK_LEFT);
	GKey[GK_MOVE_RIGHT].Set('D', VK_RIGHT);
	GKey[GK_WALK].Set(VK_SHIFT);
	GKey[GK_ROTATE_LEFT].Set('Q');
	GKey[GK_ROTATE_RIGHT].Set('E');
	GKey[GK_TAKE_WEAPON].Set(VK_SPACE);
	GKey[GK_ATTACK_USE].Set(VK_LBUTTON, 'Z');
	GKey[GK_USE].Set('R');
	GKey[GK_BLOCK].Set(VK_RBUTTON, 'X');
	GKey[GK_STATS].Set('C');
	GKey[GK_INVENTORY].Set('I');
	GKey[GK_TEAM_PANEL].Set('T');
	GKey[GK_ACTION_PANEL].Set('K');
	GKey[GK_JOURNAL].Set('J');
	GKey[GK_MINIMAP].Set('M');
	GKey[GK_QUICKSAVE].Set(VK_F5);
	GKey[GK_QUICKLOAD].Set(VK_F9);
	GKey[GK_POTION].Set('H');
	GKey[GK_MELEE_WEAPON].Set('1');
	GKey[GK_RANGED_WEAPON].Set('2');
	GKey[GK_ACTION].Set('3');
	GKey[GK_TAKE_ALL].Set('F');
	GKey[GK_SELECT_DIALOG].Set(VK_RETURN);
	GKey[GK_SKIP_DIALOG].Set(VK_SPACE);
	GKey[GK_TALK_BOX].Set('N');
	GKey[GK_PAUSE].Set(VK_PAUSE);
	GKey[GK_YELL].Set('Y');
	GKey[GK_CONSOLE].Set(VK_OEM_3);
	GKey[GK_ROTATE_CAMERA].Set('V');
	GKey[GK_AUTOWALK].Set('F');
	GKey[GK_TOGGLE_WALK].Set(VK_CAPITAL);
}

//=================================================================================================
void Game::SaveGameKeys()
{
	for(int i = 0; i < GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j = 0; j < 2; ++j)
			cfg.Add(Format("%s%d", k.id, j), Format("%d", k[j]));
	}

	SaveCfg();
}

//=================================================================================================
void Game::LoadGameKeys()
{
	for(int i = 0; i < GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j = 0; j < 2; ++j)
		{
			cstring s = Format("%s%d", k.id, j);
			int w = cfg.GetInt(s);
			if(w == VK_ESCAPE || w < -1 || w > 255)
			{
				Warn("Config: Invalid value for %s: %d.", s, w);
				w = -1;
				cfg.Add(s, Format("%d", k[j]));
			}
			if(w != -1)
				k[j] = (byte)w;
		}
	}
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
	// typ broni
	WeaponTypeInfo::info[WT_SHORT_BLADE].name = Str("wt_shortBlade");
	WeaponTypeInfo::info[WT_LONG_BLADE].name = Str("wt_longBlade");
	WeaponTypeInfo::info[WT_BLUNT].name = Str("wt_blunt");
	WeaponTypeInfo::info[WT_AXE].name = Str("wt_axe");
}

//=================================================================================================
void Game::SetGameText()
{
	txGoldPlus = Str("goldPlus");
	txQuestCompletedGold = Str("questCompletedGold");

	// ai
	LoadArray(txAiNoHpPot, "aiNoHpPot");
	LoadArray(txAiCity, "aiCity");
	LoadArray(txAiVillage, "aiVillage");
	txAiMoonwell = Str("aiMoonwell");
	txAiForest = Str("aiForest");
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
	txAiLabirynth = Str("aiLabirynth");
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
	LoadArray(txAiDrunkmanText, "aiDrunkmanText");

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
	txSavedGameN = Str("savedGameN");
	txLoadFailed = Str("loadFailed");
	txQuickSave = Str("quickSave");
	txGameSaved = Str("gameSaved");
	txLoadingLocations = Str("loadingLocations");
	txLoadingData = Str("loadingData");
	txLoadingQuests = Str("loadingQuests");
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
	txLoadError = Str("loadError");
	txLoadErrorGeneric = Str("loadErrorGeneric");
	txLoadOpenError = Str("loadOpenError");

	txPvpRefuse = Str("pvpRefuse");
	txWin = Str("win");
	txWinMp = Str("winMp");
	txLevelUp = Str("levelUp");
	txLevelDown = Str("levelDown");
	txLocationText = Str("locationText");
	txLocationTextMap = Str("locationTextMap");
	txRegeneratingLevel = Str("regeneratingLevel");
	txReallyQuit = Str("reallyQuit");
	txGainTextAttrib = Str("gainTextAttrib");
	txGainTextSkill = Str("gainTextSkill");
	txNeedItem = Str("needItem");
	txGmsAddedItems = Str("gmsAddedItems");
	
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
	LoadArray(txRumorQ, "rumorQ");
	txNeedMoreGold = Str("needMoreGold");
	txNoNearLoc = Str("noNearLoc");
	txNearLoc = Str("nearLoc");
	LoadArray(txNearLocEmpty, "nearLocEmpty");
	txNearLocCleared = Str("nearLocCleared");
	LoadArray(txNearLocEnemy, "nearLocEnemy");
	LoadArray(txNoNews, "noNews");
	LoadArray(txAllNews, "allNews");
	txNewsCampCleared = Str("newsCampCleared");
	txNewsLocCleared = Str("newsLocCleared");
	txAllNearLoc = Str("allNearLoc");

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
	txArthur = Str("arthur");
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
	txConnectTimeout = Str("connectTimeout");
	txConnectInvalid = Str("connectInvalid");
	txConnectVersion = Str("connectVersion");
	txConnectRaknet = Str("connectRaknet");
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
	txDisconnected = Str("disconnected");
	txLost = Str("lost");
	txLeft = Str("left");
	txLost2 = Str("left2");
	txUnconnected = Str("unconnected");
	txClosing = Str("closing");
	txKicked = Str("kicked");
	txUnknown = Str("unknown");
	txUnknown2 = Str("unknown2");
	txWaitingForServer = Str("waitingForServer");
	txStartingGame = Str("startingGame");
	txPreparingWorld = Str("preparingWorld");
	txInvalidCrc = Str("invalidCrc");

	// net
	txServer = Str("server");
	txPlayerKicked = Str("playerKicked");
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
	txPlayerLeft = Str("playerLeft");
	txPlayerDisconnected = Str("playerDisconnected");
	txPlayerQuit = Str("playerQuit");
	txPlayerKicked = Str("playerKicked");
	txServerClosed = Str("serverClosed");

	// obóz wrogów
	txSGOOrcs = Str("sgo_orcs");
	txSGOGoblins = Str("sgo_goblins");
	txSGOBandits = Str("sgo_bandits");
	txSGOEnemies = Str("sgo_enemies");
	txSGOUndead = Str("sgo_undead");
	txSGOMages = Str("sgo_mages");
	txSGOGolems = Str("sgo_golems");
	txSGOMagesAndGolems = Str("sgo_magesAndGolems");
	txSGOUnk = Str("sgo_unk");
	txSGOPowerfull = Str("sgo_powerfull");

	// rodzaje wrogów
	for(int i = 0; i < SG_MAX; ++i)
	{
		SpawnGroup& sg = g_spawn_groups[i];
		if(!sg.name)
			sg.name = Str(Format("sg_%s", sg.unit_group_id));
	}

	// dialogi
	LoadArray(txYell, "yell");

	TakenPerk::LoadText();
}

//=================================================================================================
Unit* Game::FindPlayerTradingWithUnit(Unit& u)
{
	for(Unit* unit : Team.active_members)
	{
		if(unit->IsPlayer() && unit->player->IsTradingWith(&u))
			return unit;
	}

	return nullptr;
}

//=================================================================================================
bool Game::ValidateTarget(Unit& u, Unit* target)
{
	assert(target);

	LevelContext& ctx = L.GetContext(u);

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == target)
			return true;
	}

	return false;
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
		PostEffect* e = nullptr, *e2;
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
		e->skill = e2->skill = Vec4(1.f / GetWindowSize().x*mod, 1.f / GetWindowSize().y*mod, 0, 0);
		// 0.1-0
		// 1-1
		e->power = e2->power = (drunk - 0.1f) / 0.9f;
	}
}

//=================================================================================================
void Game::PlayerYell(Unit& u)
{
	UnitTalk(u, RandomString(txYell));

	LevelContext& ctx = L.GetContext(u);
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(u2.IsAI() && u2.IsStanding() && !IsEnemy(u, u2) && !IsFriend(u, u2) && u2.busy == Unit::Busy_No && u2.frozen == FROZEN::NO && !u2.usable
			&& u2.ai->state == AIController::Idle && !IS_SET(u2.data->flags, F_AI_STAY)
			&& 	(u2.ai->idle_action == AIController::Idle_None || u2.ai->idle_action == AIController::Idle_Animation || u2.ai->idle_action == AIController::Idle_Rot
				|| u2.ai->idle_action == AIController::Idle_Look))
		{
			u2.ai->idle_action = AIController::Idle_MoveAway;
			u2.ai->idle_data.unit = &u;
			u2.ai->timer = Random(3.f, 5.f);
		}
	}
}

//=================================================================================================
bool Game::CanBuySell(const Item* item)
{
	assert(item);
	if(!trader_buy[item->type])
	{
		if(pc->action_unit->data->id == "food_seller")
		{
			if(item->id == "ladle" || item->id == "frying_pan")
				return true;
		}
		return false;
	}
	if(item->type == IT_CONSUMABLE)
	{
		if(pc->action_unit->data->id == "alchemist")
		{
			if(item->ToConsumable().cons_type != Potion)
				return false;
		}
		else if(pc->action_unit->data->id == "food_seller")
		{
			if(item->ToConsumable().cons_type == Potion)
				return false;
		}
	}
	return true;
}

//=================================================================================================
void Game::ReloadShaders()
{
	Info("Reloading shaders...");

	ReleaseShaders();

	super_shader->Init();

	try
	{
		eMesh = CompileShader("mesh.fx");
		eParticle = CompileShader("particle.fx");
		eSkybox = CompileShader("skybox.fx");
		eTerrain = CompileShader("terrain.fx");
		eArea = CompileShader("area.fx");
		eGui = CompileShader("gui.fx");
		ePostFx = CompileShader("post.fx");
		eGlow = CompileShader("glow.fx");
		eGrass = CompileShader("grass.fx");
	}
	catch(cstring err)
	{
		FatalError(Format("Failed to reload shaders.\n%s", err));
		return;
	}

	SetupShaders();
	GUI.SetShader(eGui);
}

//=================================================================================================
void Game::ReleaseShaders()
{
	SafeRelease(eMesh);
	SafeRelease(eParticle);
	SafeRelease(eTerrain);
	SafeRelease(eSkybox);
	SafeRelease(eArea);
	SafeRelease(eGui);
	SafeRelease(ePostFx);
	SafeRelease(eGlow);
	SafeRelease(eGrass);

	super_shader->Release();
}

//=================================================================================================
void Game::SetMeshSpecular()
{
	for(Weapon* weapon : Weapon::weapons)
	{
		Weapon& w = *weapon;
		if(w.mesh && w.mesh->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[w.material];
			for(int i = 0; i < w.mesh->head.n_subs; ++i)
			{
				w.mesh->subs[i].specular_intensity = mat.intensity;
				w.mesh->subs[i].specular_hardness = mat.hardness;
			}
		}
	}

	for(Shield* shield : Shield::shields)
	{
		Shield& s = *shield;
		if(s.mesh && s.mesh->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[s.material];
			for(int i = 0; i < s.mesh->head.n_subs; ++i)
			{
				s.mesh->subs[i].specular_intensity = mat.intensity;
				s.mesh->subs[i].specular_hardness = mat.hardness;
			}
		}
	}

	for(Armor* armor : Armor::armors)
	{
		Armor& a = *armor;
		if(a.mesh && a.mesh->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[a.material];
			for(int i = 0; i < a.mesh->head.n_subs; ++i)
			{
				a.mesh->subs[i].specular_intensity = mat.intensity;
				a.mesh->subs[i].specular_hardness = mat.hardness;
			}
		}
	}

	for(UnitData* ud_ptr : UnitData::units)
	{
		UnitData& ud = *ud_ptr;
		if(ud.mesh && ud.mesh->head.version < 18)
		{
			const MaterialInfo& mat = g_materials[ud.mat];
			for(int i = 0; i < ud.mesh->head.n_subs; ++i)
			{
				ud.mesh->subs[i].specular_intensity = mat.intensity;
				ud.mesh->subs[i].specular_hardness = mat.hardness;
			}
		}
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
	ClassInfo::Validate(err);
	Item::Validate(err);
	PerkInfo::Validate(err);
	RoomType::Validate(err);
	if(major)
		VerifyDialogs(err);

	if(err == 0)
		Info("Test: Validation succeeded.");
	else
		Error("Test: Validation failed, %u errors found.", err);

	return err;
}

//=================================================================================================
MeshInstance* Game::GetBowInstance(Mesh* mesh)
{
	if(bow_instances.empty())
		return new MeshInstance(mesh);
	else
	{
		MeshInstance* instance = bow_instances.back();
		bow_instances.pop_back();
		instance->mesh = mesh;
		return instance;
	}
}

void Game::SetupConfigVars()
{
	config_vars.push_back(ConfigVar("devmode", default_devmode));
	config_vars.push_back(ConfigVar("players_devmode", default_player_devmode));
}

void Game::ParseConfigVar(cstring arg)
{
	assert(arg);

	int index = StrCharIndex(arg, '=');
	if(index == -1 || index == 0)
	{
		Warn("Broken command line variable '%s'.", arg);
		return;
	}

	ConfigVar* var = nullptr;
	for(ConfigVar& v : config_vars)
	{
		if(strncmp(arg, v.name, index) == 0)
		{
			var = &v;
			break;
		}
	}
	if(!var)
	{
		Warn("Missing config variable '%.*s'.", index, arg);
		return;
	}

	cstring value = arg + index + 1;
	if(!*value)
	{
		Warn("Missing command line variable value '%s'.", arg);
		return;
	}

	switch(var->type)
	{
	case AnyVarType::Bool:
		{
			bool b;
			if(!TextHelper::ToBool(value, b))
			{
				Warn("Value for config variable '%s' must be bool, found '%s'.", var->name, value);
				return;
			}
			var->new_value._bool = b;
			var->have_new_value = true;
		}
		break;
	}
}

void Game::SetConfigVarsFromFile()
{
	for(ConfigVar& v : config_vars)
	{
		Config::Entry* entry = cfg.GetEntry(v.name);
		if(!entry)
			continue;

		switch(v.type)
		{
		case AnyVarType::Bool:
			if(!TextHelper::ToBool(entry->value.c_str(), v.ptr->_bool))
			{
				Warn("Value for config variable '%s' must be bool, found '%s'.", v.name, entry->value.c_str());
				return;
			}
			break;
		}
	}
}

void Game::ApplyConfigVars()
{
	for(ConfigVar& v : config_vars)
	{
		if(!v.have_new_value)
			continue;
		switch(v.type)
		{
		case AnyVarType::Bool:
			v.ptr->_bool = v.new_value._bool;
			break;
		}
	}
}

cstring Game::GetShortcutText(GAME_KEYS key, cstring action)
{
	auto& k = GKey[key];
	bool first_key = (k[0] != VK_NONE),
		second_key = (k[1] != VK_NONE);
	if(!action)
		action = k.text;

	if(first_key && second_key)
		return Format("%s (%s, %s)", action, gui->controls->key_text[k[0]], gui->controls->key_text[k[1]]);
	else if(first_key || second_key)
	{
		byte used = k[first_key ? 0 : 1];
		return Format("%s (%s)", action, gui->controls->key_text[used]);
	}
	else
		return action;
}

void Game::PauseGame()
{
	paused = !paused;
	if(Net::IsOnline())
	{
		AddMultiMsg(paused ? txGamePaused : txGameResumed);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PAUSED;
		c.id = (paused ? 1 : 0);
		if(paused && game_state == GS_WORLDMAP && W.GetState() == World::State::TRAVEL)
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

	W.GenerateWorld();

	Info("Randomness integrity: %d", RandVal());
}

void Game::EnterLocation(int level, int from_portal, bool close_portal)
{
	Location& l = *L.location;

	gui->world_map->visible = false;
	gui->game_gui->Reset();
	gui->game_gui->visible = true;

	const bool reenter = L.is_open;
	L.is_open = true;
	if(W.GetState() != World::State::INSIDE_ENCOUNTER)
		W.SetState(World::State::INSIDE_LOCATION);
	if(from_portal != -1)
		L.enter_from = ENTER_FROM_PORTAL + from_portal;
	else
		L.enter_from = ENTER_FROM_OUTSIDE;
	if(!reenter)
		L.light_angle = Random(PI * 2);

	L.dungeon_level = level;
	L.event_handler = nullptr;
	pc_data.before_player = BP_NONE;
	arena->Reset();
	gui->inventory->lock = nullptr;

	bool first = false;

	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		first = true;
		level_generated = true;
	}
	else
		level_generated = false;

	if(!reenter)
		InitQuadTree();

	if(Net::IsOnline() && N.active_players > 1)
	{
		BitStreamWriter f;
		f << ID_CHANGE_LEVEL;
		f << (byte)L.location_index;
		f << (byte)0;
		f << (W.GetState() == World::State::INSIDE_ENCOUNTER);
		int ack = N.SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, Stream_TransferServer);
		for(auto info : N.players)
		{
			if(info->id == my_id)
				info->state = PlayerInfo::IN_GAME;
			else
			{
				info->state = PlayerInfo::WAITING_FOR_RESPONSE;
				info->ack = ack;
				info->timer = 5.f;
			}
		}
		Net_FilterServerChanges();
	}

	// calculate number of loading steps for drawing progress bar
	LocationGenerator* loc_gen = loc_gen_factory->Get(&l, first, reenter);
	int steps = loc_gen->GetNumberOfSteps();
	LoadingStart(steps);
	LoadingStep(txEnteringLocation);

	// generate map on first visit
	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
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
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				multi->infos[L.dungeon_level].seed = l.seed;
			}
		}

		// nie odwiedzono, trzeba wygenerowaæ
		l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		loc_gen->Generate();
	}
	else if(l.type != L_DUNGEON && l.type != L_CRYPT)
		Info("Entering location '%s'.", l.name.c_str());

	L.city_ctx = nullptr;

	if(L.location->outside)
	{
		loc_gen->OnEnter();
		SetTerrainTextures();
		CalculateQuadtree();
	}
	else
		EnterLevel(loc_gen);

	bool loaded_resources = RequireLoadingResources(L.location, nullptr);
	LoadResources(txLoadingComplete, false);

	l.last_visit = W.GetWorldtime();
	CheckIfLocationCleared();
	cam.Reset();
	pc_data.rot_buf = 0.f;
	SetMusic();

	if(close_portal)
	{
		delete L.location->portal;
		L.location->portal = nullptr;
	}

	if(Net::IsOnline())
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		if(N.active_players > 1)
		{
			prepared_stream.Reset();
			PrepareLevelData(prepared_stream, loaded_resources);
			Info("Generated location packet: %d.", prepared_stream.GetNumberOfBytesUsed());
		}
		else
			N.GetMe().state = PlayerInfo::IN_GAME;

		gui->info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		gui->load_screen->visible = false;
		gui->main_menu->visible = false;
		gui->game_gui->visible = true;
	}

	if(L.location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");
}


// dru¿yna opuœci³a lokacje
void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(Net::IsLocal())
	{
		// zawody
		if(QM.quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			QM.quest_tournament->Clean();
		// arena
		if(!arena->free)
			arena->Clean();
	}

	if(clear)
	{
		if(L.is_open)
			LeaveLevel(true);
		return;
	}

	if(!L.is_open)
		return;

	Info("Leaving location.");

	if(Net::IsLocal() && (QM.quest_crazies->check_stone
		|| (QM.quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && QM.quest_crazies->crazies_state < Quest_Crazies::State::End)))
		QM.quest_crazies->CheckStone();

	// drinking contest
	Quest_Contest* contest = QM.quest_contest;
	if(contest->state >= Quest_Contest::CONTEST_STARTING)
		contest->Cleanup();

	// clear blood & bodies from orc base
	if(Net::IsLocal() && QM.quest_orcs2->orcs_state == Quest_Orcs2::State::ClearDungeon && L.location_index == QM.quest_orcs2->target_loc)
	{
		QM.quest_orcs2->orcs_state = Quest_Orcs2::State::End;
		L.UpdateLocation(31, 100, false);
	}

	if(L.city_ctx && game_state != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// opuszczanie miasta
		Team.BuyTeamItems();
	}

	LeaveLevel();

	if(L.is_open)
	{
		if(Net::IsLocal())
		{
			// usuñ questowe postacie
			RemoveQuestUnits(true);
		}

		L.ProcessRemoveUnits(true);

		if(L.location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = (OutsideLocation*)L.location;
			outside->bloods.clear();
			DeleteElements(outside->objects);
			DeleteElements(outside->chests);
			DeleteElements(outside->items);
			outside->objects.clear();
			DeleteElements(outside->usables);
			for(vector<Unit*>::iterator it = outside->units.begin(), end = outside->units.end(); it != end; ++it)
				delete *it;
			outside->units.clear();
		}
	}

	if(Team.crazies_attack)
	{
		Team.crazies_attack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	if(!Net::IsLocal())
		pc = nullptr;
	else if(end_buffs)
	{
		// usuñ tymczasowe bufy
		for(Unit* unit : Team.members)
			unit->EndEffects();
	}

	L.is_open = false;
	L.city_ctx = nullptr;
}

void Game::Event_RandomEncounter(int)
{
	gui->world_map->dialog_enc = nullptr;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CLOSE_ENCOUNTER);
	W.StartEncounter();
	EnterLocation();
}

//=================================================================================================
void Game::OnResize()
{
	gui->OnResize();
}

//=================================================================================================
void Game::OnFocus(bool focus, const Int2& activation_point)
{
	gui->OnFocus(focus, activation_point);
}
