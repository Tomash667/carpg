// game
#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Terrain.h"
#include "ParticleSystem.h"
#include "Language.h"
#include "Version.h"
#include "CityGenerator.h"
#include "Quest_Mages.h"
#include "Content.h"
#include "DatatypeManager.h"
#include "toolset/Toolset.h"

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
Game::Game() : have_console(false), vbParticle(nullptr), peer(nullptr), quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0),
console_open(false), cl_fog(true), cl_lighting(true), draw_particle_sphere(false), draw_unit_radius(false), draw_hitbox(false), noai(false), testing(0),
game_speed(1.f), devmode(false), draw_phy(false), draw_col(false), force_seed(0), next_seed(0), force_seed_all(false),
obj_alpha("tmp_alpha", 0, 0, "tmp_alpha", nullptr, 1), alpha_test_state(-1), debug_info(false), dont_wander(false), exit_mode(false), local_ctx_valid(false),
city_ctx(nullptr), check_updates(true), skip_version(-1), skip_tutorial(false), sv_online(false), portal_anim(0), nosound(false), nomusic(false),
debug_info2(false), music_type(MusicType::None), contest_state(CONTEST_NOT_DONE), koniec_gry(false), net_stream(64*1024), net_stream2(64*1024),
exit_to_menu(false), mp_interp(0.05f), mp_use_interp(true), mp_port(PORT), paused(false), pick_autojoin(false), draw_flags(0xFFFFFFFF), tMiniSave(nullptr),
prev_game_state(GS_LOAD), clearup_shutdown(false), tSave(nullptr), sItemRegion(nullptr), sChar(nullptr), sSave(nullptr), in_tutorial(false),
cursor_allow_move(true), mp_load(false), was_client(false), sCustom(nullptr), cl_postfx(true), mp_timeout(10.f), sshader_pool(nullptr), cl_normalmap(true),
cl_specularmap(true), dungeon_tex_wrap(true), mutex(nullptr), profiler_mode(0), grass_range(40.f), vbInstancing(nullptr), vb_instancing_max(0),
screenshot_format(D3DXIFF_JPG), quickstart_class(Class::RANDOM), autopick_class(Class::INVALID), current_packet(nullptr),
game_state(GS_LOAD), default_devmode(false), default_player_devmode(false), toolset(nullptr), dt_mgr(nullptr)
{
#ifdef _DEBUG
	default_devmode = true;
	default_player_devmode = true;
#endif

	devmode = default_devmode;

	game = this;
	Quest::game = this;

	dialog_context.is_local = true;

	ClearPointers();
	NullGui();

	light_angle = 0.f;
	uv_mod = Terrain::DEFAULT_UV_MOD;
	cam.draw_range = 80.f;

	gen = new CityGenerator;

	SetupConfigVars();
}

//=================================================================================================
Game::~Game()
{
	CleanupDatatypeManager();
	delete gen;
	delete toolset;
}

//=================================================================================================
// Rysowanie gry
//=================================================================================================
void Game::OnDraw()
{
	if(game_state != GS_TOOLSET)
		OnDraw(true);
	else
	{
		V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0));
		V(device->BeginScene());
		toolset->OnDraw();
		V(device->EndScene());
	}
}

//=================================================================================================
void Game::OnDraw(bool normal)
{
	if(normal)
	{
		if(profiler_mode == 2)
			g_profiler.Start();
		else if(profiler_mode == 0)
			g_profiler.Clear();
	}

	if(post_effects.empty() || !ePostFx)
	{
		if(sCustom)
			V( device->SetRenderTarget(0, sCustom) );

		V( device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0) );
		V( device->BeginScene() );

		if(game_state == GS_LEVEL)
		{
			// draw level
			Draw();

			// draw glow
			if(before_player != BP_NONE && !draw_batch.glow_nodes.empty())
			{
				V( device->EndScene() );
				DrawGlowingNodes(false);
				V( device->BeginScene() );
			}
		}

		// draw gui
		GUI.mViewProj = cam.matViewProj;
		GUI.Draw(wnd_size);

		V( device->EndScene() );
	}
	else
	{
		// renderuj scenê do tekstury
		SURFACE sPost;
		if(!IsMultisamplingEnabled())
			V( tPostEffect[2]->GetSurfaceLevel(0, &sPost) );
		else
			sPost = sPostEffect[2];
		
		V( device->SetRenderTarget(0, sPost) );
		V( device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET | D3DCLEAR_STENCIL, clear_color, 1.f, 0) );

		if(game_state == GS_LEVEL)
		{
			V( device->BeginScene() );
			Draw();
			V( device->EndScene() );
			if(before_player != BP_NONE && !draw_batch.glow_nodes.empty())
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
			V( tPostEffect[0]->GetSurfaceLevel(0, &surf2) );
			V( device->StretchRect(sPost, nullptr, surf2, nullptr, D3DTEXF_NONE) );
			surf2->Release();
			t = tPostEffect[0];
		}

		// post effects
		V( device->SetVertexDeclaration(vertex_decl[VDI_TEX]) );
		V( device->SetStreamSource(0, vbFullscreen, 0, sizeof(VTex)) );
		SetAlphaTest(false);
		SetAlphaBlend(false);
		SetNoCulling(false);
		SetNoZWrite(true);

		UINT passes;
		int index_surf = 1;
		for(vector<PostEffect>::iterator it = post_effects.begin(), end = post_effects.end(); it != end; ++it)
		{
			SURFACE surf;
			if(it+1 == end)
			{
				// ostatni pass
				if(sCustom)
					surf = sCustom;
				else
					V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
			}
			else
			{
				// jest nastêpny
				if(!IsMultisamplingEnabled())
					V( tPostEffect[index_surf]->GetSurfaceLevel(0, &surf) );
				else
					surf = sPostEffect[index_surf];
			}

			V( device->SetRenderTarget(0, surf) );
			V( device->BeginScene() );

			V( ePostFx->SetTechnique(it->tech) );
			V( ePostFx->SetTexture(hPostTex, t) );
			V( ePostFx->SetFloat(hPostPower, it->power) );
			V( ePostFx->SetVector(hPostSkill, &it->skill) );

			V( ePostFx->Begin(&passes, 0) );
			V( ePostFx->BeginPass(0) );
			V( device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2) );
			V( ePostFx->EndPass() );
			V( ePostFx->End() );

			if(it+1 == end)
			{
				GUI.mViewProj = cam.matViewProj;
				GUI.Draw(wnd_size);
			}

			V( device->EndScene() );

			if(it+1 == end)
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
				V( tPostEffect[0]->GetSurfaceLevel(0, &surf2) );
				V( device->StretchRect(surf, nullptr, surf2, nullptr, D3DTEXF_NONE) );
				surf2->Release();
				t = tPostEffect[0];
			}
			
			index_surf = (index_surf + 1) % 3;
		}
	}

	g_profiler.End();
}

