#include "Pch.h"
#include "Engine.h"

//-----------------------------------------------------------------------------
extern const uint MIN_WIDTH = 800;
extern const uint MIN_HEIGHT = 600;
extern const uint DEFAULT_WIDTH = 1024;
extern const uint DEFAULT_HEIGHT = 768;

//-----------------------------------------------------------------------------
Engine* Engine::engine;
KeyStates Key;
extern string g_system_dir;

//=================================================================================================
// Konstruktur
//=================================================================================================
Engine::Engine() : engine_shutdown(false), timer(false), hwnd(nullptr), d3d(nullptr), device(nullptr), sprite(nullptr), fmod_system(nullptr),
phy_config(nullptr), phy_dispatcher(nullptr), phy_broadphase(nullptr), phy_world(nullptr), current_music(nullptr), replace_cursor(false), locked_cursor(true),
lost_device(false), clear_color(BLACK), mouse_wheel(0), s_wnd_pos(-1,-1), s_wnd_size(-1,-1), music_ended(false), disabled_sound(false), key_callback(nullptr),
res_freed(false), vsync(true), resMgr(ResourceManager::Get())
{
	engine = this;
}

//=================================================================================================
// Adjust window size to take exact value
//=================================================================================================
void Engine::AdjustWindowSize()
{
	if(!fullscreen)
	{
		RECT rect = {0};
		rect.right = wnd_size.x;
		rect.bottom = wnd_size.y;

		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

		real_size.x = abs(rect.left - rect.right);
		real_size.y = abs(rect.top - rect.bottom);
	}
	else
		real_size = wnd_size;
}

//=================================================================================================
// Zmienia tryb wyœwietlania gry
//=================================================================================================
void Engine::ChangeMode()
{
	AdjustWindowSize();

	if(!fullscreen)
	{
		// tryb okienkowy
		SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE);

		Reset(true);

		SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN)-real_size.x)/2, (GetSystemMetrics(SM_CYSCREEN)-real_size.y)/2,
			real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	}
	else
	{
		// tryb pe³noekranowy
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUPWINDOW);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE);

		Reset(true);

		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, real_size.x, real_size.y, SWP_NOMOVE | SWP_SHOWWINDOW);
	}

	// ustaw myszkê na œrodku
	if(active)
		PlaceCursor();
	else
		replace_cursor = true;
	mouse_dif = INT2(0,0);
}

//=================================================================================================
// Zmienia tryb wyœwietlania gry
//=================================================================================================
bool Engine::ChangeMode(bool _fullscreen)
{
	if(fullscreen == _fullscreen)
		return false;

	LOG(_fullscreen ? "Engine: Changing mode to fullscreen." : "Engine: Changing mode to windowed.");

	fullscreen = _fullscreen;
	ChangeMode();

	return true;
}

//=================================================================================================
// Zmienia rozdzielczoœæ gry i trybu
//=================================================================================================
bool Engine::ChangeMode(int w, int h, bool _fullscreen, int hz)
{
	assert(w > 0 && h > 0 && hz >= 0);

	if(!CheckDisplay(w, h, hz))
	{
		ERROR(Format("Engine: Can't change display mode to %dx%d (%d Hz, %s).", w, h, hz, _fullscreen ? "fullscreen" : "windowed"));
		return false;
	}

	if(wnd_size.x == w && wnd_size.y == h && fullscreen == _fullscreen && wnd_hz == hz)
		return false;

	LOG(Format("Engine: Resolution changed to %dx%d (%d Hz, %s).", w, h, hz, _fullscreen ? "fullscreen" : "windowed"));

	bool size_changed = (wnd_size.x != w || wnd_size.y != h);

	fullscreen = _fullscreen;
	wnd_size.x = w;
	wnd_size.y = h;
	wnd_hz = hz;
	ChangeMode();

	if(size_changed)
		OnResize();

	return true;
}

//=================================================================================================
// Zmienia tryb multisamplingu
//=================================================================================================
int Engine::ChangeMultisampling(int type, int level)
{
	if(type == multisampling && (level == -1 || level == multisampling_quality))
		return 1;

	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels)) &&
		SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels2)))
	{
		levels = min(levels, levels2);
		if(level < 0)
			level = levels-1;
		else if(level >= (int)levels)
			return 0;

		multisampling = type;
		multisampling_quality = level;

		Reset(true);

		return 2;
	}
	else
		return 0;
}

//=================================================================================================
bool Engine::CheckDisplay(int w, int h, int& hz)
{
	assert(w >= MIN_WIDTH && h >= MIN_HEIGHT);

	// check minimum resolution
	if(w < MIN_WIDTH || h < MIN_HEIGHT)
		return false;

	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);

	if(hz == 0)
	{
		bool valid = false;

		for(uint i=0; i<display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V( d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode) );
			if(w == d_mode.Width && h == d_mode.Height)
			{
				valid = true;
				if(hz < (int)d_mode.RefreshRate)
					hz = d_mode.RefreshRate;
			}
		}

		return valid;
	}
	else
	{
		for(uint i=0; i<display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V( d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode) );
			if(w == d_mode.Width && h == d_mode.Height && hz == d_mode.RefreshRate)
				return true;
		}

		return false;
	}
}

