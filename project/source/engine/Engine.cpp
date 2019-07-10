#include "Pch.h"
#include "EngineCore.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "StartupOptions.h"
#include "DebugDrawer.h"
#include "DirectX.h"
#include "Physics.h"
#include "Render.h"
#include "App.h"

//-----------------------------------------------------------------------------
const Int2 Engine::MIN_WINDOW_SIZE = Int2(800, 600);
const Int2 Engine::DEFAULT_WINDOW_SIZE = Int2(1024, 768);

//-----------------------------------------------------------------------------
Engine* Engine::engine;

//=================================================================================================
Engine::Engine() : app(nullptr), engine_shutdown(false), timer(false), hwnd(nullptr), cursor_visible(true), replace_cursor(false), locked_cursor(true),
active(false), activation_point(-1, -1), phy_world(nullptr)
{
	engine = this;
}

//=================================================================================================
// Adjust window size to take exact value
void Engine::AdjustWindowSize()
{
	if(!fullscreen)
	{
		Rect rect = Rect::Create(Int2(0, 0), wnd_size);
		AdjustWindowRect((RECT*)&rect, WS_OVERLAPPEDWINDOW, false);
		real_size = rect.Size();
	}
	else
		real_size = wnd_size;
}

//=================================================================================================
// Called after changing mode
void Engine::ChangeMode()
{
	AdjustWindowSize();

	if(!fullscreen)
	{
		// windowed
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE);

		render->Reset(true);

		SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
			real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	}
	else
	{
		// fullscreen
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUPWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE);

		render->Reset(true);

		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, real_size.x, real_size.y, SWP_NOMOVE | SWP_SHOWWINDOW);
	}

	// reset cursor
	replace_cursor = true;
	Key.UpdateMouseDif(Int2::Zero);
	unlock_point = real_size / 2;
}

//=================================================================================================
// Change display mode
bool Engine::ChangeMode(bool new_fullscreen)
{
	if(fullscreen == new_fullscreen)
		return false;

	Info(new_fullscreen ? "Engine: Changing mode to fullscreen." : "Engine: Changing mode to windowed.");

	fullscreen = new_fullscreen;
	ChangeMode();

	return true;
}

//=================================================================================================
// Change resolution and display mode
bool Engine::ChangeMode(const Int2& size, bool new_fullscreen, int hz)
{
	assert(size.x > 0 && size.y > 0 && hz >= 0);

	if(size == wnd_size && new_fullscreen == fullscreen && hz == render->GetRefreshRate())
		return false;

	if(!render->CheckDisplay(size, hz))
	{
		Error("Engine: Can't change display mode to %dx%d (%d Hz, %s).", size.x, size.y, hz, new_fullscreen ? "fullscreen" : "windowed");
		return false;
	}

	Info("Engine: Resolution changed to %dx%d (%d Hz, %s).", size.x, size.y, hz, new_fullscreen ? "fullscreen" : "windowed");

	bool size_changed = (wnd_size != size);

	fullscreen = new_fullscreen;
	wnd_size = size;
	render->SetRefreshRateInternal(hz);
	ChangeMode();

	if(size_changed)
		app->OnResize();

	return true;
}

//=================================================================================================
// Cleanup engine
void Engine::Cleanup()
{
	Info("Engine: Cleanup.");

	app->OnCleanup();

	ResourceManager::Get().Cleanup();

	render.reset();

	CustomCollisionWorld::Cleanup(phy_world);
}