//=================================================================================================
void HumanPredraw(void* ptr, MATRIX* mat, int n)
{
	if(n != 0)
		return;

	Unit* u = (Unit*)ptr;

	if(u->type == Unit::HUMAN)
	{
		int bone = u->ani->ani->GetBone("usta")->id;
		static MATRIX mat2;
		float val = u->talking ? sin(u->talk_timer*6) : 0.f;
		D3DXMatrixRotationX(&mat2, val/5);
		mat[bone] = mat2 * mat[bone];
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

	if(game_state == GS_TOOLSET)
	{
		if(!toolset->OnUpdate(dt))
			SetToolsetState(false);
	}

	if(profiler_mode == 1)
		g_profiler.Start();
	else if(profiler_mode == 0)
		g_profiler.Clear();

	UpdateMusic();
	
	if(!IsOnline() || !paused)
	{
		// aktualizacja czasu spêdzonego w grze
		if(game_state != GS_MAIN_MENU && game_state != GS_LOAD)
		{
			gt_tick += dt;
			if(gt_tick >= 1.f)
			{
				gt_tick -= 1.f;
				++gt_second;
				if(gt_second == 60)
				{
					++gt_minute;
					gt_second = 0;
					if(gt_minute == 60)
					{
						++gt_hour;
						gt_minute = 0;
					}
				}
			}
		}
	}

	allow_input = ALLOW_INPUT;

	// utracono urz¹dzenie directx lub okno nie aktywne
	if(IsLostDevice() || !active || !locked_cursor)
	{
		Key.SetFocus(false);
		if(!IsOnline() && !inactive_update)
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
		console_open = false;
		death_fade += dt;
		if(death_fade >= 1.f && AllowKeyboard() && Key.PressedRelease(VK_ESCAPE))
		{
			ExitToMenu();
			koniec_gry = false;
		}
		allow_input = ALLOW_NONE;
	}

	// globalna obs³uga klawiszy
	if(allow_input == ALLOW_INPUT)
	{
		// konsola
		if(!GUI.HaveTopDialog("dialog_alt_f4") && !GUI.HaveDialog("console") && KeyDownUpAllowed(GK_CONSOLE))
			GUI.ShowDialog(console);

		// uwolnienie myszki
		if(!fullscreen && active && locked_cursor && Key.Shortcut(VK_CONTROL,'U'))
			UnlockCursor();

		// zmiana trybu okna
		if(Key.Shortcut(VK_MENU, VK_RETURN))
			ChangeMode(!fullscreen);

		// screenshot
		if(Key.PressedRelease(VK_SNAPSHOT))
			TakeScreenshot(false, Key.Down(VK_SHIFT));

		// zatrzymywanie/wznawianie gry
		if(KeyPressedReleaseAllowed(GK_PAUSE))
		{
			if(!IsOnline())
				paused = !paused;
			else if(IsServer())
			{
				paused = !paused;
				AddMultiMsg(paused ? txGamePaused : txGameResumed);
				NetChange& c = Add1(net_changes);
				c.type = NetChange::PAUSED;
				c.id = (paused ? 1 : 0);
				if(paused && game_state == GS_WORLDMAP && world_state == WS_TRAVEL)
					PushNetChange(NetChange::UPDATE_MAP_POS);
			}
		}
	}

	// obs³uga paneli
	if(GUI.HaveDialog() || (game_gui->mp_box->visible && game_gui->mp_box->itb.focus))
		allow_input = ALLOW_NONE;
	else if(AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode)
	{
		OpenPanel open = game_gui->GetOpenPanel(),
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
		else if(open == OpenPanel::Trade && Key.PressedRelease(VK_ESCAPE))
			to_open = OpenPanel::Trade;

		if(to_open != OpenPanel::None)
			game_gui->ShowPanel(to_open, open);

		switch(open)
		{
		case OpenPanel::None:
		case OpenPanel::Minimap:
		default:
			if(game_gui->use_cursor)
				allow_input = ALLOW_KEYBOARD;
			break;
		case OpenPanel::Stats:
		case OpenPanel::Inventory:
		case OpenPanel::Team:
		case OpenPanel::Trade:
			allow_input = ALLOW_KEYBOARD;
			break;
		case OpenPanel::Journal:
			allow_input = ALLOW_NONE;
			break;
		}
	}

	// szybkie zapisywanie
	if(KeyPressedReleaseAllowed(GK_QUICKSAVE))
		Quicksave(false);

	// szybkie wczytywanie
	if(KeyPressedReleaseAllowed(GK_QUICKLOAD))
		Quickload(false);

	// mp box
	if((game_state == GS_LEVEL || game_state == GS_WORLDMAP) && KeyPressedReleaseAllowed(GK_TALK_BOX))
		game_gui->mp_box->visible = !game_gui->mp_box->visible;

	// aktualizuj gui
	UpdateGui(dt);
	if(exit_mode)
		return;

	// otwórz menu
	if(AllowKeyboard() && CanShowMenu() && Key.PressedRelease(VK_ESCAPE))
		ShowMenu();

	if(game_menu->visible)
		game_menu->Set(CanSaveGame(), CanLoadGame(), hardcore_mode);

	if(!paused)
	{
		// pytanie o pvp
		if(game_state == GS_LEVEL && IsOnline())
		{
			if(pvp_response.ok)
			{
				pvp_response.timer += dt;
				if(pvp_response.timer >= 5.f)
				{
					pvp_response.ok = false;
					if(pvp_response.to == pc->unit)
					{
						dialog_pvp->CloseDialog();
						RemoveElement(GUI.created_dialogs, dialog_pvp);
						delete dialog_pvp;
						dialog_pvp = nullptr;
					}
					if(IsServer())
					{
						if(pvp_response.from == pc->unit)
							AddMsg(Format(txPvpRefuse, pvp_response.to->player->name.c_str()));
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::NO_PVP;
							c.pc = pvp_response.from->player;
							c.id = pvp_response.to->player->id;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
			}
		}
		else
			pvp_response.ok = false;
	}

	// aktualizacja gry
	if(!koniec_gry)
	{
		if(game_state == GS_LEVEL)
		{
			if(paused)
			{
				if(IsOnline())
					UpdateGameNet(dt);
			}
			else if(GUI.HavePauseDialog())
			{
				if(IsOnline())
					UpdateGame(dt);
			}
			else
				UpdateGame(dt);
		}
		else if(game_state == GS_WORLDMAP)
		{
			if(IsOnline())
				UpdateGameNet(dt);
		}

		if(!IsOnline() && game_state != GS_MAIN_MENU)
		{
			assert(net_changes.empty());
			assert(net_changes_player.empty());
		}
	}
	else if(IsOnline())
		UpdateGameNet(dt);

	// aktywacja mp_box
	if(AllowKeyboard() && game_state == GS_LEVEL && game_gui->mp_box->visible && !game_gui->mp_box->itb.focus && Key.PressedRelease(VK_RETURN))
	{
		game_gui->mp_box->itb.focus = true;
		game_gui->mp_box->Event(GuiEvent_GainFocus);
		game_gui->mp_box->itb.Event(GuiEvent_GainFocus);
	}

	g_profiler.End();
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

	if(change_title_a && ((game_state != GS_MAIN_MENU && game_state != GS_LOAD) || (server_panel && server_panel->visible)))
	{
		if(sv_online)
		{
			if(sv_server)
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
bool Game::Start0(bool _fullscreen, int w, int h)
{
	LocalString s;
	GetTitle(s);
	return Start(s->c_str(), _fullscreen, w, h);
}

struct Point
{
	INT2 pt, prev;
};

struct AStarSort
{
	AStarSort(vector<APoint>& a_map, int rozmiar) : a_map(a_map), rozmiar(rozmiar)
	{
	}

	bool operator() (const Point& pt1, const Point& pt2) const
	{
		return a_map[pt1.pt.x+pt1.pt.y*rozmiar].suma > a_map[pt2.pt.x+pt2.pt.y*rozmiar].suma;
	}

	vector<APoint>& a_map;
	int rozmiar;
};

//=================================================================================================
void Game::OnReload()
{
	GUI.OnReload();

	V( eMesh->OnResetDevice() );
	V( eParticle->OnResetDevice() );
	V( eTerrain->OnResetDevice() );
	V( eSkybox->OnResetDevice() );
	V( eArea->OnResetDevice() );
	V( eGui->OnResetDevice() );
	V( ePostFx->OnResetDevice() );
	V( eGlow->OnResetDevice() );
	V( eGrass->OnResetDevice() );

	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
		V( it->e->OnResetDevice() );

	V( device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD) );
	V( device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) );
	V( device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) );
	V( device->SetRenderState(D3DRS_ALPHAREF, 200) );
	V( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL) );

	CreateTextures();
	BuildDungeon();
	RebuildMinimap();

	r_alphatest = false;
	r_alphablend = false;
	r_nocull = false;
	r_nozwrite = false;
}

//=================================================================================================
void Game::OnReset()
{
	GUI.OnReset();

	V( eMesh->OnLostDevice() );
	V( eParticle->OnLostDevice() );
	V( eTerrain->OnLostDevice() );
	V( eSkybox->OnLostDevice() );
	V( eArea->OnLostDevice() );
	V( eGui->OnLostDevice() );
	V( ePostFx->OnLostDevice() );
	V( eGlow->OnLostDevice() );
	V( eGrass->OnLostDevice() );

	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
		V( it->e->OnLostDevice() );

	SafeRelease(tItemRegion);
	SafeRelease(tMinimap);
	SafeRelease(tChar);
	SafeRelease(tSave);
	SafeRelease(sItemRegion);
	SafeRelease(sChar);
	SafeRelease(sSave);
	for(int i=0; i<3; ++i)
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
	if((c != 0x08 && c != 0x0D && byte(c) < 0x20) || c == '`')
		return;

	GUI.OnChar(c);
}

//=================================================================================================
void Game::TakeScreenshot(bool text, bool no_gui)
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
		if(text)
			AddConsoleMsg(txSsFailed);
		ERROR(Format("Failed to get front buffer data to save screenshot (%d)!", hr));
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
		switch(screenshot_format)
		{
		case D3DXIFF_BMP:
			ext = "bmp";
			break;
		default:
		case D3DXIFF_JPG:
			ext = "jpg";
			break;
		case D3DXIFF_TGA:
			ext = "tga";
			break;
		case D3DXIFF_PNG:
			ext = "png";
			break;
		}

		cstring path = Format("screenshots\\%04d%02d%02d_%02d%02d%02d_%02d.%s", lt.tm_year+1900, lt.tm_mon+1,
			lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, screenshot_count, ext);

		D3DXSaveSurfaceToFileA(path, screenshot_format, back_buffer, nullptr, nullptr);

		if(text)
			AddConsoleMsg(Format(txSsDone, path));
		LOG(Format("Screenshot saved to '%s'.", path));

		back_buffer->Release();
	}
}

//=================================================================================================
void Game::ExitToMenu()
{
	if(sv_online)
		CloseConnection(VoidF(this, &Game::DoExitToMenu));
	else
		DoExitToMenu();
}

//=================================================================================================
void Game::DoExitToMenu()
{
	exit_mode = true;

	StopSounds();
	attached_sounds.clear();
	ClearGame();
	game_state = GS_MAIN_MENU;

	exit_mode = false;
	paused = false;
	mp_load = false;
	was_client = false;

	SetMusic(MusicType::Title);
	contest_state = CONTEST_NOT_DONE;
	koniec_gry = false;
	exit_to_menu = true;

	CloseAllPanels();
	GUI.CloseDialogs();
	game_menu->visible = false;
	game_gui->visible = false;
	world_map->visible = false;
	main_menu->visible = true;

	if(change_title_a)
		ChangeTitle();
}