//=================================================================================================
// Sprz¹tanie silnika
//=================================================================================================
void Engine::Cleanup()
{
	LOG("Engine: Cleanup.");

	OnCleanup();

	resMgr.Cleanup();

	// directx
	if(device)
	{
		device->SetStreamSource(0, nullptr, 0, 0);
		device->SetIndices(nullptr);
	}
	SafeRelease(sprite);
	SafeRelease(device);
	SafeRelease(d3d);

	// fizyka
	delete phy_world;
	delete phy_broadphase;
	delete phy_dispatcher;
	delete phy_config;

	// FMOD
	if(fmod_system)
		fmod_system->release();
}

//=================================================================================================
ID3DXEffect* Engine::CompileShader(cstring name)
{
	assert(name);

	CompileShaderParams params = {name};

	// add c to extension
	LocalString str = (shader_version == 3 ? "3_" : "2_");
	str += name;
	str += 'c';
	params.cache_name = str;

	// set shader version
	D3DXMACRO macros[3] = {
		"VS_VERSION", shader_version == 3 ? "vs_3_0" : "vs_2_0",
		"PS_VERSION", shader_version == 3 ? "ps_3_0" : "ps_2_0",
		nullptr, nullptr
	};
	params.macros = macros;

	return CompileShader(params);
}