//=================================================================================================
// Do pseudo update tick, used to render in update loop
void Engine::DoPseudotick(bool msg_only)
{
	MSG msg = { 0 };
	if(!timer.IsStarted())
		timer.Start();

	while(msg.message != WM_QUIT && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(msg_only)
		timer.Tick();
	else
		DoTick(false);
}

//=================================================================================================
// Common part for WindowLoop and DoPseudotick
void Engine::DoTick(bool update_game)
{
	const float dt = timer.Tick();
	assert(dt >= 0.f);

	// calculate fps
	frames++;
	frame_time += dt;
	if(frame_time >= 1.f)
	{
		fps = frames / frame_time;
		frames = 0;
		frame_time = 0.f;
	}

	// update activity state
	bool is_active = IsWindowActive();
	bool was_active = active;
	UpdateActivity(is_active);

	// handle cursor movement
	Int2 mouse_dif = Int2::Zero;
	if(active)
	{
		if(locked_cursor)
		{
			if(replace_cursor)
				replace_cursor = false;
			else if(was_active)
			{
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				mouse_dif = Int2(pt.x, pt.y) - real_size / 2;
			}
			PlaceCursor();
		}
	}
	else if(!locked_cursor && lock_on_focus)
		locked_cursor = true;
	Key.UpdateMouseDif(mouse_dif);

	// update keyboard shortcuts info
	Key.UpdateShortcuts();

	// update game
	if(update_game)
		app->OnTick(dt);
	if(engine_shutdown)
	{
		if(active && locked_cursor)
		{
			Rect rect;
			GetClientRect(hwnd, (RECT*)&rect);
			Int2 wh = rect.Size();
			POINT pt;
			pt.x = int(float(unlock_point.x)*wh.x / wnd_size.x);
			pt.y = int(float(unlock_point.y)*wh.y / wnd_size.y);
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
		}
		return;
	}
	Key.UpdateMouseWheel(0);

	render->Draw();
	Key.Update();
	sound_mgr->Update(dt);
}

//=================================================================================================
bool Engine::IsWindowActive()
{
	HWND foreground = GetForegroundWindow();
	if(foreground != hwnd)
		return false;
	return !IsIconic(hwnd);
}

//=================================================================================================
// Start closing engine
void Engine::EngineShutdown()
{
	if(!engine_shutdown)
	{
		engine_shutdown = true;
		Info("Engine: Started closing engine...");
	}
}

//=================================================================================================
// Show fatal error
void Engine::FatalError(cstring err)
{
	assert(err);
	ShowError(err, Logger::L_FATAL);
	EngineShutdown();
}

//=================================================================================================
// Handle windows events
long Engine::HandleEvent(HWND in_hwnd, uint msg, uint wParam, long lParam)
{
	switch(msg)
	{
	// window closed/destroyed
	case WM_CLOSE:
	case WM_DESTROY:
		engine_shutdown = true;
		return 0;

	// handle keyboard
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		Key.Process((byte)wParam, true);
		return 0;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key.Process((byte)wParam, false);
		return 0;

	// handle mouse
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		{
			byte key;
			int result;
			bool down = MsgToKey(msg, wParam, key, result);

			if((!locked_cursor || !active) && down && lock_on_focus)
			{
				ShowCursor(false);
				Rect rect;
				GetClientRect(hwnd, (RECT*)&rect);
				Int2 wh = rect.Size();
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				activation_point = Int2(pt.x * wnd_size.x / wh.x, pt.y * wnd_size.y / wh.y);
				PlaceCursor();

				if(active)
					locked_cursor = true;

				return result;
			}

			Key.Process(key, down);
			return result;
		}

	// handle double click
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		{
			byte key;
			int result = 0;
			MsgToKey(msg, wParam, key, result);
			Key.ProcessDoubleClick(key);
			return result;
		}

	// close alt+space menu
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	// handle text input
	case WM_CHAR:
	case WM_SYSCHAR:
		app->OnChar((char)wParam);
		return 0;

	// handle mouse wheel
	case WM_MOUSEWHEEL:
		Key.UpdateMouseWheel(Key.GetMouseWheel() + float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
		return 0;
	}

	// return default message
	return DefWindowProc(in_hwnd, msg, wParam, lParam);
}

//=================================================================================================
// Convert message to virtual key
bool Engine::MsgToKey(uint msg, uint wParam, byte& key, int& result)
{
	bool down = false;

	switch(msg)
	{
	default:
		assert(0);
		break;
	case WM_LBUTTONDOWN:
		down = true;
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		key = VK_LBUTTON;
		result = 0;
		break;
	case WM_RBUTTONDOWN:
		down = true;
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		key = VK_RBUTTON;
		result = 0;
		break;
	case WM_MBUTTONDOWN:
		down = true;
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		key = VK_MBUTTON;
		result = 0;
		break;
	case WM_XBUTTONDOWN:
		down = true;
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
		result = TRUE;
		break;
	}

	return down;
}

//=================================================================================================
// Create window
void Engine::InitWindow(StartupOptions& options)
{
	assert(options.title);

	// register window class
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		[](HWND hwnd, uint msg, WPARAM wParam, LPARAM lParam) -> LRESULT { return Engine::Get().HandleEvent(hwnd, msg, wParam, lParam); },
		0, 0, GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), "Icon"), LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
		nullptr, "Krystal", nullptr
	};
	if(!RegisterClassEx(&wc))
		throw Format("Failed to register window class (%d).", GetLastError());

	// create window
	AdjustWindowSize();
	hwnd = CreateWindowEx(0, "Krystal", options.title, fullscreen ? WS_POPUPWINDOW : WS_OVERLAPPEDWINDOW, 0, 0, real_size.x, real_size.y,
		nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if(!hwnd)
		throw Format("Failed to create window (%d).", GetLastError());

	// position window
	if(!fullscreen)
	{
		if(options.force_pos != Int2(-1, -1) || options.force_size != Int2(-1, -1))
		{
			// set window position from config file
			Rect rect;
			GetWindowRect(hwnd, (RECT*)&rect);
			if(options.force_pos.x != -1)
				rect.Left() = options.force_pos.x;
			if(options.force_pos.y != -1)
				rect.Top() = options.force_pos.y;
			Int2 size = real_size;
			if(options.force_size.x != -1)
				size.x = options.force_size.x;
			if(options.force_size.y != -1)
				size.y = options.force_size.y;
			SetWindowPos(hwnd, 0, rect.Left(), rect.Top(), size.x, size.y, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		else
		{
			// set window at center of screen
			MoveWindow(hwnd,
				(GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2,
				(GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
				real_size.x, real_size.y, false);
		}
	}

	// show window
	ShowWindow(hwnd, options.hidden_window ? SW_HIDE : SW_SHOWNORMAL);

	// reset cursor
	replace_cursor = true;
	Key.UpdateMouseDif(Int2::Zero);
	unlock_point = real_size / 2;

	Info("Engine: Window created.");
}

//=================================================================================================
// Place cursor on window center
void Engine::PlaceCursor()
{
	POINT p;
	p.x = real_size.x / 2;
	p.y = real_size.y / 2;
	ClientToScreen(hwnd, &p);
	SetCursorPos(p.x, p.y);
}

//=================================================================================================
// Change window title
void Engine::SetTitle(cstring title)
{
	assert(title);
	SetWindowTextA(hwnd, title);
}

//=================================================================================================
// Show/hide cursor
void Engine::ShowCursor(bool _show)
{
	if (IsCursorVisible() != _show)
	{
		::ShowCursor(_show);
		cursor_visible = _show;
	}
}

//=================================================================================================
// Show error
void Engine::ShowError(cstring msg, Logger::Level level)
{
	assert(msg);

	ShowWindow(hwnd, SW_HIDE);
	ShowCursor(true);
	Logger::global->Log(level, msg);
	Logger::global->Flush();
	MessageBox(nullptr, msg, nullptr, MB_OK | MB_ICONERROR | MB_APPLMODAL);
}

//=================================================================================================
// Initialize and start engine
bool Engine::Start(App* app, StartupOptions& options)
{
	// set parameters
	this->app = app;
	fullscreen = options.fullscreen;
	wnd_size = Int2::Max(options.size, MIN_WINDOW_SIZE);

	// initialize engine
	try
	{
		Init(options);
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize CaRpg engine!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// initialize game
	if(!app->OnInit())
	{
		Cleanup();
		return false;
	}

	// loop game
	try
	{
		if(locked_cursor && active)
			PlaceCursor();
		WindowLoop();
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Game error!\n%s", e));
		Cleanup();
		return false;
	}

	// cleanup
	Cleanup();
	return true;
}

//=================================================================================================
void Engine::Init(StartupOptions& options)
{
	InitWindow(options);
	render.reset(new Render);
	render->Init(options);
	sound_mgr.reset(new SoundManager);
	sound_mgr->Init(options);
	phy_world = CustomCollisionWorld::Init();
	ResourceManager::Get().Init(render->GetDevice(), sound_mgr.get());
}

//=================================================================================================
// Unlock cursor - show system cursor and allow to move outside of window
void Engine::UnlockCursor(bool _lock_on_focus)
{
	lock_on_focus = _lock_on_focus;
	if(!locked_cursor)
		return;
	locked_cursor = false;

	if(!IsCursorVisible())
	{
		Rect rect;
		GetClientRect(hwnd, (RECT*)&rect);
		Int2 wh = rect.Size();
		POINT pt;
		pt.x = int(float(unlock_point.x)*wh.x / wnd_size.x);
		pt.y = int(float(unlock_point.y)*wh.y / wnd_size.y);
		ClientToScreen(hwnd, &pt);
		SetCursorPos(pt.x, pt.y);
	}

	ShowCursor(true);
}

//=================================================================================================
// Lock cursor when window gets activated
void Engine::LockCursor()
{
	if(locked_cursor)
		return;
	lock_on_focus = true;
	locked_cursor = true;
	if(active)
	{
		ShowCursor(false);
		PlaceCursor();
	}
}

//=================================================================================================
// Update window activity
void Engine::UpdateActivity(bool is_active)
{
	if(is_active == active)
		return;
	active = is_active;
	if(active)
	{
		if (locked_cursor)
		{
			ShowCursor(false);
			PlaceCursor();
		}
	}
	else
	{
		ShowCursor(true);
		Key.ReleaseKeys();
	}
	app->OnFocus(active, activation_point);
	activation_point = Int2(-1, -1);
}

//=================================================================================================
// Main window loop
void Engine::WindowLoop()
{
	MSG msg = { 0 };

	// start timer
	timer.Start();
	frames = 0;
	frame_time = 0.f;
	fps = 0.f;

	while(msg.message != WM_QUIT)
	{
		// handle winapi messages
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
			DoTick(true);

		if(engine_shutdown)
			break;
	}
}

//=================================================================================================
void Engine::SetWindowSizeInternal(const Int2& size)
{
	wnd_size = size;
	AdjustWindowSize();
	SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2, (GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2,
		real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
}