//=================================================================================================
// szuka œcie¿ki u¿ywaj¹c algorytmu A-Star
// zwraca true jeœli znaleziono i w wektorze jest ta œcie¿ka, w œcie¿ce nie ma pocz¹tkowego kafelka
bool Game::FindPath(LevelContext& ctx, const INT2& _start_tile, const INT2& _target_tile, vector<INT2>& _path, bool can_open_doors, bool wedrowanie, vector<INT2>* blocked)
{
	if(_start_tile == _target_tile || ctx.type == LevelContext::Building)
	{
		// jest w tym samym miejscu
		_path.clear();
		_path.push_back(_target_tile);
		return true;
	}

	static vector<Point> do_sprawdzenia;
	do_sprawdzenia.clear();

	if(ctx.type == LevelContext::Outside)
	{
		// zewnêtrze
		OutsideLocation* outside = (OutsideLocation*)location;
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		// czy poza map¹
		if(!outside->IsInside(_start_tile) || !outside->IsInside(_target_tile))
			return false;

		// czy na blokuj¹cym miejscu
		if(m[_start_tile(w)].IsBlocking() || m[_target_tile(w)].IsBlocking())
			return false;

		// powiêksz mapê
		const uint size = uint(w*w);
		if(size > a_map.size())
			a_map.resize(size);

		// wyzeruj
		memset(&a_map[0], 0, sizeof(APoint)*size);
		_path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<INT2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].stan = 1;
			if(a_map[_start_tile(w)].stan == 1 || a_map[_target_tile(w)].stan == 1)
				return false;
		}

		// dodaj pierwszy kafelek do sprawdzenia
		APoint apt, prev_apt;
		apt.suma = apt.odleglosc = 10 * (abs(_target_tile.x - _start_tile.x) + abs(_target_tile.y - _start_tile.y));
		apt.stan = 1;
		apt.koszt = 0;
		a_map[_start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = _start_tile;
		do_sprawdzenia.push_back(pt);

		AStarSort sortownik(a_map, w);

		// szukaj drogi
		while(!do_sprawdzenia.empty())
		{
			pt = do_sprawdzenia.back();
			do_sprawdzenia.pop_back();

			apt = a_map[pt.pt(w)];
			prev_apt = a_map[pt.prev(w)];

			if(pt.pt == _target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.stan = 1;
				ept.prev = pt.prev;
				break;
			}

			const INT2 kierunek[4] = {
				INT2(0,1),
				INT2(1,0),
				INT2(0,-1),
				INT2(-1,0)
			};

			const INT2 kierunek2[4] = {
				INT2(1,1),
				INT2(1,-1),
				INT2(-1,1),
				INT2(-1,-1)
			};

			for(int i=0; i<4; ++i)
			{
				const INT2& pt1 = kierunek[i] + pt.pt;
				const INT2& pt2 = kierunek2[i] + pt.pt;

				if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<w-1 && a_map[pt1(w)].stan == 0 && !m[pt1(w)].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.koszt = prev_apt.koszt + 10;
					if(wedrowanie)
					{
						TERRAIN_TILE type = m[pt1(w)].t;
						if(type == TT_SAND)
							apt.koszt += 10;
						else if(type != TT_ROAD)
							apt.koszt += 30;
					}
					apt.odleglosc = (abs(pt1.x - _target_tile.x)+abs(pt1.y-_target_tile.y))*10;
					apt.suma = apt.odleglosc + apt.koszt;
					apt.stan = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}

				if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<w-1 && a_map[pt2(w)].stan == 0 &&
					!m[pt2(w)].IsBlocking() &&
					!m[kierunek2[i].x+pt.pt.x+pt.pt.y*w].IsBlocking() &&
					!m[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.koszt = prev_apt.koszt + 15;
					if(wedrowanie)
					{
						TERRAIN_TILE type = m[pt2(w)].t;
						if(type == TT_SAND)
							apt.koszt += 10;
						else if(type != TT_ROAD)
							apt.koszt += 30;
					}
					apt.odleglosc = (abs(pt2.x - _target_tile.x)+abs(pt2.y-_target_tile.y))*10;
					apt.suma = apt.odleglosc + apt.koszt;
					apt.stan = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}

			std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
		}

		// jeœli cel jest niezbadany to nie znaleziono œcie¿ki
		if(a_map[_target_tile(w)].stan == 0)
			return false;

		// wype³nij elementami œcie¿kê
		_path.push_back(_target_tile);

		INT2 p;

		while((p = _path.back()) != _start_tile)
			_path.push_back(a_map[p(w)].prev);
	}
	else
	{
		// wnêtrze
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Pole* m = lvl.map;
		const int w = lvl.w, h = lvl.h;

		// czy poza map¹
		if(!lvl.IsInside(_start_tile) || !lvl.IsInside(_target_tile))
			return false;

		// czy na blokuj¹cym miejscu
		if(czy_blokuje2(m[_start_tile(w)]) || czy_blokuje2(m[_target_tile(w)]))
			return false;

		// powiêksz mapê
		const uint size = uint(w*h);
		if(size > a_map.size())
			a_map.resize(size);

		// wyzeruj
		memset(&a_map[0], 0, sizeof(APoint)*size);
		_path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<INT2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].stan = 1;
			if(a_map[_start_tile(w)].stan == 1 || a_map[_target_tile(w)].stan == 1)
				return false;
		}

		// dodaj pierwszy kafelek do sprawdzenia
		APoint apt, prev_apt;
		apt.suma = apt.odleglosc = 10 * (abs(_target_tile.x - _start_tile.x) + abs(_target_tile.y - _start_tile.y));
		apt.stan = 1;
		apt.koszt = 0;
		a_map[_start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = _start_tile;
		do_sprawdzenia.push_back(pt);

		AStarSort sortownik(a_map, w);

		// szukaj drogi
		while(!do_sprawdzenia.empty())
		{
			pt = do_sprawdzenia.back();
			do_sprawdzenia.pop_back();

			prev_apt = a_map[pt.pt(w)];

			//LOG(Format("(%d,%d)->(%d,%d), K:%d D:%d S:%d", pt.prev.x, pt.prev.y, pt.pt.x, pt.pt.y, prev_apt.koszt, prev_apt.odleglosc, prev_apt.suma));

			if(pt.pt == _target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.stan = 1;
				ept.prev = pt.prev;
				break;
			}

			const INT2 kierunek[4] = {
				INT2(0,1),
				INT2(1,0),
				INT2(0,-1),
				INT2(-1,0)
			};

			const INT2 kierunek2[4] = {
				INT2(1,1),
				INT2(1,-1),
				INT2(-1,1),
				INT2(-1,-1)
			};

			if(can_open_doors)
			{
				for(int i=0; i<4; ++i)
				{
					const INT2& pt1 = kierunek[i] + pt.pt;
					const INT2& pt2 = kierunek2[i] + pt.pt;

					if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<h-1 && !czy_blokuje2(m[pt1(w)]))
					{
						apt.prev = pt.pt;
						apt.koszt = prev_apt.koszt + 10;
						apt.odleglosc = (abs(pt1.x - _target_tile.x)+abs(pt1.y - _target_tile.y))*10;
						apt.suma = apt.odleglosc + apt.koszt;

						if(a_map[pt1(w)].IsLower(apt.suma))
						{
							apt.stan = 1;
							a_map[pt1(w)] = apt;

							new_pt.pt = pt1;
							new_pt.prev = pt.pt;
							do_sprawdzenia.push_back(new_pt);
						}
					}

					if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<h-1 &&
						!czy_blokuje2(m[pt2(w)]) &&
						!czy_blokuje2(m[kierunek2[i].x+pt.pt.x+pt.pt.y*w]) &&
						!czy_blokuje2(m[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]))
					{
						apt.prev = pt.pt;
						apt.koszt = prev_apt.koszt + 15;
						apt.odleglosc = (abs(pt2.x - _target_tile.x)+abs(pt2.y - _target_tile.y))*10;
						apt.suma = apt.odleglosc + apt.koszt;

						if(a_map[pt2(w)].IsLower(apt.suma))
						{
							apt.stan = 1;
							a_map[pt2(w)] = apt;

							new_pt.pt = pt2;
							new_pt.prev = pt.pt;
							do_sprawdzenia.push_back(new_pt);
						}
					}
				}
			}
			else
			{
				for(int i=0; i<4; ++i)
				{
					const INT2& pt1 = kierunek[i] + pt.pt;
					const INT2& pt2 = kierunek2[i] + pt.pt;

					if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<h-1 && !czy_blokuje2(m[pt1(w)]))
					{
						int ok = 2; // 2-ok i dodaj, 1-ok, 0-nie

						if(m[pt1(w)].type == DRZWI)
						{
							Door* door = FindDoor(ctx, pt1);
							if(door && door->IsBlocking())
							{
								// ustal gdzie s¹ drzwi na polu i czy z tej strony mo¿na na nie wejœæ
								if(czy_blokuje2(lvl.map[pt1.x - 1 + pt1.y*lvl.w].type))
								{
									// #   #
									// #---#
									// #   #
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y - 1)*lvl.w].room].IsCorridor())
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y + 1)*lvl.w].room].IsCorridor())
										--mov;
									if(mov == 1)
									{
										// #---#
										// #   #
										// #   #
										if(i != 0)
											ok = 0;
									}
									else if(mov == -1)
									{
										// #   #
										// #   #
										// #---#
										if(i != 2)
											ok = 0;
									}
									else
										ok = 1;
								}
								else
								{
									// ###
									//  | 
									//  | 
									// ###
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x - 1 + pt1.y*lvl.w].room].IsCorridor())
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + 1 + pt1.y*lvl.w].room].IsCorridor())
										--mov;
									if(mov == 1)
									{
										// ###
										//   |
										//   |
										// ###
										if(i != 1)
											ok = 0;
									}
									else if(mov == -1)
									{
										// ###
										// |
										// |
										// ###
										if(i != 3)
											ok = 0;
									}
									else
										ok = 1;
								}
							}
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.koszt = prev_apt.koszt + 10;
							apt.odleglosc = (abs(pt1.x - _target_tile.x)+abs(pt1.y - _target_tile.y))*10;
							apt.suma = apt.odleglosc + apt.koszt;

							if(a_map[pt1(w)].IsLower(apt.suma))
							{
								apt.stan = 1;
								a_map[pt1(w)] = apt;

								if(ok == 2)
								{
									new_pt.pt = pt1;
									new_pt.prev = pt.pt;
									do_sprawdzenia.push_back(new_pt);
								}
							}
						}
					}

					if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<h-1 &&
						!czy_blokuje2(m[pt2(w)]) &&
						!czy_blokuje2(m[kierunek2[i].x+pt.pt.x+pt.pt.y*w]) &&
						!czy_blokuje2(m[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]))
					{
						bool ok = true;

						if(m[pt2(w)].type == DRZWI)
						{
							Door* door = FindDoor(ctx, pt2);
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[kierunek2[i].x + pt.pt.x + pt.pt.y*w].type == DRZWI)
						{
							Door* door = FindDoor(ctx, INT2(kierunek2[i].x+pt.pt.x, pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[pt.pt.x + (kierunek2[i].y + pt.pt.y)*w].type == DRZWI)
						{
							Door* door = FindDoor(ctx, INT2(pt.pt.x, kierunek2[i].y+pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.koszt = prev_apt.koszt + 15;
							apt.odleglosc = (abs(pt2.x - _target_tile.x)+abs(pt2.y - _target_tile.y))*10;
							apt.suma = apt.odleglosc + apt.koszt;

							if(a_map[pt2(w)].IsLower(apt.suma))
							{
								apt.stan = 1;
								a_map[pt2(w)] = apt;

								new_pt.pt = pt2;
								new_pt.prev = pt.pt;
								do_sprawdzenia.push_back(new_pt);
							}
						}
					}
				}
			}

			std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
		}

		// jeœli cel jest niezbadany to nie znaleziono œcie¿ki
		if(a_map[_target_tile(w)].stan == 0)
			return false;

		// wype³nij elementami œcie¿kê
		_path.push_back(_target_tile);

		INT2 p;

		while((p = _path.back()) != _start_tile)
			_path.push_back(a_map[p(w)].prev);
	}

	// usuñ ostatni element œcie¿ki
	_path.pop_back();

	return true;
}