//=================================================================================================
ID3DXEffect* Engine::CompileShader(CompileShaderParams& params)
{
	assert(params.name && params.cache_name);

	ID3DXBuffer* errors = nullptr;
	ID3DXEffectCompiler* compiler = nullptr;
	cstring filename = Format("%s/shaders/%s", g_system_dir.c_str(), params.name);
	cstring cache_path = Format("cache/%s", params.cache_name);
	HRESULT hr;

	const DWORD flags =
#ifdef _DEBUG
		D3DXSHADER_DEBUG | D3DXSHADER_OPTIMIZATION_LEVEL1;
#else
		D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

	// open file and get date if not from string
	FileReader file;
	if(!params.input)
	{
		if(!file.Open(filename))
			throw Format("Engine: Failed to load shader '%s'. (%d)", params.name, GetLastError());
		GetFileTime(file.file, nullptr, nullptr, &params.file_time);
	}

	// check if in cache
	{
		FileReader cache_file(cache_path);
		if(cache_file)
		{
			FILETIME cache_time;
			GetFileTime(cache_file.file, nullptr, nullptr, &cache_time);
			if(CompareFileTime(&params.file_time, &cache_time) == 0)
			{
				// same last modify time, use cache
				cache_file.ReadToString(g_tmp_string);
				ID3DXEffect* effect = nullptr;
				hr = D3DXCreateEffect(device, g_tmp_string.c_str(), g_tmp_string.size(), params.macros, nullptr, flags, params.pool, &effect, &errors);
				if(FAILED(hr))
				{
					ERROR(Format("Engine: Failed to create effect from cache '%s' (%d).\n%s (%d)", params.cache_name, hr,
						errors ? (cstring)errors->GetBufferPointer() : "No errors information."));
					SafeRelease(errors);
					SafeRelease(effect);
				}
				else
				{
					SafeRelease(errors);
					return effect;
				}
			}
		}
	}

	// load from file
	if(!params.input)
	{
		file.ReadToString(g_tmp_string);
		params.input = &g_tmp_string;
	}
	hr = D3DXCreateEffectCompiler(params.input->c_str(), params.input->size(), params.macros, nullptr, flags, &compiler, &errors);
	if(FAILED(hr))
	{
		cstring str;
		if(errors)
			str = (cstring)errors->GetBufferPointer();
		else
		{
			switch(hr)
			{
			case D3DXERR_INVALIDDATA:
				str = "Invalid data.";
				break;
			case D3DERR_INVALIDCALL:
				str = "Invalid call.";
				break;
			case E_OUTOFMEMORY:
				str = "Out of memory.";
				break;
			case ERROR_MOD_NOT_FOUND:
			case 0x8007007e:
				str = "Can't find module (missing d3dcompiler_43.dll?).";
				break;
			default:
				str = "Unknown error.";
				break;
			}
		}

		cstring msg = Format("Engine: Failed to compile shader '%s'. (%d)\n%s", params.name, hr, str);

		SafeRelease(errors);

		throw msg;
	}
	SafeRelease(errors);

	// compile shader
	ID3DXBuffer* effect_buffer = nullptr;
	hr = compiler->CompileEffect(flags, &effect_buffer, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Engine: Failed to compile effect '%s' (%d).\n%s (%d)", params.name, hr,
			errors ?(cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		throw msg;
	}
	SafeRelease(errors);

	// save to cache
	CreateDirectory("cache", nullptr);
	FileWriter f(cache_path);
	if(f)
	{
		FILETIME fake_time = {0xFFFFFFFF, 0xFFFFFFFF};
		SetFileTime(f.file, nullptr, nullptr, &fake_time);
		f.Write(effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize());
		SetFileTime(f.file, nullptr, nullptr, &params.file_time);
	}
	else
		WARN(Format("Engine: Failed to save effect '%s' to cache. (%d)", params.cache_name, GetLastError()));

	// create effect from effect buffer
	ID3DXEffect* effect = nullptr;
	hr = D3DXCreateEffect(device, effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize(),
		params.macros, nullptr, flags, params.pool, &effect, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Engine: Failed to create effect '%s' (%d).\n%s (%d)", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

		SafeRelease(errors);
		SafeRelease(effect_buffer);
		SafeRelease(compiler);

		throw msg;
	}

	// free directx stuff
	SafeRelease(errors);
	SafeRelease(effect_buffer);
	SafeRelease(compiler);

	return effect;
}

//=================================================================================================
// podczas wczytywania aktualizuje okno i obs³uguje komunikaty
//=================================================================================================
void Engine::DoPseudotick()
{
	MSG msg = {0};
	if(!timer.IsStarted())
		timer.Start();

	while(msg.message != WM_QUIT && PeekMessage(&msg,0,0,0,PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DoTick(false);
}

//=================================================================================================
// Czêœæ wspólna dla WindowLoop i DoPseudoTick
//=================================================================================================
void Engine::DoTick(bool update_game)
{
	const float dt = timer.Tick();
	assert(dt >= 0.f);

	// oblicz fps
	frames++;
	frame_time += dt;
	if(frame_time >= 1.f)
	{
		fps = frames / frame_time;
		frames = 0;
		frame_time = 0.f;
	}

	// sprawdzanie czy okno jest aktywne
	if(active && GetForegroundWindow() != hwnd)
	{
		active = false;
		Key.ReleaseKeys();
		ShowCursor(false);
	}

	// pozycjonowanie myszki na œrodku okna
	if(active && replace_cursor)
	{
		PlaceCursor();
		replace_cursor = false;
	}

	// aktualizacja myszki
	if(active && locked_cursor)
	{
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hwnd, &p);
		mouse_dif.x = p.x - real_size.x/2;
		mouse_dif.y = p.y - real_size.y/2;
		PlaceCursor();
		ShowCursor(false);
	}
	else
		mouse_dif = INT2(0,0);

	// aktualizacja gry
	if(update_game)
		OnTick(dt);
	else
		UpdateMusic(dt);
	if(engine_shutdown)
	{
		if(active && locked_cursor)
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			int w = abs(rect.right-rect.left),
				h = abs(rect.bottom-rect.top);
			POINT pt;
			pt.x = int(float(unlock_point.x)*w/wnd_size.x);
			pt.y = int(float(unlock_point.y)*h/wnd_size.y);
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
		}
		return;
	}
	mouse_wheel = 0;

	Render();
	Key.Update();
	if(!disabled_sound)
		UpdateMusic(dt);
}

//=================================================================================================
// Rozpoczyna zamykanie silnika
//=================================================================================================
void Engine::EngineShutdown()
{
	if(!engine_shutdown)
	{
		engine_shutdown = true;
		LOG("Engine: Started closing engine...");
	}
}

//=================================================================================================
void Engine::FatalError(cstring err)
{
	assert(err);
	ShowError(err, Logger::L_FATAL);
	EngineShutdown();
}

//=================================================================================================
// Wype³nia parametry directx
//=================================================================================================
void Engine::GatherParams(D3DPRESENT_PARAMETERS& d3dpp)
{
	d3dpp.Windowed						= !fullscreen;
	d3dpp.BackBufferCount				= 1;
	d3dpp.BackBufferFormat				= BACKBUFFER_FORMAT;
	d3dpp.BackBufferWidth				= wnd_size.x;
	d3dpp.BackBufferHeight				= wnd_size.y;
	d3dpp.EnableAutoDepthStencil		= TRUE;
	d3dpp.MultiSampleType				= (D3DMULTISAMPLE_TYPE)multisampling;
	d3dpp.MultiSampleQuality			= multisampling_quality;
	d3dpp.hDeviceWindow					= hwnd;
	d3dpp.SwapEffect					= D3DSWAPEFFECT_DISCARD;
	d3dpp.AutoDepthStencilFormat		= ZBUFFER_FORMAT;
	d3dpp.Flags							= 0;
	d3dpp.PresentationInterval			= (vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
	d3dpp.FullScreen_RefreshRateInHz	= (fullscreen ? wnd_hz : 0);
}

//=================================================================================================
// Obs³uga komunikatu okna
//=================================================================================================
LRESULT Engine::HandleEvent(HWND in_hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool down = false;

	switch(msg)
	{
	// (dez)aktywowano okno
	case WM_ACTIVATE:
		down = (wParam != WA_INACTIVE);
		if(!active && down)
		{
			if(locked_cursor)
				PlaceCursor();
			else
				locked_cursor = true;
			mouse_dif = INT2(0,0);
		}
		else if(locked_cursor)
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			int w = abs(rect.right-rect.left),
				h = abs(rect.bottom-rect.top);
			POINT pt;
			pt.x = int(float(unlock_point.x)*w/wnd_size.x);
			pt.y = int(float(unlock_point.y)*h/wnd_size.y);
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);
		}
		if(active != down)
		{
			active = down;
			OnFocus(active);
		}
		ShowCursor(!active);
		if(!active)
			Key.ReleaseKeys();
		return 0;

	// okno zosta³o zamkniête / zniszczone
	case WM_CLOSE:
	case WM_DESTROY:
		engine_shutdown = true;
		return 0;

	// obs³uga klawiatury
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		down = true;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		if(key_callback)
		{
			if(down)
				key_callback(wParam);
		}
		else
			Key.Process((byte)wParam, down);
		return 0;

	// obs³uga myszki
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		down = true;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	case WM_MOUSEMOVE:
		if(key_callback)
		{
			if(down)
			{
				byte key;
				int ret = 0;
				switch(msg)
				{
				default:
					assert(0);
					break;
				case WM_LBUTTONDOWN:
					key = VK_LBUTTON;
					break;
				case WM_RBUTTONDOWN:
					key = VK_RBUTTON;
					break;
				case WM_MBUTTONDOWN:
					key = VK_MBUTTON;
					break;
				case WM_XBUTTONDOWN:
					key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
					ret = 1;
					break;
				}

				key_callback(key);
				return ret;
			}
		}
		else
		{
			if(msg != WM_MOUSEMOVE && !locked_cursor && down)
			{
				ShowCursor(false);
				RECT rect;
				GetClientRect(hwnd, &rect);
				int w = abs(rect.right-rect.left),
					h = abs(rect.bottom-rect.top);
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				cursor_pos.x = float(pt.x)*wnd_size.x/w;
				cursor_pos.y = float(pt.y)*wnd_size.y/h;
				PlaceCursor();

				if(active)
					locked_cursor = true;

				if(msg == WM_XBUTTONDOWN)
					return TRUE;
				else
					return 0;
			}

			byte key;
			int ret = 0;
			switch(msg)
			{
			case WM_MOUSEMOVE:
				return 0;
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				key = VK_LBUTTON;
				break;
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				key = VK_RBUTTON;
				break;
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				key = VK_MBUTTON;
				break;
			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
				key = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
				ret = 1;
				break;
			}

			Key.Process(key, down);
			return ret;
		}

	// zamknij okno z alt+spacja
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	// obs³uga wpisywania tekstu
	case WM_CHAR:
		OnChar((char)wParam);
		return 0;

	// rolka myszki
	case WM_MOUSEWHEEL:
		mouse_wheel += GET_WHEEL_DELTA_WPARAM(wParam);
		return 0;
	}

	// zwróæ domyœlny komunikat
	return DefWindowProc(in_hwnd, msg, wParam, lParam);
}

