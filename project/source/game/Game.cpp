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
#include "Pathfinding.h"
#include "SaveSlot.h"
#include "PlayerInfo.h"
#include "Render.h"
#include "RenderTarget.h"
#include "LobbyApi.h"
#include "GameMessages.h"
#include "GrassShader.h"
#include "TerrainShader.h"

const float LIMIT_DT = 0.3f;
Game* Game::game;
GameKeys GKey;
extern string g_system_dir;
extern cstring RESTART_MUTEX_NAME;
extern const int ITEM_IMAGE_SIZE = 64;

const float HIT_SOUND_DIST = 1.5f;
const float ARROW_HIT_SOUND_DIST = 1.5f;
const float SHOOT_SOUND_DIST = 1.f;
const float SPAWN_SOUND_DIST = 1.5f;
const float MAGIC_SCROLL_SOUND_DIST = 1.5f;

//=================================================================================================
Game::Game() : have_console(false), vbParticle(nullptr), quickstart(QUICKSTART_NONE), inactive_update(false), last_screenshot(0), draw_particle_sphere(false),
draw_unit_radius(false), draw_hitbox(false), noai(false), testing(false), game_speed(1.f), devmode(false), force_seed(0), next_seed(0), force_seed_all(false),
debug_info(false), dont_wander(false), check_updates(true), skip_tutorial(false), portal_anim(0), debug_info2(false), music_type(MusicType::None),
end_of_game(false), prepared_stream(64 * 1024), paused(false), draw_flags(0xFFFFFFFF), prev_game_state(GS_LOAD), rt_save(nullptr), rt_item(nullptr),
rt_item_rot(nullptr), cl_postfx(true), mp_timeout(10.f), cl_normalmap(true), cl_specularmap(true), dungeon_tex_wrap(true), profiler_mode(0),
screenshot_format(ImageFormat::JPG), quickstart_class(Class::RANDOM), game_state(GS_LOAD), default_devmode(false), default_player_devmode(false),
quickstart_slot(SaveSlot::MAX_SLOTS)
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
	L.camera.draw_range = 80.f;

	SetupConfigVars();
}

//=================================================================================================
Game::~Game()
{
}

//=================================================================================================
void Game::OnDraw()
{
	if(profiler_mode == 2)
		Profiler::g_profiler.Start();
	else if(profiler_mode == 0)
		Profiler::g_profiler.Clear();

	DrawGame(nullptr);

	Profiler::g_profiler.End();
}