//=================================================================================================
INT2 Game::RandomNearTile(const INT2& _tile)
{
	struct DoSprawdzenia
	{
		int ile;
		INT2 tile[3];
	};
	static vector<INT2> tiles;

	const DoSprawdzenia allowed[] = {
		{2, INT2(-2,0), INT2(-1,0), INT2(0,0)},
		{1, INT2(-1,0), INT2(0,0), INT2(0,0)},
		{3, INT2(-1,1), INT2(-1,0), INT2(0,1)},
		{3, INT2(-1,-1), INT2(-1,0), INT2(0,-1)},
		{2, INT2(2,0), INT2(1,0), INT2(0,0)},
		{1, INT2(1,0), INT2(0,0), INT2(0,0)},
		{3, INT2(1,1), INT2(1,0), INT2(0,1)},
		{3, INT2(1,-1), INT2(1,0), INT2(0,-1)},
		{2, INT2(0,2), INT2(0,1), INT2(0,0)},
		{1, INT2(0,1), INT2(0,0), INT2(0,0)},
		{2, INT2(0,-2), INT2(0,0), INT2(0,0)},
		{1, INT2(0,-1),  INT2(0,0), INT2(0,0)}
	};

	bool blokuje = true;

	if(location->outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		for(uint i=0; i<12; ++i)
		{
			const int x = _tile.x + allowed[i].tile[0].x,
				y = _tile.y + allowed[i].tile[0].y;
			if(!outside->IsInside(x, y))
				continue;
			if(m[x+y*w].IsBlocking())
				continue;
			blokuje = false;
			for(int j=1; j<allowed[i].ile; ++j)
			{
				if(m[_tile.x+allowed[i].tile[j].x+(_tile.y+allowed[i].tile[j].y)*w].IsBlocking())
				{
					blokuje = true;
					break;
				}
			}
			if(!blokuje)
				tiles.push_back(allowed[i].tile[0]);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Pole* m = lvl.map;
		const int w = lvl.w;

		for(uint i=0; i<12; ++i)
		{
			const int x = _tile.x + allowed[i].tile[0].x,
				      y = _tile.y + allowed[i].tile[0].y;
			if(!lvl.IsInside(x, y))
				continue;
			if(czy_blokuje2(m[x+y*w]))
				continue;
			blokuje = false;
			for(int j=1; j<allowed[i].ile; ++j)
			{
				if(czy_blokuje2(m[_tile.x+allowed[i].tile[j].x+(_tile.y+allowed[i].tile[j].y)*w]))
				{
					blokuje = true;
					break;
				}
			}
			if(!blokuje)
				tiles.push_back(allowed[i].tile[0]);
		}
	}

	if(tiles.empty())
		return _tile;
	else
		return tiles[rand2()%tiles.size()] + _tile;
}

//=================================================================================================
// 0 - path found
// 1 - start pos and target pos too far
// 2 - start location is blocked
// 3 - start tile and target tile is equal
// 4 - target tile is blocked
// 5 - path not found
int Game::FindLocalPath(LevelContext& ctx, vector<INT2>& _path, const INT2& my_tile, const INT2& target_tile, const Unit* _me, const Unit* _other, const void* useable, bool is_end_point)
{
	assert(_me);

	_path.clear();
#ifdef DRAW_LOCAL_PATH
	if(marked == _me)
		test_pf.clear();
#endif

	if(my_tile == target_tile)
		return 3;

	int dist = distance(my_tile, target_tile);

	if(dist >= 32)
		return 1;

	if(dist <= 0 || my_tile.x < 0 || my_tile.y < 0)
	{
		ERROR(Format("Invalid FindLocalPath, ctx type %d, ctx building %d, my tile %d %d, target tile %d %d, me %s (%p; %g %g %g; %d), useable %p, is end point %d.",
			ctx.type, ctx.building_id, my_tile.x, my_tile.y, target_tile.x, target_tile.y, _me->data->id.c_str(), _me, _me->pos.x, _me->pos.y, _me->pos.z, _me->in_building,
			useable, is_end_point ? 1 : 0));
		if(_other)
		{
			ERROR(Format("Other unit %s (%p; %g, %g, %g, %d).", _other->data->id.c_str(), _other, _other->pos.x, _other->pos.y, _other->pos.z, _other->in_building));
		}
	}
	
	// œrodek
	const INT2 center_tile((my_tile+target_tile)/2);

	int minx = max(ctx.mine.x*8, center_tile.x-15),
		miny = max(ctx.mine.y*8, center_tile.y-15),
		maxx = min(ctx.maxe.x*8-1, center_tile.x+16),
		maxy = min(ctx.maxe.y*8-1, center_tile.y+16);

	int w = maxx-minx,
		h = maxy-miny;

	uint size = (w+1)*(h+1);

	if(a_map.size() < size)
		a_map.resize(size);
	memset(&a_map[0], 0, sizeof(APoint)*size);
	if(local_pfmap.size() < size)
		local_pfmap.resize(size);

	const Unit* ignored_units[3] = {_me, 0};
	if(_other)
		ignored_units[1] = _other;
	IgnoreObjects ignore = {0};
	ignore.ignored_units = (const Unit**)ignored_units;
	const void* ignored_objects[2] = {0};
	if(useable)
	{
		ignored_objects[0] = useable;
		ignore.ignored_objects = ignored_objects;
	}

	global_col.clear();
	GatherCollisionObjects(ctx, global_col, BOX2D(float(minx)/4-0.25f, float(miny)/4-0.25f, float(maxx)/4+0.25f, float(maxy)/4+0.25f), &ignore);

	const float r = _me->GetUnitRadius()-0.25f/2;

	for(int y=miny, y2 = 0; y<maxy; ++y, ++y2)
	{
		for(int x=minx, x2 = 0; x<maxx; ++x, ++x2)
		{
			if(!Collide(global_col, BOX2D(0.25f*x, 0.25f*y, 0.25f*(x+1), 0.25f*(y+1)), r))
			{
#ifdef DRAW_LOCAL_PATH
				if(marked == _me)
					test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*x, 0.25f*y), 0));
#endif
				local_pfmap[x2+y2*w] = false;
			}
			else
				local_pfmap[x2+y2*w] = true;
		}
	}

	const INT2 target_rel(target_tile.x - minx, target_tile.y - miny),
		       my_rel(my_tile.x - minx, my_tile.y - miny);

	// target tile is blocked
	if(!is_end_point && local_pfmap[target_rel(w)])
		return 4;

	// oznacz moje pole jako wolne
	local_pfmap[my_rel(w)] = false;
	const INT2 neis[8] = {
		INT2(-1,-1),
		INT2(0,-1),
		INT2(1,-1),
		INT2(-1,0),
		INT2(1,0),
		INT2(-1,1),
		INT2(0,1),
		INT2(1,1)
	};

	bool jest = false;
	for(int i=0; i<8; ++i)
	{
		INT2 pt = my_rel + neis[i];
		if(pt.x < 0 || pt.y < 0 || pt.x > 32 || pt.y > 32)
			continue;
		if(!local_pfmap[pt(w)])
		{
			jest = true;
			break;
		}
	}

	if(!jest)
		return 2;

	// dodaj pierwszy punkt do sprawdzenia
	static vector<Point> do_sprawdzenia;
	do_sprawdzenia.clear();
	{
		Point& pt = Add1(do_sprawdzenia);
		pt.pt = target_rel;
		pt.prev = INT2(0,0);
	}

	APoint apt, prev_apt;
	apt.odleglosc = apt.suma = distance(my_rel, target_rel)*10;
	apt.koszt = 0;
	apt.stan = 1;
	apt.prev = INT2(0,0);
	a_map[target_rel(w)] = apt;

	AStarSort sortownik(a_map, w);
	Point new_pt;

	const int MAX_STEPS = 100;
	int steps = 0;

	// rozpocznij szukanie œcie¿ki
	while(!do_sprawdzenia.empty())
	{
		Point pt = do_sprawdzenia.back();
		do_sprawdzenia.pop_back();
		prev_apt = a_map[pt.pt(w)];

		if(pt.pt == my_rel)
		{
			APoint& ept = a_map[pt.pt(w)];
			ept.stan = 1;
			ept.prev = pt.prev;
			break;
		}

		const INT2 kierunek[4] = {
			INT2(0,1),
			INT2(1,0),
			INT2(0,-1),
			INT2(-1,0)
		};

		const INT2 kierunek2[4] = {
			INT2(1,1),
			INT2(1,-1),
			INT2(-1,1),
			INT2(-1,-1)
		};

		for(int i=0; i<4; ++i)
		{
			const INT2& pt1 = kierunek[i] + pt.pt;
			const INT2& pt2 = kierunek2[i] + pt.pt;

			if(pt1.x>=0 && pt1.y>=0 && pt1.x<w-1 && pt1.y<h-1 && !local_pfmap[pt1(w)])
			{
				apt.prev = pt.pt;
				apt.koszt = prev_apt.koszt + 10;
				apt.odleglosc = distance(pt1, my_rel)*10;
				apt.suma = apt.odleglosc + apt.koszt;

				if(a_map[pt1(w)].IsLower(apt.suma))
				{
					apt.stan = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}

			if(pt2.x>=0 && pt2.y>=0 && pt2.x<w-1 && pt2.y<h-1 && !local_pfmap[pt2(w)] /*&&
				!local_pfmap[kierunek2[i].x+pt.pt.x+pt.pt.y*w] && !local_pfmap[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]*/)
			{
				apt.prev = pt.pt;
				apt.koszt = prev_apt.koszt + 15;
				apt.odleglosc = distance(pt2, my_rel)*10;
				apt.suma = apt.odleglosc + apt.koszt;

				if(a_map[pt2(w)].IsLower(apt.suma))
				{
					apt.stan = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}
		}

		++steps;
		if(steps > MAX_STEPS)
			break;

		std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
	}

	// set debug path drawing
#ifdef DRAW_LOCAL_PATH
	if(marked == _me)
		test_pf_outside = (location->outside && _me->in_building == -1);
#endif

	if(a_map[my_rel(w)].stan == 0)
	{
		// path not found
#ifdef DRAW_LOCAL_PATH
		if(marked == _me)
		{
			test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
			test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
		}
#endif
		return 5;
	}

#ifdef DRAW_LOCAL_PATH
	if(marked == _me)
	{
		test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
		test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
	}
#endif

	// populate path vector (and debug drawing)
	INT2 p = my_rel;

	do 
	{
#ifdef DRAW_LOCAL_PATH
		if(marked == _me)
			test_pf.push_back(std::pair<VEC2, int>(VEC2(0.25f*(p.x+minx), 0.25f*(p.y+miny)), 1));
#endif
		_path.push_back(INT2(p.x+minx, p.y+miny));
		p = a_map[p(w)].prev;
	}
	while (p != target_rel);

	_path.push_back(target_tile);
	std::reverse(_path.begin(), _path.end());
	_path.pop_back();

	return 0;
}

//=================================================================================================
void Game::SaveCfg()
{
	if(cfg.Save(cfg_file.c_str()) == Config::CANT_SAVE)
		ERROR(Format("Failed to save configuration file '%s'!", cfg_file.c_str()));
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
	tMinimap = nullptr;
	tChar = nullptr;
	for(int i=0; i<3; ++i)
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
	obj_arrow = nullptr;
	obj_spell = nullptr;

	// vertex declarations
	for(int i=0; i<VDI_MAX; ++i)
		vertex_decl[i] = nullptr;
}

//=================================================================================================
void Game::OnCleanup()
{
	if(!clearup_shutdown)
		ClearGame();

	RemoveGui();
	GUI.OnClean();
	CleanScene();
	DeleteElements(bow_instances);
	ClearQuadtree();
	CleanupDialogs();
	ClearLanguages();

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
	SafeRelease(tMinimap);
	SafeRelease(tChar);
	SafeRelease(tSave);
	SafeRelease(sItemRegion);
	SafeRelease(sChar);
	SafeRelease(sSave);
	for(int i=0; i<3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}

	// item textures
	for(auto& it : item_texture_map)
		SafeRelease(it.second);

	CleanupUnits();
	CleanupItems();
	CleanupSpells();
	DeleteElements(musics);

	// vertex data
	delete vdSchodyGora;
	delete vdSchodyDol;
	delete vdNaDrzwi;

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
	delete shape_schody;
	if(obj_arrow)
		delete obj_arrow->getCollisionShape();
	delete obj_arrow;
	delete obj_spell;

	// kszta³ty obiektów
	for(uint i=0; i<n_objs; ++i)
	{
		delete g_objs[i].shape;
		if(IS_SET(g_objs[i].flags, OBJ_DOUBLE_PHYSICS) && g_objs[i].next_obj)
		{
			delete g_objs[i].next_obj->shape;
			delete g_objs[i].next_obj;
		}
		else if(IS_SET(g_objs[i].flags2, OBJ2_MULTI_PHYSICS) && g_objs[i].next_obj)
		{
			for(int j=0;;++j)
			{
				bool have_next = (g_objs[i].next_obj[j].shape != nullptr);
				delete g_objs[i].next_obj[j].shape;
				if(!have_next)
					break;
			}
			delete[] g_objs[i].next_obj;
		}
	}

	draw_batch.Clear();
	free_cave_data();

	if(peer)
		RakNet::RakPeerInterface::DestroyInstance(peer);
}

//=================================================================================================
void Game::CreateTextures()
{
	V( device->CreateTexture(64, 64, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tItemRegion, nullptr) );
	V( device->CreateTexture(128, 128, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tMinimap, nullptr) );
	V( device->CreateTexture(128, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tChar, nullptr) );
	V( device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tSave, nullptr) );

	int ms, msq;
	GetMultisampling(ms, msq);
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)ms;
	if(ms != D3DMULTISAMPLE_NONE)
	{
		V( device->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, type, msq, FALSE, &sItemRegion, nullptr) );
		V( device->CreateRenderTarget(128, 256, D3DFMT_A8R8G8B8, type, msq, FALSE, &sChar, nullptr) );
		V( device->CreateRenderTarget(256, 256, D3DFMT_X8R8G8B8, type, msq, FALSE, &sSave, nullptr) );
		for(int i=0; i<3; ++i)
		{
			V( device->CreateRenderTarget(wnd_size.x, wnd_size.y, D3DFMT_X8R8G8B8, type, msq, FALSE, &sPostEffect[i], nullptr) );
			tPostEffect[i] = nullptr;
		}
		V( device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tPostEffect[0], nullptr) );
	}
	else
	{
		for(int i=0; i<3; ++i)
		{
			V( device->CreateTexture(wnd_size.x, wnd_size.y, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tPostEffect[i], nullptr) );
			sPostEffect[i] = nullptr;
		}
	}

	// fullscreen vertexbuffer
	VTex* v;
	V( device->CreateVertexBuffer(sizeof(VTex)*6, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vbFullscreen, nullptr) );
	V( vbFullscreen->Lock(0, sizeof(VTex)*6, (void**)&v, 0) );

	// coœ mi siê obi³o o uszy z tym pó³ teksela przy renderowaniu
	// ale szczegó³ów nie znam
	const float u_start = 0.5f / wnd_size.x;
	const float u_end = 1.f + 0.5f / wnd_size.x;
	const float v_start = 0.5f / wnd_size.y;
	const float v_end = 1.f + 0.5f / wnd_size.y;

	v[0] = VTex(-1.f,1.f,0.f,u_start,v_start);
	v[1] = VTex(1.f,1.f,0.f,u_end,v_start);
	v[2] = VTex(1.f,-1.f,0.f,u_end,v_end);
	v[3] = VTex(1.f,-1.f,0.f,u_end,v_end);
	v[4] = VTex(-1.f,-1.f,0.f,u_start,v_end);
	v[5] = VTex(-1.f,1.f,0.f,u_start,v_start);

	V( vbFullscreen->Unlock() );
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
	GKey[GK_TAKE_ALL].id = "keyTakeAll";
	GKey[GK_SELECT_DIALOG].id = "keySelectDialog";
	GKey[GK_SKIP_DIALOG].id = "keySkipDialog";
	GKey[GK_TALK_BOX].id = "keyTalkBox";
	GKey[GK_PAUSE].id = "keyPause";
	GKey[GK_YELL].id = "keyYell";
	GKey[GK_CONSOLE].id = "keyConsole";
	GKey[GK_ROTATE_CAMERA].id = "keyRotateCamera";
	GKey[GK_AUTOWALK].id = "keyAutowalk";

	for(int i=0; i<GK_MAX; ++i)
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
	GKey[GK_TAKE_ALL].Set('F');
	GKey[GK_SELECT_DIALOG].Set(VK_RETURN);
	GKey[GK_SKIP_DIALOG].Set(VK_SPACE);
	GKey[GK_TALK_BOX].Set('N');
	GKey[GK_PAUSE].Set(VK_PAUSE);
	GKey[GK_YELL].Set('Y');
	GKey[GK_CONSOLE].Set(VK_OEM_3);
	GKey[GK_ROTATE_CAMERA].Set('V');
	GKey[GK_AUTOWALK].Set('F');
}