//=================================================================================================
// Inicjalizacja fizyki Bullet Physics
//=================================================================================================
void Engine::InitPhysics()
{
	phy_config = new btDefaultCollisionConfiguration;
	phy_dispatcher = new btCollisionDispatcher(phy_config);
	phy_broadphase = new btDbvtBroadphase;
	phy_world = new CustomCollisionWorld(phy_dispatcher, phy_broadphase, phy_config);

	INFO("Engine: Bullet physics system created.");
}

//=================================================================================================
// Tworzenie render directx 9
//=================================================================================================
void Engine::InitRender()
{
	HRESULT hr;

	// stwórz podstawowy obiekt direct3d
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(!d3d)
		throw "Engine: Failed to create direct3d object.";

	// ile jest adapterów
	uint adapters = d3d->GetAdapterCount();
	LOG(Format("Engine: Adapters count: %u", adapters));

	// pobierz informacje o adapterach
	D3DADAPTER_IDENTIFIER9 adapter;
	for(uint i=0; i<adapters; ++i)
	{
		hr = d3d->GetAdapterIdentifier(i, 0, &adapter);
		if(FAILED(hr))
			WARN(Format("Engine: Can't get info about adapter %d (%d).", i, hr));
		else
		{
			LOG(Format("Engine: Adapter %d: %s, version %d.%d.%d.%d", i, adapter.Description, HIWORD(adapter.DriverVersion.HighPart),
				LOWORD(adapter.DriverVersion.HighPart), HIWORD(adapter.DriverVersion.LowPart), LOWORD(adapter.DriverVersion.LowPart)));
		}
	}
	if(used_adapter > (int)adapters)
	{
		WARN(Format("Engine: Invalid adapter %d, defaulting to 0.", used_adapter));
		used_adapter = 0;
	}

	// sprawdŸ wersjê shaderów
	D3DCAPS9 caps;
	d3d->GetDeviceCaps(used_adapter, D3DDEVTYPE_HAL, &caps);
	if(D3DVS_VERSION(2,0) > caps.VertexShaderVersion || D3DPS_VERSION(2,0) > caps.PixelShaderVersion)
	{
		throw Format("Engine: Too old graphic card! This game require vertex and pixel shader in version 2.0+. "
			"Your card support:\nVertex shader: %d.%d\nPixel shader: %d.%d",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
	}
	else
	{
		LOG(Format("Supported shader version vertex: %d.%d, pixel: %d.%d.",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion)));

		int version = min(D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion));
		if(shader_version == -1 || shader_version > version)
			shader_version = version;

		LOG(Format("Using shader version %d.", shader_version));
	}

	// sprawdŸ rodzaj tekstur
	hr = d3d->CheckDeviceType(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Engine: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Engine: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Engine: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
			STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

	// multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisampling_quality < 0 || multisampling_quality >= (int)levels)
		{
			WARN("Engine: Unavailable multisampling quality, changed to 0.");
			multisampling_quality = 0;
		}
	}
	else
	{
		WARN(Format("Engine: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling));
		multisampling = 0;
		multisampling_quality = 0;
	}
	LogMultisampling();

	// rozdzielczoœci
	SelectResolution();

	// parametry urz¹dzenia
	D3DPRESENT_PARAMETERS d3dpp = {0};
	GatherParams(d3dpp);

	// tryby w których mo¿na utworzyæ urz¹dzenie
	const DWORD mode[] = {
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		D3DCREATE_MIXED_VERTEXPROCESSING,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING
	};
	const cstring mode_str[] = {
		"hardware",
		"mixed",
		"software"
	};

	// spróbuj utworzyæ urz¹dzenie w którymœ z tych trzech trybów
	for(uint i=0; i<3; ++i)
	{
		DWORD sel_mode = mode[i];
		hr = d3d->CreateDevice(used_adapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

		if(SUCCEEDED(hr))
		{
			LOG(Format("Engine: Created direct3d device in %s mode.", mode_str[i]));
			break;
		}
	}

	// nie uda³o siê utworzyæ urz¹dzenia
	if(FAILED(hr))
		throw Format("Engine: Failed to create direct3d device (%d).", hr);

	// sprite
	hr = D3DXCreateSprite(device, &sprite);
	if(FAILED(hr))
		throw Format("Engine: Failed to create direct3dx sprite (%d).", hr);
	
	INFO("Engine: Directx device created.");
}

//=================================================================================================
// Initialize FMOD library
//=================================================================================================
void Engine::InitSound()
{
	// if disabled, log it
	if(disabled_sound)
	{
		INFO("Engine: Sound and music is disabled.");
		return;
	}

	// create FMOD system
	FMOD_RESULT result = FMOD::System_Create(&fmod_system);
	if(result != FMOD_OK)
		throw Format("Engine: Failed to create FMOD system (%d).", result);

	// get number of drivers
	int count;
	result = fmod_system->getNumDrivers(&count);
	if(result != FMOD_OK)
		throw Format("Engine: Failed to get FMOD number of drivers (%d).", result);
	if(count == 0)
		WARN("Engine: No sound drivers.");
	else
	{
		LOG(Format("Engine: Sound drivers (%d):", count));
		for(int i = 0; i < count; ++i)
		{
			result = fmod_system->getDriverInfo(i, BUF, 256, nullptr);
			if(result == FMOD_OK)
				LOG(Format("Engine: Driver %d - %s", i, BUF));
			else
				ERROR(Format("Engine: Failed to get driver %d info (%d).", i, result));
		}
	}

	// get info about selected driver and output device
	int driver;
	FMOD_OUTPUTTYPE output;
	fmod_system->getDriver(&driver);
	fmod_system->getOutput(&output);
	LOG(Format("Engine: Using driver %d and output type %d.", driver, output));

	// initialize FMOD system
	const int tries = 3;
	bool ok = false;
	for(int i = 0; i < 3; ++i)
	{
		result = fmod_system->init(128, FMOD_INIT_NORMAL, nullptr);
		if(result != FMOD_OK)
		{
			ERROR(Format("Engine: Failed to initialize FMOD system (%d).", result));
			Sleep(100);
		}
		else
		{
			ok = true;
			break;
		}
	}
	if(!ok)
	{
		ERROR("Engine: Failed to initialize FMOD, disabling sound!");
		disabled_sound = true;
		return;
	}

	// create group for sounds and music
	fmod_system->createChannelGroup("default", &group_default);
	fmod_system->createChannelGroup("music", &group_music);

	INFO("Engine: FMOD sound system created.");
}

//=================================================================================================
// Tworzenie okna
//=================================================================================================
void Engine::InitWindow(cstring title)
{
	assert(title);

	// register window class
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, 
		[](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT { return Engine::Get().HandleEvent(hwnd, msg, wParam, lParam); },
		0, 0, GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), "Icon"), LoadCursor(nullptr, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), 
		nullptr, "Krystal", nullptr
	};
	if(!RegisterClassEx(&wc))
		throw Format("Failed to register window class (%d).", GetLastError());
	
	// create window
	AdjustWindowSize();
	hwnd = CreateWindowEx(0, "Krystal", title, fullscreen ? WS_POPUPWINDOW : WS_OVERLAPPEDWINDOW, 0, 0, real_size.x, real_size.y,
		nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	if(!hwnd)
		throw Format("Failed to create window (%d).", GetLastError());

	// position window
	if(!fullscreen)
	{
		if(s_wnd_pos.x != -1 || s_wnd_pos.y != -1 || s_wnd_size.x != -1 || s_wnd_size.y != -1)
		{
			// set window position from config file
			RECT rect;
			GetWindowRect(hwnd, &rect);
			if(s_wnd_pos.x != -1)
				rect.left = s_wnd_pos.x;
			if(s_wnd_pos.y != -1)
				rect.top = s_wnd_pos.y;
			INT2 size = real_size;
			if(s_wnd_size.x != -1)
				size.x = s_wnd_size.x;
			if(s_wnd_size.y != -1)
				size.y = s_wnd_size.y;
			SetWindowPos(hwnd, 0, rect.left, rect.top, size.x, size.y, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
		}
		else
		{
			// set window at center of screen
			MoveWindow(hwnd,
				(GetSystemMetrics(SM_CXSCREEN)-real_size.x)/2,
				(GetSystemMetrics(SM_CYSCREEN)-real_size.y)/2,
				real_size.x, real_size.y, false);
		}
	}

	// show window
	ShowWindow(hwnd, SW_SHOWNORMAL);
	unlock_point.x = (GetSystemMetrics(SM_CXSCREEN) - real_size.x) / 2;
	unlock_point.y = (GetSystemMetrics(SM_CYSCREEN) - real_size.y) / 2;

	INFO("Engine: Window created.");
}

//=================================================================================================
// Loguje dostêpne ustawienia multisamplingu
//=================================================================================================
void Engine::LogMultisampling()
{
	LocalString s = "Engine: Available multisampling: ";

	for(int j=2; j<=16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels)) &&
			SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			s += Format("x%d(%d), ", j, min(levels, levels2));
		}
	}

	if(s.at_back(1) == ':')
		s += "none";
	else
		s.pop(2);

	LOG(s);
}