//=================================================================================================
void Game::DrawGame(RenderTarget* target)
{
	Render* render = GetRender();
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
			if(pc_data.before_player != BP_NONE && !draw_batch.glow_nodes.empty())
			{
				V(device->EndScene());
				DrawGlowingNodes(false);
				V(device->BeginScene());
			}

			// debug draw
			debug_drawer->Draw();
		}

		// draw gui
		GUI.mViewProj = L.camera.matViewProj;
		GUI.Draw(IS_SET(draw_flags, DF_GUI), IS_SET(draw_flags, DF_MENU));

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
			if(pc_data.before_player != BP_NONE && !draw_batch.glow_nodes.empty())
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
		V(device->SetVertexDeclaration(vertex_decl[VDI_TEX]));
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
			{
				GUI.mViewProj = L.camera.matViewProj;
				GUI.Draw(IS_SET(draw_flags, DF_GUI), IS_SET(draw_flags, DF_MENU));
			}

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
		dd->SetCamera(L.camera);
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
void Game::OnTick(float dt)
{
	// check for memory corruption
	assert(_CrtCheckMemory());

	if(dt > LIMIT_DT)
		dt = LIMIT_DT;

	if(profiler_mode == 1)
		Profiler::g_profiler.Start();
	else if(profiler_mode == 0)
		Profiler::g_profiler.Clear();

	N.api->Update();

	UpdateMusic();

	if(Net::IsSingleplayer() || !paused)
	{
		// update time spent in game
		if(game_state != GS_MAIN_MENU && game_state != GS_LOAD)
			GameStats::Get().Update(dt);
	}

	GKey.allow_input = GameKeys::ALLOW_INPUT;

	// lost directx device or window don't have focus
	if(GetRender()->IsLostDevice() || !IsActive() || !IsCursorLocked())
	{
		Key.SetFocus(false);
		if(Net::IsSingleplayer() && !inactive_update)
		{
			Profiler::g_profiler.End();
			return;
		}
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

	// fast quit (alt+f4)
	if(Key.Focus() && Key.Down(VK_MENU) && Key.Down(VK_F4) && !GUI.HaveTopDialog("dialog_alt_f4"))
		gui->ShowQuitDialog();

	if(end_of_game)
	{
		death_fade += dt;
		if(death_fade >= 1.f && GKey.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE))
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
		if(!GUI.HaveTopDialog("dialog_alt_f4") && !GUI.HaveDialog("console") && GKey.KeyDownUpAllowed(GK_CONSOLE))
			GUI.ShowDialog(gui->console);

		// unlock cursor
		if(!IsFullscreen() && IsActive() && IsCursorLocked() && Key.Shortcut(KEY_CONTROL, 'U'))
			UnlockCursor();

		// switch window mode
		if(Key.Shortcut(KEY_ALT, VK_RETURN))
			ChangeMode(!IsFullscreen());

		// screenshot
		if(Key.PressedRelease(VK_SNAPSHOT))
			TakeScreenshot(Key.Down(VK_SHIFT));

		// pause/resume game
		if(GKey.KeyPressedReleaseAllowed(GK_PAUSE) && !Net::IsClient())
			PauseGame();
	}

	// handle panels
	if(GUI.HaveDialog() || (gui->mp_box->visible && gui->mp_box->itb.focus))
		GKey.allow_input = GameKeys::ALLOW_NONE;
	else if(GKey.AllowKeyboard() && game_state == GS_LEVEL && death_screen == 0 && !dialog_context.dialog_mode)
	{
		OpenPanel open = gui->game_gui->GetOpenPanel(),
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

	// quicksave, quickload
	bool console_open = GUI.HaveTopDialog("console");
	bool special_key_allowed = (GKey.allow_input == GameKeys::ALLOW_KEYBOARD || GKey.allow_input == GameKeys::ALLOW_INPUT || (!GUI.HaveDialog() || console_open));
	if(GKey.KeyPressedReleaseSpecial(GK_QUICKSAVE, special_key_allowed))
		Quicksave(console_open);
	if(GKey.KeyPressedReleaseSpecial(GK_QUICKLOAD, special_key_allowed))
		Quickload(console_open);

	// mp box
	if(game_state == GS_LEVEL && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
		gui->mp_box->visible = !gui->mp_box->visible;

	// update gui
	gui->UpdateGui(dt);
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
	if(GKey.AllowKeyboard() && CanShowMenu() && Key.PressedRelease(VK_ESCAPE))
		gui->ShowMenu();

	arena->UpdatePvpRequest(dt);

	// update game
	if(!end_of_game)
	{
		if(game_state == GS_LEVEL)
		{
			if(paused)
			{
				UpdateFallback(dt);
				if(Net::IsLocal())
				{
					if(Net::IsOnline())
						UpdateWarpData(dt);
					L.ProcessUnitWarps();
				}
				SetupCamera(dt);
				if(Net::IsOnline())
					UpdateGameNet(dt);
			}
			else if(GUI.HavePauseDialog())
			{
				if(Net::IsOnline())
					UpdateGame(dt);
				else
				{
					UpdateFallback(dt);
					if(Net::IsLocal())
					{
						if(Net::IsOnline())
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

	// open/close mp box
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
		loc_gen_factory->Get(L.location)->CreateMinimap();
	if(gui && gui->inventory)
		gui->inventory->OnReload();
}

//=================================================================================================
void Game::OnReset()
{
	if(gui && gui->inventory)
		gui->inventory->OnReset();

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

	SafeRelease(tMinimap);
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
void Game::OnChar(char c)
{
	if((c != VK_BACK && c != VK_RETURN && byte(c) < 0x20) || c == '`')
		return;

	GUI.OnChar(c);
}

//=================================================================================================
void Game::TakeScreenshot(bool no_gui)
{
	Render* render = GetRender();

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
		gui->console->AddMsg(msg);
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
		gui->console->AddMsg(msg);
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
	N.mp_load = false;
	N.was_client = false;

	SetMusic(MusicType::Title);
	end_of_game = false;

	gui->CloseAllPanels();
	string msg;
	DialogBox* box = GUI.GetDialog("fatal");
	bool console = gui->console->visible;
	if(box)
		msg = box->text;
	GUI.CloseDialogs();
	if(!msg.empty())
		GUI.SimpleDialog(msg.c_str(), nullptr, "fatal");
	if(console)
		GUI.ShowDialog(gui->console);
	gui->game_menu->visible = false;
	gui->game_gui->visible = false;
	gui->world_map->Hide();
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
	tMinimap = nullptr;
	for(int i = 0; i < 3; ++i)
	{
		sPostEffect[i] = nullptr;
		tPostEffect[i] = nullptr;
	}

	// vertex data
	vdSchodyGora = nullptr;
	vdSchodyDol = nullptr;
	vdNaDrzwi = nullptr;

	// vertex declarations
	for(int i = 0; i < VDI_MAX; ++i)
		vertex_decl[i] = nullptr;
}

//=================================================================================================
void Game::OnCleanup()
{
	if(game_state != GS_QUIT && game_state != GS_LOAD_MENU)
		ClearGame();

	content.CleanupContent();

	for(GameComponent* component : components)
		component->Cleanup();

	CleanScene();
	DeleteElements(bow_instances);
	ClearQuadtree();

	// shadery
	ReleaseShaders();

	// bufory wierzcho³ków i indeksy
	SafeRelease(vbParticle);
	SafeRelease(vbDungeon);
	SafeRelease(ibDungeon);
	SafeRelease(vbFullscreen);

	// tekstury render target, powierzchnie
	SafeRelease(tMinimap);
	for(int i = 0; i < 3; ++i)
	{
		SafeRelease(sPostEffect[i]);
		SafeRelease(tPostEffect[i]);
	}

	// item textures
	for(auto& it : item_texture_map)
		SafeRelease(it.second);

	draw_batch.Clear();
}

//=================================================================================================
void Game::CreateTextures()
{
	if(tMinimap)
		return;

	Render* render = GetRender();
	IDirect3DDevice9* device = render->GetDevice();
	const Int2& wnd_size = GetWindowSize();

	V(device->CreateTexture(128, 128, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tMinimap, nullptr));

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
	Render* render = GetRender();
	rt_save = render->CreateRenderTarget(Int2(256, 256));
	rt_item = render->CreateRenderTarget(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));
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
		e->skill = e2->skill = Vec4(1.f / GetWindowSize().x*mod, 1.f / GetWindowSize().y*mod, 0, 0);
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
		Render* render = GetRender();
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
		FatalError(Format("Failed to reload shaders.\n%s", err));
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

	for(ShaderHandler* shader : GetRender()->GetShaders())
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
	ClassInfo::Validate(err);
	Item::Validate(err);
	PerkInfo::Validate(err);
	RoomType::Validate(err);

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
	{
		if(!mesh->IsLoaded())
			ResourceManager::Get<Mesh>().Load(mesh);
		return new MeshInstance(mesh);
	}
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
	cfg.AddVar(ConfigVar("devmode", default_devmode));
	cfg.AddVar(ConfigVar("players_devmode", default_player_devmode));
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
	L.entering = true;

	gui->world_map->Hide();
	gui->game_gui->Reset();
	gui->game_gui->visible = true;

	const bool reenter = L.is_open;
	L.is_open = true;
	L.reenter = reenter;
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

	if(l.last_visit == -1)
		first = true;

	if(!reenter)
		InitQuadTree();

	if(Net::IsOnline() && N.active_players > 1)
	{
		BitStreamWriter f;
		f << ID_CHANGE_LEVEL;
		f << (byte)L.location_index;
		f << (byte)0;
		f << (W.GetState() == World::State::INSIDE_ENCOUNTER);
		int ack = N.SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
		for(auto info : N.players)
		{
			if(info->id == Team.my_id)
				info->state = PlayerInfo::IN_GAME;
			else
			{
				info->state = PlayerInfo::WAITING_FOR_RESPONSE;
				info->ack = ack;
				info->timer = 5.f;
			}
		}
		N.FilterServerChanges();
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
			InsideLocation* inside = static_cast<InsideLocation*>(L.location);
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
				multi->infos[L.dungeon_level].seed = l.seed;
			}
		}

		// nie odwiedzono, trzeba wygenerowaæ
		if(l.state != LS_HIDDEN)
			l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		loc_gen->Generate();
	}
	else if(!Any(l.type, L_DUNGEON, L_CRYPT, L_CAVE))
		Info("Entering location '%s'.", l.name.c_str());

	L.city_ctx = nullptr;

	if(L.location->outside)
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

	bool loaded_resources = L.location->RequireLoadingResources(nullptr);
	LoadResources(txLoadingComplete, false);

	l.last_visit = W.GetWorldtime();
	L.CheckIfLocationCleared();
	L.camera.Reset();
	pc_data.rot_buf = 0.f;
	SetMusic();

	if(close_portal)
	{
		delete L.location->portal;
		L.location->portal = nullptr;
	}

	if(L.location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	if(Net::IsOnline())
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		if(N.active_players > 1)
		{
			prepared_stream.Reset();
			N.WriteLevelData(prepared_stream, loaded_resources);
			Info("Generated location packet: %d.", prepared_stream.GetNumberOfBytesUsed());
		}
		else
			N.GetMe().state = PlayerInfo::IN_GAME;

		gui->info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = L.clear_color2;
		game_state = GS_LEVEL;
		gui->load_screen->visible = false;
		gui->main_menu->visible = false;
		gui->game_gui->visible = true;
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");
	L.entering = false;
}

// dru¿yna opuœci³a lokacje
void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(!L.is_open)
		return;

	if(Net::IsLocal() && !N.was_client)
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
		LeaveLevel(true);
		return;
	}

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
			OutsideLocation* outside = static_cast<OutsideLocation*>(L.location);
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

	if(Net::IsLocal() && end_buffs)
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

//=================================================================================================
void Game::ReportError(int id, cstring text)
{
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
	gui->messages->AddGameMsg(str, 5.f);
#endif
	N.api->Report(id, Format("[%s] %s", mode, text));
}