//=================================================================================================
void Game::SaveGameKeys()
{
	for(int i=0; i<GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j=0; j<2; ++j)
			cfg.Add(Format("%s%d", k.id, j), Format("%d", k[j]));
	}

	SaveCfg();
}

//=================================================================================================
void Game::LoadGameKeys()
{
	for(int i=0; i<GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j=0; j<2; ++j)
		{
			cstring s = Format("%s%d", k.id, j);
			int w = cfg.GetInt(s);
			if(w == VK_ESCAPE || w < -1 || w > 255)
			{
				WARN(Format("Config: Invalid value for %s: %d.", s, w));
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

	STARTUPINFO si = {0};
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi = {0};

	// drugi parametr tak na prawdê nie jest modyfikowany o ile nie u¿ywa siê unicode (tak jest napisane w doku)
	// z ka¿dym restartem dodaje prze³¹cznik, mam nadzieje ¿e nikt nie bêdzie restartowa³ 100 razy pod rz¹d bo mo¿e skoñczyæ siê miejsce w cmdline albo co
	CreateProcess(nullptr, (char*)Format("%s -restart", GetCommandLine()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);

	Quit();
}

//=================================================================================================
void Game::SetStatsText()
{
	// typ broni
	weapon_type_info[WT_SHORT].name = Str("wt_shortBlade");
	weapon_type_info[WT_LONG].name = Str("wt_longBlade");
	weapon_type_info[WT_MACE].name = Str("wt_blunt");
	weapon_type_info[WT_AXE].name = Str("wt_axe");
}

//=================================================================================================
void Game::SetGameText()
{
#define LOAD_ARRAY(var, str) for(int i=0; i<countof(var); ++i) var[i] = Str(Format(str "%d", i))

	txGoldPlus = Str("goldPlus");
	txQuestCompletedGold = Str("questCompletedGold");

	// ai
	LOAD_ARRAY(txAiNoHpPot, "aiNoHpPot");
	LOAD_ARRAY(txAiJoinTour, "aiJoinTour");
	LOAD_ARRAY(txAiCity, "aiCity");
	LOAD_ARRAY(txAiVillage, "aiVillage");
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
	LOAD_ARRAY(txAiInsaneText, "aiInsaneText");
	LOAD_ARRAY(txAiDefaultText, "aiDefaultText");
	LOAD_ARRAY(txAiOutsideText, "aiOutsideText");
	LOAD_ARRAY(txAiInsideText, "aiInsideText");
	LOAD_ARRAY(txAiHumanText, "aiHumanText");
	LOAD_ARRAY(txAiOrcText, "aiOrcText");
	LOAD_ARRAY(txAiGoblinText, "aiGoblinText");
	LOAD_ARRAY(txAiMageText, "aiMageText");
	LOAD_ARRAY(txAiSecretText, "aiSecretText");
	LOAD_ARRAY(txAiHeroDungeonText, "aiHeroDungeonText");
	LOAD_ARRAY(txAiHeroCityText, "aiHeroCityText");
	LOAD_ARRAY(txAiBanditText, "aiBanditText");
	LOAD_ARRAY(txAiHeroOutsideText, "aiHeroOutsideText");
	LOAD_ARRAY(txAiDrunkMageText, "aiDrunkMageText");
	LOAD_ARRAY(txAiDrunkText, "aiDrunkText");
	LOAD_ARRAY(txAiDrunkmanText, "aiDrunkmanText");

	// nazwy lokacji
	txRandomEncounter = Str("randomEncounter");
	txCamp = Str("camp");

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
	txGeneratingTerrain = Str("generatingTerrain");

	// zawody w piciu
	txContestNoWinner = Str("contestNoWinner");
	txContestStart = Str("contestStart");
	LOAD_ARRAY(txContestTalk, "contestTalk");
	txContestWin = Str("contestWin");
	txContestWinNews = Str("contestWinNews");
	txContestDraw = Str("contestDraw");
	txContestPrize = Str("contestPrize");
	txContestNoPeople = Str("contestNoPeople");

	// samouczek
	LOAD_ARRAY(txTut, "tut");
	txTutNote = Str("tutNote");
	txTutLoc = Str("tutLoc");
	txTutPlay = Str("tutPlay");
	txTutTick = Str("tutTick");

	// zawody
	LOAD_ARRAY(txTour, "tour");

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
	txCantLoadGame = Str("cantLoadGame");
	txLoadSignature = Str("loadSignature");
	txLoadVersion = Str("loadVersion");
	txLoadSaveVersionNew = Str("loadSaveVersionNew");
	txLoadSaveVersionOld = Str("loadSaveVersionOld");
	txLoadMP = Str("loadMP");
	txLoadSP = Str("loadSP");
	txLoadError = Str("loadError");
	txLoadOpenError = Str("loadOpenError");

	txPvpRefuse = Str("pvpRefuse");
	txSsFailed = Str("ssFailed");
	txSsDone = Str("ssDone");
	txWin = Str("win");
	txWinMp = Str("winMp");
	txINeedWeapon = Str("iNeedWeapon");
	txNoHpp = Str("noHpp");
	txCantDo = Str("cantDo");
	txDontLootFollower = Str("dontLootFollower");
	txDontLootArena = Str("dontLootArena");
	txUnlockedDoor = Str("unlockedDoor");
	txNeedKey = Str("needKey");
	txLevelUp = Str("levelUp");
	txLevelDown = Str("levelDown");
	txLocationText = Str("locationText");
	txLocationTextMap = Str("locationTextMap");
	txRegeneratingLevel = Str("regeneratingLevel");
	txGmsLooted = Str("gmsLooted");
	txGmsRumor = Str("gmsRumor");
	txGmsJournalUpdated = Str("gmsJournalUpdated");
	txGmsUsed = Str("gmsUsed");
	txGmsUnitBusy = Str("gmsUnitBusy");
	txGmsGatherTeam = Str("gmsGatherTeam");
	txGmsNotLeader = Str("gmsNotLeader");
	txGmsNotInCombat = Str("gmsNotInCombat");
	txGainTextAttrib = Str("gainTextAttrib");
	txGainTextSkill = Str("gainTextSkill");
	txNeedLadle = Str("needLadle");
	txNeedPickaxe = Str("needPickaxe");
	txNeedHammer = Str("needHammer");
	txNeedUnk = Str("needUnk");
	txReallyQuit = Str("reallyQuit");
	txSecretAppear = Str("secretAppear");
	txGmsAddedItem = Str("gmsAddedItem");
	txGmsAddedItems = Str("gmsAddedItems");

	// plotki
	LOAD_ARRAY(txRumor, "rumor_");
	LOAD_ARRAY(txRumorD, "rumor_d_");

	// dialogi 1
	LOAD_ARRAY(txMayorQFailed, "mayorQFailed");
	LOAD_ARRAY(txQuestAlreadyGiven, "questAlreadyGiven");
	LOAD_ARRAY(txMayorNoQ, "mayorNoQ");
	LOAD_ARRAY(txCaptainQFailed, "captainQFailed");
	LOAD_ARRAY(txCaptainNoQ, "captainNoQ");
	LOAD_ARRAY(txLocationDiscovered, "locationDiscovered");
	LOAD_ARRAY(txAllDiscovered, "allDiscovered");
	LOAD_ARRAY(txCampDiscovered, "campDiscovered");
	LOAD_ARRAY(txAllCampDiscovered, "allCampDiscovered");
	LOAD_ARRAY(txNoQRumors, "noQRumors");
	LOAD_ARRAY(txRumorQ, "rumorQ");
	txNeedMoreGold = Str("needMoreGold");
	txNoNearLoc = Str("noNearLoc");
	txNearLoc = Str("nearLoc");
	LOAD_ARRAY(txNearLocEmpty, "nearLocEmpty");
	txNearLocCleared = Str("nearLocCleared");
	LOAD_ARRAY(txNearLocEnemy, "nearLocEnemy");
	LOAD_ARRAY(txNoNews, "noNews");
	LOAD_ARRAY(txAllNews, "allNews");
	txPvpTooFar = Str("pvpTooFar");
	txPvp = Str("pvp");
	txPvpWith = Str("pvpWith");
	txNewsCampCleared = Str("newsCampCleared");
	txNewsLocCleared = Str("newsLocCleared");
	LOAD_ARRAY(txArenaText, "arenaText");
	LOAD_ARRAY(txArenaTextU, "arenaTextU");
	txAllNearLoc = Str("allNearLoc");

	// dystans / si³a
	txNear = Str("near");
	txFar = Str("far");
	txVeryFar = Str("veryFar");
	LOAD_ARRAY(txELvlVeryWeak, "eLvlVeryWeak");
	LOAD_ARRAY(txELvlWeak, "eLvlWeak");
	LOAD_ARRAY(txELvlAverage, "eLvlAverage");
	LOAD_ARRAY(txELvlQuiteStrong, "eLvlQuiteStrong");
	LOAD_ARRAY(txELvlStrong, "eLvlStrong");

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
	LOAD_ARRAY(txQuest, "quest");
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
	txCreateServerFailed = Str("createServerFailed");
	txInitConnectionFailed = Str("initConnectionFailed");
	txServer = Str("server");
	txPlayerKicked = Str("playerKicked");
	txYouAreLeader = Str("youAreLeader");
	txRolledNumber = Str("rolledNumber");
	txPcIsLeader = Str("pcIsLeader");
	txReceivedGold = Str("receivedGold");
	txYouDisconnected = Str("youDisconnected");
	txYouKicked = Str("youKicked");
	txPcWasKicked = Str("pcWasKicked");
	txPcLeftGame = Str("pcLeftGame");
	txGamePaused = Str("gamePaused");
	txGameResumed = Str("gameResumed");
	txDevmodeOn = Str("devmodeOn");
	txDevmodeOff = Str("devmodeOff");
	txPlayerLeft = Str("playerLeft");

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

	// nazwy u¿ywalnych obiektów
	for(int i=0; i<U_MAX; ++i)
	{
		BaseUsable& u = g_base_usables[i];
		u.name = Str(u.id);
	}
		
	// rodzaje wrogów
	for(int i=0; i<SG_MAX; ++i)
	{
		SpawnGroup& sg = g_spawn_groups[i];
		if(!sg.name)
			sg.name = Str(Format("sg_%s", sg.unit_group_id));
	}

	// dialogi
	LOAD_ARRAY(txYell, "yell");

	TakenPerk::LoadText();
}

//=================================================================================================
Unit* Game::FindPlayerTradingWithUnit(Unit& u)
{
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() && (*it)->player->IsTradingWith(&u))
			return *it;
	}
	return nullptr;
}

//=================================================================================================
bool Game::ValidateTarget(Unit& u, Unit* target)
{
	assert(target);

	LevelContext& ctx = GetContext(u);

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
		s.t_pos = s.pos + random(VEC3(-0.05f, -0.05f, -0.05f), VEC3(0.05f, 0.05f, 0.05f));
		s.t_color = clamp(s.color + random(VEC3(-0.1f, -0.1f, -0.1f), VEC3(0.1f, 0.1f, 0.1f)));
	}
}