//=================================================================================================
// Ustawia kursor na œrodku okna
//=================================================================================================
void Engine::PlaceCursor()
{
	POINT p;
	p.x = real_size.x/2;
	p.y = real_size.y/2;
	ClientToScreen(hwnd, &p);
	SetCursorPos(p.x, p.y);
}

//=================================================================================================
// Odtwarzanie muzyki
//=================================================================================================
void Engine::PlayMusic(FMOD::Sound* music)
{
	if(!music && !current_music)
		return;

	if(music && current_music)
	{
		FMOD::Sound* music_sound;
		current_music->getCurrentSound(&music_sound);

		if(music_sound == music)
			return;
	}

	if(current_music)
		fallbacks.push_back(current_music);

	if(music)
	{
		fmod_system->playSound(FMOD_CHANNEL_FREE, music, true, &current_music);
		current_music->setVolume(0.f);
		current_music->setPaused(false);
		current_music->setChannelGroup(group_music);
	}
	else
		current_music = nullptr;
}

//=================================================================================================
// Odtwarzanie dŸwiêku 2d
//=================================================================================================
void Engine::PlaySound2d(FMOD::Sound* sound)
{
	assert(sound);

	FMOD::Channel* channel;
	fmod_system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
	channel->setChannelGroup(group_default);
	playing_sounds.push_back(channel);
}