//=================================================================================================
bool Game::IsDrunkman(Unit& u)
{
	if(IS_SET(u.data->flags, F_AI_DRUNKMAN))
		return true;
	else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE))
		return quest_mages2->mages_state < Quest_Mages2::State::MageCured;
	else if(IS_SET(u.data->flags3, F3_DRUNKMAN_AFTER_CONTEST))
		return contest_state == CONTEST_DONE;
	else
		return false;
}

//=================================================================================================
void Game::PlayUnitSound(Unit& u, SOUND snd, float range)
{
	if(&u == pc->unit)
		PlaySound2d(snd);
	else
		PlaySound3d(snd, u.GetHeadSoundPos(), range);
}

//=================================================================================================
void Game::UnitFall(Unit& u)
{
	ACTION prev_action = u.action;
	u.live_state = Unit::FALLING;

	if(IsLocal())
	{
		// przerwij akcjê
		BreakAction(u, true);

		// wstawanie
		u.raise_timer = random(5.f,7.f);

		// event
		if(u.event_handler)
			u.event_handler->HandleUnitEvent(UnitEventHandler::FALL, &u);

		// komunikat
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::FALL;
			c.unit = &u;
		}

		if(u.player == pc)
			before_player = BP_NONE;
	}
	else
	{
		// przerwij akcjê
		BreakAction(u, true);

		// komunikat
		if(&u == pc->unit)
		{
			u.raise_timer = random(5.f,7.f);
			before_player = BP_NONE;
		}
	}

	if(prev_action == A_ANIMATION)
	{
		u.action = A_NONE;
		u.current_animation = ANI_STAND;
	}
	u.animation = ANI_DIE;
	u.talking = false;
	u.ani->need_update = true;
}

//=================================================================================================
void Game::UnitDie(Unit& u, LevelContext* ctx, Unit* killer)
{
	ACTION prev_action = u.action;

	if(u.live_state == Unit::FALL)
	{
		// postaæ ju¿ le¿y na ziemi, dodaj krew
		CreateBlood(GetContext(u), u);
		u.live_state = Unit::DEAD;
	}
	else
		u.live_state = Unit::DYING;

	if(IsLocal())
	{
		// przerwij akcjê
		BreakAction(u, true);

		// dodaj z³oto do ekwipunku
		if(u.gold && !(u.IsPlayer() || u.IsFollower()))
		{
			u.AddItem(gold_item_ptr, (uint)u.gold);
			u.gold = 0;
		}

		// og³oœ œmieræ
		for(vector<Unit*>::iterator it = ctx->units->begin(), end = ctx->units->end(); it != end; ++it)
		{
			if((*it)->IsPlayer() || !(*it)->IsStanding() || !IsFriend(u, **it))
				continue;

			if(distance(u.pos, (*it)->pos) <= 20.f && CanSee(u, **it))
				(*it)->ai->morale -= 2.f;
		}

		// o¿ywianie / sprawdŸ czy lokacja oczyszczona
		if(u.IsTeamMember())
			u.raise_timer = random(5.f,7.f);
		else
			CheckIfLocationCleared();

		// event
		if(u.event_handler)
			u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);

		// muzyka bossa
		if(IS_SET(u.data->flags2, F2_BOSS))
		{
			if(RemoveElementTry(boss_levels, INT2(current_location, dungeon_level)))
				SetMusic();
		}

		// komunikat
		if(IsOnline())
		{
			NetChange& c2 = Add1(net_changes);
			c2.type = NetChange::DIE;
			c2.unit = &u;
		}

		// statystyki
		++total_kills;
		if(killer && killer->IsPlayer())
		{
			++killer->player->kills;
			if(IsOnline())
				killer->player->stat_flags |= STAT_KILLS;
		}
		if(u.IsPlayer())
		{
			++u.player->knocks;
			if(IsOnline())
				u.player->stat_flags |= STAT_KNOCKS;
			if(u.player == pc)
				before_player = BP_NONE;
		}
	}
	else
	{
		u.hp = 0.f;

		// przerwij akcjê
		BreakAction(u, true);

		// o¿ywianie
		if(&u == pc->unit)
		{
			u.raise_timer = random(5.f,7.f);
			before_player = BP_NONE;
		}

		// muzyka bossa
		if(IS_SET(u.data->flags2, F2_BOSS) && boss_level_mp)
		{
			boss_level_mp = false;
			SetMusic();
		}
	}

	if(prev_action == A_ANIMATION)
	{
		u.action = A_NONE;
		u.current_animation = ANI_STAND;
	}
	u.animation = ANI_DIE;
	u.talking = false;
	u.ani->need_update = true;

	// dŸwiêk
	if(sound_volume)
	{
		SOUND snd = u.data->sounds->sound[SOUND_DEATH];
		if(!snd)
			snd = u.data->sounds->sound[SOUND_PAIN];
		if(snd)
			PlayUnitSound(u, snd, 2.f);
	}

	// przenieœ fizyke
	btVector3 a_min, a_max;
	u.cobj->getWorldTransform().setOrigin(btVector3(1000,1000,1000));
	u.cobj->getCollisionShape()->getAabb(u.cobj->getWorldTransform(), a_min, a_max);
	phy_broadphase->setAabb(u.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);
}

//=================================================================================================
void Game::UnitTryStandup(Unit& u, float dt)
{
	if(u.in_arena != -1 || death_screen != 0)
		return;

	u.raise_timer -= dt;
	bool ok = false;

	if(u.live_state == Unit::DEAD)
	{
		if(u.IsTeamMember())
		{
			if(u.hp > 0.f && u.raise_timer > 0.1f)
				u.raise_timer = 0.1f;
			
			if(u.raise_timer <= 0.f)
			{
				u.RemovePoison();

				if(u.alcohol > u.hpmax)
				{
					// móg³by wstaæ ale jest zbyt pijany
					u.live_state = Unit::FALL;
					UpdateUnitPhysics(u, u.pos);
				}
				else
				{
					// sprawdŸ czy nie ma wrogów
					LevelContext& ctx = GetContext(u);
					ok = true;
					for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
					{
						if((*it)->IsStanding() && IsEnemy(u, **it) && distance(u.pos, (*it)->pos) <= 20.f && CanSee(u, **it))
						{
							ok = false;
							break;
						}
					}
				}

				if(!ok)
				{
					if(u.hp > 0.f)
						u.raise_timer = 0.1f;
					else
						u.raise_timer = random(1.f,2.f);
				}
			}
		}
	}
	else if(u.live_state == Unit::FALL)
	{
		if(u.raise_timer <= 0.f)
		{
			if(u.alcohol < u.hpmax)
				ok = true;
			else
				u.raise_timer = random(1.f,2.f);
		}
	}

	if(ok)
	{
		UnitStandup(u);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::STAND_UP;
			c.unit = &u;
		}
	}
}

//=================================================================================================
void Game::UnitStandup(Unit& u)
{
	u.HealPoison();
	u.live_state = Unit::ALIVE;
	Animesh::Animation* anim = u.ani->ani->GetAnimation("wstaje2");
	if(anim)
	{
		u.ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
		u.action = A_ANIMATION;
	}
	else
		u.action = A_NONE;
	u.used_item = nullptr;

	if(IsLocal() && u.IsAI())
	{
		if(u.ai->state != AIController::Idle)
		{
			u.ai->state = AIController::Idle;
			u.ai->change_ai_mode = true;
		}
		u.ai->alert_target = nullptr;
		u.ai->idle_action = AIController::Idle_None;
		u.ai->target = nullptr;
		u.ai->timer = random(2.f,5.f);
	}

	WarpUnit(u, u.pos);
}

//=================================================================================================
void Game::UpdatePostEffects(float dt)
{
	post_effects.clear();
	if(!cl_postfx || game_state != GS_LEVEL)
		return;

	// szarzenie
	if(pc->unit->IsAlive())
		grayout = max(grayout-dt, 0.f);
	else
		grayout = min(grayout+dt, 1.f);
	if(grayout > 0.f)
	{
		PostEffect& e = Add1(post_effects);
		e.tech = ePostFx->GetTechniqueByName("Monochrome");
		e.power = grayout;
	}

	// upicie
	float drunk = pc->unit->alcohol/pc->unit->hpmax;
	if(drunk > 0.1f)
	{
		PostEffect* e = nullptr, *e2;
		post_effects.resize(post_effects.size()+2);
		e = &*(post_effects.end()-2);
		e2 = &*(post_effects.end()-1);

		e->id = e2->id = 0;
		e->tech = ePostFx->GetTechniqueByName("BlurX");
		e2->tech = ePostFx->GetTechniqueByName("BlurY");
		// 0.1-0.5 - 1
		// 1 - 2
		float mod;
		if(drunk < 0.5f)
			mod = 1.f;
		else
			mod = 1.f+(drunk-0.5f)*2;
		e->skill = e2->skill = VEC4(1.f/wnd_size.x*mod, 1.f/wnd_size.y*mod, 0, 0);
		// 0.1-0
		// 1-1
		e->power = e2->power = (drunk-0.1f)/0.9f;
	}
}

//=================================================================================================
void Game::PlayerYell(Unit& u)
{
	UnitTalk(u, random_string(txYell));

	LevelContext& ctx = GetContext(u);
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(u2.IsAI() && u2.IsStanding() && !IsEnemy(u, u2) && !IsFriend(u, u2) && u2.busy == Unit::Busy_No && u2.frozen == 0 && !u2.useable && u2.ai->state == AIController::Idle &&
			!IS_SET(u2.data->flags, F_AI_STAY) &&
			(u2.ai->idle_action == AIController::Idle_None || u2.ai->idle_action == AIController::Idle_Animation || u2.ai->idle_action == AIController::Idle_Rot ||
			u2.ai->idle_action == AIController::Idle_Look))
		{
			u2.ai->idle_action = AIController::Idle_MoveAway;
			u2.ai->idle_data.unit = &u;
			u2.ai->timer = random(3.f,5.f);
		}
	}
}

//=================================================================================================
bool Game::CanBuySell(const Item* item)
{
	assert(item);
	if(!trader_buy[item->type])
		return false;
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
void Game::ResetCollisionPointers()
{
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		if(it->base && IS_SET(it->base->flags, OBJ_PHYSICS_PTR))
		{
			btCollisionObject* cobj = (btCollisionObject*)it->ptr;
			if(cobj->getUserPointer() != (void*)&*it)
				cobj->setUserPointer(&*it);
		}
	}
}

//=================================================================================================
void Game::InitSuperShader()
{
	V( D3DXCreateEffectPool(&sshader_pool) );

	FileReader f(Format("%s/shaders/super.fx", g_system_dir.c_str()));
	FILETIME file_time;
	GetFileTime(f.file, nullptr, nullptr, &file_time);
	if(CompareFileTime(&file_time, &sshader_edit_time) != 0)
	{
		f.ReadToString(sshader_code);
		GetFileTime(f.file, nullptr, nullptr, &sshader_edit_time);
	}

	GetSuperShader(0);

	SetupSuperShader();
}