//=================================================================================================
// Odtwarzanie dŸwiêku 3d
//=================================================================================================
void Engine::PlaySound3d(FMOD::Sound* sound, const VEC3& pos, float smin, float smax)
{
	assert(sound);

	FMOD::Channel* channel;
	fmod_system->playSound(FMOD_CHANNEL_FREE, sound, true, &channel);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(smin, 10000.f/*smax*/);
	channel->setPaused(false);
	channel->setChannelGroup(group_default);
	playing_sounds.push_back(channel);
}

//=================================================================================================
// Renderowanie
//=================================================================================================
void Engine::Render(bool dont_call_present)
{
	HRESULT hr = device->TestCooperativeLevel();
	if(hr != D3D_OK)
	{
		lost_device = true;
		if(hr == D3DERR_DEVICELOST)
		{
			// urz¹dzenie utracone, nie mo¿na jeszcze resetowaæ
			Sleep(1);
			return;
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			// mo¿na resetowaæ
			if(!Reset(false))
			{
				Sleep(1);
				return;
			}
		}
		else
			throw Format("Engine: Lost directx device (%d)!", hr);
	}

	OnDraw();
	
	if(!dont_call_present)
	{
		hr = device->Present(nullptr, nullptr, hwnd, nullptr);
		if(FAILED(hr))
		{
			if(hr == D3DERR_DEVICELOST)
				lost_device = true;
			else
				throw Format("Engine: Failed to present screen (%d)!", hr);
		}
	}
}