//=================================================================================================
SuperShader* Game::GetSuperShader(uint id)
{
	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
	{
		if(it->id == id)
			return &*it;
	}

	return CompileSuperShader(id);
}

//=================================================================================================
SuperShader* Game::CompileSuperShader(uint id)
{
	D3DXMACRO macros[10] = {0};
	uint i = 0;

	if(IS_SET(id, 1<<SSS_ANIMATED))
	{
		macros[i].Name = "ANIMATED";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_HAVE_BINORMALS))
	{
		macros[i].Name = "HAVE_BINORMALS";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_FOG))
	{
		macros[i].Name = "FOG";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_SPECULAR))
	{
		macros[i].Name = "SPECULAR_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_NORMAL))
	{
		macros[i].Name = "NORMAL_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1<<SSS_POINT_LIGHT))
	{
		macros[i].Name = "POINT_LIGHT";
		macros[i].Definition = "1";
		++i;

		macros[i].Name = "LIGHTS";
		macros[i].Definition = (shader_version == 2 ? "2" : "3");
		++i;
	}
	else if(IS_SET(id, 1<<SSS_DIR_LIGHT))
	{
		macros[i].Name = "DIR_LIGHT";
		macros[i].Definition = "1";
		++i;
	}

	macros[i].Name = "VS_VERSION";
	macros[i].Definition = (shader_version == 3 ? "vs_3_0" : "vs_2_0");
	++i;

	macros[i].Name = "PS_VERSION";
	macros[i].Definition = (shader_version == 3 ? "ps_3_0" : "ps_2_0");
	++i;

	LOG(Format("Compiling super shader: %u", id));

	CompileShaderParams params = {"super.fx"};
	params.cache_name = Format("%d_super%u.fcx", shader_version, id);
	params.file_time = sshader_edit_time;
	params.input = &sshader_code;
	params.macros = macros;
	params.pool = sshader_pool;
	
	SuperShader& s = Add1(sshaders);
	s.e = CompileShader(params);
	s.id = id;

	return &s;
}

//=================================================================================================
void Game::SetupSuperShader()
{
	ID3DXEffect* e = sshaders[0].e;

	LOG("Setting up super shader parameters.");
	hSMatCombined = e->GetParameterByName(nullptr, "matCombined");
	hSMatWorld = e->GetParameterByName(nullptr, "matWorld");
	hSMatBones = e->GetParameterByName(nullptr, "matBones");
	hSTint = e->GetParameterByName(nullptr, "tint");
	hSAmbientColor = e->GetParameterByName(nullptr, "ambientColor");
	hSFogColor = e->GetParameterByName(nullptr, "fogColor");
	hSFogParams = e->GetParameterByName(nullptr, "fogParams");
	hSLightDir = e->GetParameterByName(nullptr, "lightDir");
	hSLightColor = e->GetParameterByName(nullptr, "lightColor");
	hSLights = e->GetParameterByName(nullptr, "lights");
	hSSpecularColor = e->GetParameterByName(nullptr, "specularColor");
	hSSpecularIntensity = e->GetParameterByName(nullptr, "specularIntensity");
	hSSpecularHardness = e->GetParameterByName(nullptr, "specularHardness");
	hSCameraPos = e->GetParameterByName(nullptr, "cameraPos");
	hSTexDiffuse = e->GetParameterByName(nullptr, "texDiffuse");
	hSTexNormal = e->GetParameterByName(nullptr, "texNormal");
	hSTexSpecular = e->GetParameterByName(nullptr, "texSpecular");
	assert(hSMatCombined && hSMatWorld && hSMatBones && hSTint && hSAmbientColor && hSFogColor && hSFogParams && hSLightDir && hSLightColor && hSLights && hSSpecularColor && hSSpecularIntensity
		&& hSSpecularHardness && hSCameraPos && hSTexDiffuse && hSTexNormal && hSTexSpecular);
}

//=================================================================================================
void Game::ReloadShaders()
{
	LOG("Reloading shaders...");

	ReleaseShaders();

	InitSuperShader();

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

	SafeRelease(sshader_pool);
	for(vector<SuperShader>::iterator it = sshaders.begin(), end = sshaders.end(); it != end; ++it)
		SafeRelease(it->e);
	sshaders.clear();
}

//=================================================================================================
void Game::SetMeshSpecular()
{
	for(Weapon* weapon : g_weapons)
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

	for(Shield* shield : g_shields)
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

	for(Armor* armor : g_armors)
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

	for(UnitData* ud_ptr : unit_datas)
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
	LOG("Test: Validating game data...");

	uint err = TestGameData(major);

	AttributeInfo::Validate(err);
	SkillInfo::Validate(err);
	ClassInfo::Validate(err);
	Item::Validate(err);
	PerkInfo::Validate(err);

	if(err == 0)
		LOG("Test: Validation succeeded.");
	else
		ERROR(Format("Test: Validation failed, %u errors found.", err));

	return err;
}

//=================================================================================================
AnimeshInstance* Game::GetBowInstance(Animesh* mesh)
{
	if(bow_instances.empty())
		return new AnimeshInstance(mesh);
	else
	{
		AnimeshInstance* instance = bow_instances.back();
		bow_instances.pop_back();
		instance->ani = mesh;
		return instance;
	}
}

//=================================================================================================
void Game::SetupTrap(TaskData& task_data)
{
	BaseTrap& trap = *(BaseTrap*)task_data.ptr;
	trap.mesh = (Animesh*)task_data.res->data;

	Animesh::Point* pt = trap.mesh->FindPoint("hitbox");
	assert(pt);
	if(pt->type == Animesh::Point::BOX)
	{
		trap.rw = pt->size.x;
		trap.h = pt->size.z;
	}
	else
		trap.h = trap.rw = pt->size.x;
}

//=================================================================================================
void Game::SetupObject(TaskData& task_data)
{
	Obj& o = *(Obj*)task_data.ptr;
	if(task_data.res)
		o.mesh = (Animesh*)task_data.res->data;

	if(IS_SET(o.flags, OBJ_BUILDING))
		return;

	Animesh::Point* point;
	if(!IS_SET(o.flags2, OBJ2_VARIANT))
		point = o.mesh->FindPoint("hit");
	else
		point = o.variant->entries[0].mesh->FindPoint("hit");

	if(!point || !point->IsBox())
	{
		o.shape = nullptr;
		o.matrix = nullptr;
		return;
	}

	assert(point->size.x >= 0 && point->size.y >= 0 && point->size.z >= 0);
	if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
	{
		btBoxShape* shape = new btBoxShape(ToVector3(point->size));
		o.shape = shape;
	}
	else
		o.shape = nullptr;
	o.matrix = &point->mat;
	o.size = ToVEC2(point->size);

	if(IS_SET(o.flags, OBJ_PHY_ROT))
		o.type = OBJ_HITBOX_ROT;

	if(IS_SET(o.flags2, OBJ2_MULTI_PHYSICS))
	{
		LocalVector2<Animesh::Point*> points;
		Animesh::Point* prev_point = point;

		while(true)
		{
			Animesh::Point* new_point = o.mesh->FindNextPoint("hit", prev_point);
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
		o.next_obj = new Obj[points.size()+1];
		for(uint i = 0, size = points.size(); i < size; ++i)
		{
			Obj& o2 = o.next_obj[i];
			o2.shape = new btBoxShape(ToVector3(points[i]->size));
			if(IS_SET(o.flags, OBJ_PHY_BLOCKS_CAM))
				o2.flags = OBJ_PHY_BLOCKS_CAM;
			o2.matrix = &points[i]->mat;
			o2.size = ToVEC2(points[i]->size);
			o2.type = o.type;
		}
		o.next_obj[points.size()].shape = nullptr;
	}
	else if(IS_SET(o.flags, OBJ_DOUBLE_PHYSICS))
	{
		Animesh::Point* point2 = o.mesh->FindNextPoint("hit", point);
		if(point2 && point2->IsBox())
		{
			assert(point2->size.x >= 0 && point2->size.y >= 0 && point2->size.z >= 0);
			o.next_obj = new Obj("", 0, 0, "", "");
			if(!IS_SET(o.flags, OBJ_NO_PHYSICS))
			{
				btBoxShape* shape = new btBoxShape(ToVector3(point2->size));
				o.next_obj->shape = shape;
				if(IS_SET(o.flags, OBJ_PHY_BLOCKS_CAM))
					o.next_obj->flags = OBJ_PHY_BLOCKS_CAM;
			}
			else
				o.next_obj->shape = nullptr;
			o.next_obj->matrix = &point2->mat;
			o.next_obj->size = ToVEC2(point2->size);
			o.next_obj->type = o.type;
		}
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

	int index = strchr_index(arg, '=');
	if(index == -1 || index == 0)
	{
		WARN(Format("Broken command line variable '%s'.", arg));
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
		WARN(Format("Missing config variable '%.*s'.", index, arg));
		return;
	}

	cstring value = arg + index + 1;
	if(!*value)
	{
		WARN(Format("Missing command line variable value '%s'.", arg));
		return;
	}

	switch(var->type)
	{
	case AnyVarType::Bool:
		{
			bool b;
			if(!TextHelper::ToBool(value, b))
			{
				WARN(Format("Value for config variable '%s' must be bool, found '%s'.", var->name, value));
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
				WARN(Format("Value for config variable '%s' must be bool, found '%s'.", v.name, entry->value.c_str()));
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
		return Format("%s (%s, %s)", action, controls->key_text[k[0]], controls->key_text[k[1]]);
	else if(first_key || second_key)
	{
		byte used = k[first_key ? 0 : 1];
		return Format("%s (%s)", action, controls->key_text[used]);
	}
	else
		return action;
}

void Game::SetToolsetState(bool started)
{
	if(!toolset)
	{
		toolset = new Toolset;
		toolset->Init(this);
	}

	if(started)
	{
		main_menu->visible = false;
		game_state = GS_TOOLSET;
		toolset->Start();
	}
	else
	{
		main_menu->visible = true;
		game_state = GS_MAIN_MENU;
	}
}