//=================================================================================================
// Resetowanie urz¹dzenia directx
//=================================================================================================
bool Engine::Reset(bool force)
{
	LOG("Engine: Reseting device.");

	// zwolnij zasoby
	if(!res_freed)
	{
		res_freed = true;
		V( sprite->OnLostDevice() );
		OnReset();
	}

	// ustaw parametry
	D3DPRESENT_PARAMETERS d3dpp = {0};
	GatherParams(d3dpp);

	// resetuj
	HRESULT hr = device->Reset(&d3dpp);
	if(FAILED(hr))
	{
		if(force || hr != D3DERR_DEVICELOST)
			throw Format("Engine: Failed to reset directx device (%d)!", hr);
		else
		{
			WARN("Failed to reset device.");
			return false;
		}
	}

	// przywróæ zasoby
	OnReload();
	V( sprite->OnResetDevice() );
	lost_device = false;
	res_freed = false;

	return true;
}

namespace E
{
	struct Res
	{
		int w, h, hz;

		Res(int w, int h, int hz) : w(w), h(h), hz(hz) {}
	};

	inline bool ResPred(const Res& r1, const Res& r2)
	{
		if(r1.w > r2.w)
			return false;
		else if(r1.w < r2.w)
			return true;
		else if(r1.h > r2.h)
			return false;
		else if(r1.h < r2.h)
			return true;
		else if(r1.hz > r2.hz)
			return false;
		else if(r1.hz < r2.hz)
			return true;
		else
			return false;
	}
}

//=================================================================================================
// Log avaiable resolutions and select valid 
//=================================================================================================
void Engine::SelectResolution()
{
	// wypisz dostêpne rozdzielczoœci i ustal co wybraæ
	vector<E::Res> ress;
	LocalString str = "Engine: Available display modes:";
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	int best_hz = 0, best_valid_hz = 0;
	bool res_valid = false, hz_valid = false;
	for(uint i=0; i<display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V( d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode) );
		if(d_mode.Width < MIN_WIDTH || d_mode.Height < MIN_HEIGHT)
			continue;
		ress.push_back(E::Res(d_mode.Width, d_mode.Height, d_mode.RefreshRate));
		if(d_mode.Width == DEFAULT_WIDTH && d_mode.Height == DEFAULT_HEIGHT)
		{
			if(d_mode.RefreshRate > (uint)best_hz)
				best_hz = d_mode.RefreshRate;
		}
		if(d_mode.Width == wnd_size.x && d_mode.Height == wnd_size.y)
		{
			res_valid = true;
			if(d_mode.RefreshRate == wnd_hz)
				hz_valid = true;
			if((int)d_mode.RefreshRate > best_valid_hz)
				best_valid_hz = d_mode.RefreshRate;
		}
	}
	std::sort(ress.begin(), ress.end(), E::ResPred);
	int cw=0, ch=0;
	for(vector<E::Res>::iterator it = ress.begin(), end = ress.end(); it != end; ++it)
	{
		E::Res& r = *it;
		if(r.w != cw || r.h != ch)
		{
			if(it != ress.begin())
				str += " Hz)";
			str += Format("\n\t%dx%d (%d", r.w, r.h, r.hz);
			cw = r.w;
			ch = r.h;
		}
		else
			str += Format(", %d", r.hz);
	}
	str += " Hz)";
	LOG(str->c_str());

	// dostosuj wybran¹ rozdzielczoœæ
	if(!res_valid)
	{
		const INT2 defaul_res(1024,768);
		if(wnd_size.x != 0)
			WARN(Format("Engine: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wnd_size.x, wnd_size.y, defaul_res.x, defaul_res.y, best_hz));
		else
			LOG(Format("Engine: Defaulting resolution to %dx%dx (%d Hz).", defaul_res.x, defaul_res.y, best_hz));
		wnd_size = defaul_res;
		wnd_hz = best_hz;
		AdjustWindowSize();
		SetWindowPos(hwnd, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN)-real_size.x)/2, (GetSystemMetrics(SM_CYSCREEN)-real_size.y)/2,
			real_size.x, real_size.y, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	}
	else if(!hz_valid)
	{
		if(wnd_hz != 0)
			WARN(Format("Engine: Refresh rate %d Hz is not valid, defaulting to %d Hz.", wnd_hz, best_valid_hz));
		else
			LOG(Format("Engine: Defaulting refresh rate to %d Hz.", best_valid_hz));
		wnd_hz = best_valid_hz;
	}
}

//=================================================================================================
// Ustawienia startowego multisamplingu
//=================================================================================================
void Engine::SetStartingMultisampling(int _multisampling, int _multisampling_quality)
{
	if(_multisampling < 0 || _multisampling == 1 || _multisampling > D3DMULTISAMPLE_16_SAMPLES)
	{
		multisampling = 0;
		WARN(Format("Engine: Unsupported multisampling: %d.", _multisampling));
	}
	else
	{
		multisampling = _multisampling;
		multisampling_quality = _multisampling_quality;
	}
}

//=================================================================================================
// Zmienia tytu³ okna
//=================================================================================================
void Engine::SetTitle(cstring title)
{
	assert(title);
	SetWindowTextA(hwnd, title);
}

//=================================================================================================
// Wyœwietla/ukrywa kursor, u¿ywane przez przechwytywanie myszki
//=================================================================================================
void Engine::ShowCursor(bool _show)
{
	DWORD flag = _show ? CURSOR_SHOWING : 0;
	CURSORINFO cInfo;
	cInfo.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&cInfo);

	if(cInfo.flags != flag)
		::ShowCursor(_show);
}

//=================================================================================================
// Ukrywa okno i wyœwietla b³¹d
//=================================================================================================
void Engine::ShowError(cstring msg, Logger::LOG_LEVEL level)
{
	assert(msg);

	ShowWindow(hwnd, SW_HIDE);
	::ShowCursor(TRUE);
	logger->Log(msg, level);
	MessageBox(nullptr, msg, nullptr, MB_OK|MB_ICONERROR|MB_APPLMODAL);
}

//=================================================================================================
// Pocz¹tek inicjalizacji gry i jej uruchomienie
//=================================================================================================
bool Engine::Start(cstring title, bool _fullscreen, uint w, uint h)
{
	assert(title);
	assert(w >= MIN_WIDTH && h >= MIN_HEIGHT);

	// set parameters
	fullscreen = _fullscreen;
	wnd_size.x = max(w, MIN_WIDTH);
	wnd_size.y = max(h, MIN_HEIGHT);
	cursor_pos.x = float(wnd_size.x / 2);
	cursor_pos.y = float(wnd_size.y / 2);

	// initialize engine
	try
	{
		InitWindow(title);
		InitRender();
		InitSound();
		InitPhysics();
		resMgr.Init(device, fmod_system);
	}
	catch(cstring e)
	{
		ShowError(Format("Engine: Failed to initialize CaRpg engine!\n%s", e), Logger::L_FATAL);
		Cleanup();
		return false;
	}

	// initialize game
	if(!InitGame())
	{
		Cleanup();
		return false;
	}

	// loop game
	try
	{
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
// Zatrzymuje wszystkie dŸwiêki
//=================================================================================================
void Engine::StopSounds()
{
	for(vector<FMOD::Channel*>::iterator it = playing_sounds.begin(), end = playing_sounds.end(); it != end; ++it)
		(*it)->stop();
	playing_sounds.clear();
}

//=================================================================================================
// Uwalnianie kursora
//=================================================================================================
void Engine::UnlockCursor()
{
	locked_cursor = false;
	RECT rect;
	GetClientRect(hwnd, &rect);
	int w = abs(rect.right-rect.left),
		h = abs(rect.bottom-rect.top);
	POINT pt;
	pt.x = int(float(unlock_point.x)*w/wnd_size.x);
	pt.y = int(float(unlock_point.y)*h/wnd_size.y);
	ClientToScreen(hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
	ShowCursor(true);
}

//=================================================================================================
// Aktualizacja muzyki
//=================================================================================================
void Engine::UpdateMusic(float dt)
{
	bool deletions = false;
	float volume;

	for(vector<FMOD::Channel*>::iterator it = fallbacks.begin(), end = fallbacks.end(); it != end; ++it)
	{
		(*it)->getVolume(&volume);
		if((volume -= dt) <= 0.f)
		{
			(*it)->stop();
			*it = nullptr;
			deletions = true;
		}
		else
			(*it)->setVolume(volume);
	}

	if(deletions)
		RemoveNullElements(fallbacks);

	if(current_music)
	{
		current_music->getVolume(&volume);
		if(volume != 1.f)
		{
			volume = min(1.f, volume + dt);
			current_music->setVolume(volume);
		}

		bool playing;
		current_music->isPlaying(&playing);
		if(!playing)
			music_ended = true;
	}

	fmod_system->update();
}

//=================================================================================================
// G³ówna pêtla, obs³uga okna
//=================================================================================================
void Engine::WindowLoop()
{
	MSG msg = {0};

	// uruchom czas
	timer.Start();
	frames = 0;
	frame_time = 0.f;
	fps = 0.f;

	while(msg.message != WM_QUIT)
	{
		// obs³uga komunikatów winapi
		if(PeekMessage(&msg,0,0,0,PM_REMOVE))
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
