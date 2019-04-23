#include "Pch.h"
#include "EngineCore.h"
#include "Render.h"
#include "RenderTarget.h"
#include "StartupOptions.h"
#include "Engine.h"
#include "ShaderHandler.h"
#include "File.h"
#include "DirectX.h"

static const D3DFORMAT DISPLAY_FORMAT = D3DFMT_X8R8G8B8;
static const D3DFORMAT BACKBUFFER_FORMAT = D3DFMT_A8R8G8B8;
static const D3DFORMAT ZBUFFER_FORMAT = D3DFMT_D24S8;
extern string g_system_dir;

//=================================================================================================
Render::Render() : d3d(nullptr), device(nullptr), sprite(nullptr), current_target(nullptr), current_surf(nullptr), vsync(true), lost_device(false),
res_freed(false)
{
}

//=================================================================================================
Render::~Render()
{
	for(RenderTarget* target : targets)
	{
		SafeRelease(target->tex);
		SafeRelease(target->surf);
		delete target;
	}
	if(device)
	{
		device->SetStreamSource(0, nullptr, 0, 0);
		device->SetIndices(nullptr);
	}
	SafeRelease(sprite);
	SafeRelease(device);
	SafeRelease(d3d);
}

//=================================================================================================
void Render::Init(StartupOptions& options)
{
	HRESULT hr;

	// copy settings
	vsync = options.vsync;
	used_adapter = options.used_adapter;
	shader_version = options.shader_version;
	multisampling = options.multisampling;
	multisampling_quality = options.multisampling_quality;
	refresh_hz = options.refresh_hz;

	// create direct3d object
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(!d3d)
		throw "Render: Failed to create direct3d object.";

	// get adapters count
	uint adapters = d3d->GetAdapterCount();
	Info("Render: Adapters count: %u", adapters);

	// get adapters info
	D3DADAPTER_IDENTIFIER9 adapter;
	for(uint i = 0; i < adapters; ++i)
	{
		hr = d3d->GetAdapterIdentifier(i, 0, &adapter);
		if(FAILED(hr))
			Warn("Render: Can't get info about adapter %d (%d).", i, hr);
		else
		{
			Info("Render: Adapter %d: %s, version %d.%d.%d.%d", i, adapter.Description, HIWORD(adapter.DriverVersion.HighPart),
				LOWORD(adapter.DriverVersion.HighPart), HIWORD(adapter.DriverVersion.LowPart), LOWORD(adapter.DriverVersion.LowPart));
		}
	}
	if(used_adapter > (int)adapters)
	{
		Warn("Render: Invalid adapter %d, defaulting to 0.", used_adapter);
		used_adapter = 0;
		options.used_adapter = 0;
	}

	// check shaders version
	D3DCAPS9 caps;
	d3d->GetDeviceCaps(used_adapter, D3DDEVTYPE_HAL, &caps);
	if(D3DVS_VERSION(2, 0) > caps.VertexShaderVersion || D3DPS_VERSION(2, 0) > caps.PixelShaderVersion)
	{
		throw Format("Render: Too old graphic card! This game require vertex and pixel shader in version 2.0+. "
			"Your card support:\nVertex shader: %d.%d\nPixel shader: %d.%d",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
	}
	else
	{
		Info("Supported shader version vertex: %d.%d, pixel: %d.%d.",
			D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion),
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion), D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));

		int version = min(D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion), D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion));
		if(shader_version == -1 || shader_version > version || shader_version < 2)
		{
			shader_version = version;
			options.shader_version = version;
		}

		Info("Using shader version %d.", shader_version);
	}

	// check texture types
	bool fullscreen = Engine::Get().IsFullscreen();
	hr = d3d->CheckDeviceType(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Render: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(used_adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
		STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisampling_quality < 0 || multisampling_quality >= (int)levels)
		{
			Warn("Render: Unavailable multisampling quality, changed to 0.");
			multisampling_quality = 0;
			options.multisampling_quality = 0;
		}
	}
	else
	{
		Warn("Render: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling);
		multisampling = 0;
		multisampling_quality = 0;
		options.multisampling = 0;
		options.multisampling_quality = 0;
	}

	LogMultisampling();
	LogAndSelectResolution();

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// available modes
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

	// try to create device in one of modes
	for(uint i = 0; i < 3; ++i)
	{
		DWORD sel_mode = mode[i];
		hr = d3d->CreateDevice(used_adapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

		if(SUCCEEDED(hr))
		{
			Info("Render: Created direct3d device in %s mode.", mode_str[i]);
			break;
		}
	}

	// failed to create device
	if(FAILED(hr))
		throw Format("Render: Failed to create direct3d device (%d).", hr);

	// create sprite
	hr = D3DXCreateSprite(device, &sprite);
	if(FAILED(hr))
		throw Format("Render: Failed to create direct3dx sprite (%d).", hr);

	SetDefaultRenderState();

	Info("Render: Directx device created.");
}

//=================================================================================================
void Render::GatherParams(D3DPRESENT_PARAMETERS& d3dpp)
{
	Engine& engine = Engine::Get();
	d3dpp.Windowed = !engine.IsFullscreen();
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferFormat = BACKBUFFER_FORMAT;
	d3dpp.BackBufferWidth = engine.GetWindowSize().x;
	d3dpp.BackBufferHeight = engine.GetWindowSize().y;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)multisampling;
	d3dpp.MultiSampleQuality = multisampling_quality;
	d3dpp.hDeviceWindow = engine.GetWindowHandle();
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.AutoDepthStencilFormat = ZBUFFER_FORMAT;
	d3dpp.Flags = 0;
	d3dpp.PresentationInterval = (vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
	d3dpp.FullScreen_RefreshRateInHz = (d3dpp.Windowed ? 0 : refresh_hz);
}

//=================================================================================================
void Render::LogMultisampling()
{
	LocalString s = "Render: Available multisampling: ";

	for(int j = 2; j <= 16; ++j)
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

	Info(s);
}

//=================================================================================================
void Render::LogAndSelectResolution()
{
	struct Res
	{
		int w, h, hz;

		Res(int w, int h, int hz) : w(w), h(h), hz(hz) {}
		bool operator < (const Res& r) const
		{
			if(w > r.w)
				return false;
			else if(w < r.w)
				return true;
			else if(h > r.h)
				return false;
			else if(h < r.h)
				return true;
			else if(hz > r.hz)
				return false;
			else if(hz < r.hz)
				return true;
			else
				return false;
		}
	};

	vector<Res> ress;
	LocalString str = "Render: Available display modes:";
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	Int2 wnd_size = Engine::Get().GetWindowSize();
	int best_hz = 0, best_valid_hz = 0;
	bool res_valid = false, hz_valid = false;
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width < (uint)Engine::MIN_WINDOW_SIZE.x || d_mode.Height < (uint)Engine::MIN_WINDOW_SIZE.y)
			continue;
		ress.push_back(Res(d_mode.Width, d_mode.Height, d_mode.RefreshRate));
		if(d_mode.Width == (uint)Engine::DEFAULT_WINDOW_SIZE.x && d_mode.Height == (uint)Engine::DEFAULT_WINDOW_SIZE.y)
		{
			if(d_mode.RefreshRate > (uint)best_hz)
				best_hz = d_mode.RefreshRate;
		}
		if(d_mode.Width == wnd_size.x && d_mode.Height == wnd_size.y)
		{
			res_valid = true;
			if(d_mode.RefreshRate == refresh_hz)
				hz_valid = true;
			if((int)d_mode.RefreshRate > best_valid_hz)
				best_valid_hz = d_mode.RefreshRate;
		}
	}
	std::sort(ress.begin(), ress.end());
	int cw = 0, ch = 0;
	for(vector<Res>::iterator it = ress.begin(), end = ress.end(); it != end; ++it)
	{
		Res& r = *it;
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
	Info(str->c_str());

	// adjust selected resolution
	if(!res_valid)
	{
		if(wnd_size.x != 0)
		{
			Warn("Render: Resolution %dx%d is not valid, defaulting to %dx%d (%d Hz).", wnd_size.x, wnd_size.y,
				Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, best_hz);
		}
		else
			Info("Render: Defaulting resolution to %dx%dx (%d Hz).", Engine::DEFAULT_WINDOW_SIZE.x, Engine::DEFAULT_WINDOW_SIZE.y, best_hz);
		refresh_hz = best_hz;
		Engine::Get().SetWindowSizeInternal(Engine::DEFAULT_WINDOW_SIZE);
	}
	else if(!hz_valid)
	{
		if(refresh_hz != 0)
			Warn("Render: Refresh rate %d Hz is not valid, defaulting to %d Hz.", refresh_hz, best_valid_hz);
		else
			Info("Render: Defaulting refresh rate to %d Hz.", best_valid_hz);
		refresh_hz = best_valid_hz;
	}
}

//=================================================================================================
void Render::SetDefaultRenderState()
{
	V(device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));
	V(device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	V(device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));
	V(device->SetRenderState(D3DRS_ALPHAREF, 200));
	V(device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL));

	r_alphatest = false;
	r_alphablend = false;
	r_nocull = false;
	r_nozwrite = false;
}

//=================================================================================================
bool Render::Reset(bool force)
{
	Info("Render: Reseting device.");
	BeforeReset();

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// reset
	HRESULT hr = device->Reset(&d3dpp);
	if(FAILED(hr))
	{
		if(force || hr != D3DERR_DEVICELOST)
		{
			if(hr == D3DERR_INVALIDCALL)
				throw "Render: Device reset returned D3DERR_INVALIDCALL, not all resources was released.";
			else
				throw Format("Render: Failed to reset directx device (%d).", hr);
		}
		else
		{
			Warn("Render: Failed to reset device.");
			return false;
		}
	}

	AfterReset();
	return true;
}

//=================================================================================================
void Render::WaitReset()
{
	Info("Render: Device lost at loading. Waiting for reset.");
	BeforeReset();

	Engine::Get().UpdateActivity(false);

	// gather params
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	GatherParams(d3dpp);

	// wait for reset
	while(true)
	{
		Engine::Get().DoPseudotick(true);

		HRESULT hr = device->TestCooperativeLevel();
		if(hr == D3DERR_DEVICELOST)
		{
			Info("Render: Device lost, waiting...");
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			Info("Render: Device can be reseted, trying...");

			// reset
			hr = device->Reset(&d3dpp);
			if(FAILED(hr))
			{
				if(hr == D3DERR_DEVICELOST)
					Warn("Render: Can't reset, device is lost.");
				else if(hr == D3DERR_INVALIDCALL)
					throw "Render: Device reset returned D3DERR_INVALIDCALL, not all resources was released.";
				else
					throw Format("Render: Device reset returned error (%u).", hr);
			}
			else
				break;
		}
		else
			throw Format("Render: Device lost and cannot reset (%u).", hr);
		Sleep(500);
	}

	AfterReset();
}


//=================================================================================================
void Render::Draw(bool call_present)
{
	HRESULT hr = device->TestCooperativeLevel();
	if(hr != D3D_OK)
	{
		lost_device = true;
		if(hr == D3DERR_DEVICELOST)
		{
			// device lost, can't reset yet
			Sleep(1);
			return;
		}
		else if(hr == D3DERR_DEVICENOTRESET)
		{
			// try reset
			if(!Reset(false))
			{
				Sleep(1);
				return;
			}
		}
		else
			throw Format("Render: Lost directx device (%d).", hr);
	}

	Engine::Get().OnDraw();

	if(call_present)
	{
		hr = device->Present(nullptr, nullptr, Engine::Get().GetWindowHandle(), nullptr);
		if(FAILED(hr))
		{
			if(hr == D3DERR_DEVICELOST)
				lost_device = true;
			else
				throw Format("Render: Failed to present screen (%d).", hr);
		}
	}
}

//=================================================================================================
void Render::BeforeReset()
{
	if(res_freed)
		return;
	res_freed = true;
	Engine::Get().OnReset();
	V(sprite->OnLostDevice());
	for(ShaderHandler* shader : shaders)
		shader->OnReset();
	for(RenderTarget* target : targets)
	{
		SafeRelease(target->tex);
		SafeRelease(target->surf);
	}
}

//=================================================================================================
void Render::AfterReset()
{
	SetDefaultRenderState();
	V(sprite->OnResetDevice());
	for(ShaderHandler* shader : shaders)
		shader->OnReload();
	for(RenderTarget* target : targets)
		CreateRenderTargetTexture(target);
	V(sprite->OnResetDevice());
	Engine::Get().OnReload();
	lost_device = false;
	res_freed = false;
}

//=================================================================================================
bool Render::CheckDisplay(const Int2& size, int& hz)
{
	assert(size.x >= Engine::MIN_WINDOW_SIZE.x && size.x >= Engine::MIN_WINDOW_SIZE.y);

	// check minimum resolution
	if(size.x < Engine::MIN_WINDOW_SIZE.x || size.y < Engine::MIN_WINDOW_SIZE.y)
		return false;

	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);

	if(hz == 0)
	{
		bool valid = false;

		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height)
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
		for(uint i = 0; i < display_modes; ++i)
		{
			D3DDISPLAYMODE d_mode;
			V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
			if(size.x == d_mode.Width && size.y == d_mode.Height && hz == d_mode.RefreshRate)
				return true;
		}

		return false;
	}
}

//=================================================================================================
void Render::RegisterShader(ShaderHandler* shader)
{
	assert(shader);
	shaders.push_back(shader);
	shader->OnInit();
}

//=================================================================================================
ID3DXEffect* Render::CompileShader(cstring name)
{
	assert(name);

	CompileShaderParams params = { name };

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
ID3DXEffect* Render::CompileShader(CompileShaderParams& params)
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
			throw Format("Render: Failed to load shader '%s' (%d).", params.name, GetLastError());
		params.file_time = file.GetTime();
	}

	// check if in cache
	{
		FileReader cache_file(cache_path);
		if(cache_file && params.file_time == cache_file.GetTime())
		{
			// same last modify time, use cache
			cache_file.ReadToString(g_tmp_string);
			ID3DXEffect* effect = nullptr;
			hr = D3DXCreateEffect(device, g_tmp_string.c_str(), g_tmp_string.size(), params.macros, nullptr, flags, params.pool, &effect, &errors);
			if(FAILED(hr))
			{
				Error("Render: Failed to create effect from cache '%s' (%d).\n%s", params.cache_name, hr,
					errors ? (cstring)errors->GetBufferPointer() : "No errors information.");
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

		cstring msg = Format("Render: Failed to compile shader '%s' (%d).\n%s", params.name, hr, str);

		SafeRelease(errors);

		throw msg;
	}
	SafeRelease(errors);

	// compile shader
	ID3DXBuffer* effect_buffer = nullptr;
	hr = compiler->CompileEffect(flags, &effect_buffer, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Render: Failed to compile effect '%s' (%d).\n%s", params.name, hr,
			errors ? (cstring)errors->GetBufferPointer() : "No errors information.");

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
		f.Write(effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize());
		f.SetTime(params.file_time);
	}
	else
		Warn("Render: Failed to save effect '%s' to cache (%d).", params.cache_name, GetLastError());

	// create effect from effect buffer
	ID3DXEffect* effect = nullptr;
	hr = D3DXCreateEffect(device, effect_buffer->GetBufferPointer(), effect_buffer->GetBufferSize(),
		params.macros, nullptr, flags, params.pool, &effect, &errors);
	if(FAILED(hr))
	{
		cstring msg = Format("Render: Failed to create effect '%s' (%d).\n%s", params.name, hr,
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
TEX Render::CreateTexture(const Int2& size)
{
	TEX tex;
	V(device->CreateTexture(size.x, size.y, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr));
	return tex;
}

//=================================================================================================
RenderTarget* Render::CreateRenderTarget(const Int2& size)
{
	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
	RenderTarget* target = new RenderTarget;
	target->size = size;
	CreateRenderTargetTexture(target);
	targets.push_back(target);
	return target;
}

//=================================================================================================
void Render::CreateRenderTargetTexture(RenderTarget* target)
{
	V(device->CreateTexture(target->size.x, target->size.y, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &target->tex, nullptr));
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)multisampling;
	if(type != D3DMULTISAMPLE_NONE)
		V(device->CreateRenderTarget(target->size.x, target->size.y, D3DFMT_A8R8G8B8, type, multisampling_quality, FALSE, &target->surf, nullptr));
	else
		target->surf = nullptr;
}

//=================================================================================================
TEX Render::CopyToTexture(RenderTarget* target)
{
	assert(target);
	D3DSURFACE_DESC desc;
	V(target->tex->GetLevelDesc(0, &desc));
	TEX tex;
	V(device->CreateTexture(desc.Width, desc.Height, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr));
	SURFACE surf;
	V(tex->GetSurfaceLevel(0, &surf));
	HRESULT hr = D3DXLoadSurfaceFromSurface(surf, nullptr, nullptr, target->GetSurface(), nullptr, nullptr, D3DX_DEFAULT, 0);
	target->FreeSurface();
	surf->Release();
	if(SUCCEEDED(hr))
		return tex;
	else
	{
		tex->Release();
		WaitReset();
		return nullptr;
	}
}

//=================================================================================================
void Render::SetAlphaBlend(bool use_alphablend)
{
	if(use_alphablend != r_alphablend)
	{
		r_alphablend = use_alphablend;
		V(device->SetRenderState(D3DRS_ALPHABLENDENABLE, r_alphablend ? TRUE : FALSE));
	}
}

//=================================================================================================
void Render::SetAlphaTest(bool use_alphatest)
{
	if(use_alphatest != r_alphatest)
	{
		r_alphatest = use_alphatest;
		V(device->SetRenderState(D3DRS_ALPHATESTENABLE, r_alphatest ? TRUE : FALSE));
	}
}

//=================================================================================================
void Render::SetNoCulling(bool use_nocull)
{
	if(use_nocull != r_nocull)
	{
		r_nocull = use_nocull;
		V(device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW));
	}
}

//=================================================================================================
void Render::SetNoZWrite(bool use_nozwrite)
{
	if(use_nozwrite != r_nozwrite)
	{
		r_nozwrite = use_nozwrite;
		V(device->SetRenderState(D3DRS_ZWRITEENABLE, r_nozwrite ? FALSE : TRUE));
	}
}

//=================================================================================================
void Render::SetVsync(bool new_vsync)
{
	if(new_vsync == vsync)
		return;

	vsync = new_vsync;
	Reset(true);
}

//=================================================================================================
int Render::SetMultisampling(int type, int level)
{
	if(type == multisampling && (level == -1 || level == multisampling_quality))
		return 1;

	bool fullscreen = Engine::Get().IsFullscreen();
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels)) &&
		SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE, (D3DMULTISAMPLE_TYPE)type, &levels2)))
	{
		levels = min(levels, levels2);
		if(level < 0)
			level = levels - 1;
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
void Render::GetResolutions(vector<Resolution>& v) const
{
	v.clear();
	uint display_modes = d3d->GetAdapterModeCount(used_adapter, DISPLAY_FORMAT);
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(d3d->EnumAdapterModes(used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width >= (uint)Engine::MIN_WINDOW_SIZE.x && d_mode.Height >= (uint)Engine::MIN_WINDOW_SIZE.y)
			v.push_back({ Int2(d_mode.Width, d_mode.Height), d_mode.RefreshRate });
	}
}

//=================================================================================================
void Render::GetMultisamplingModes(vector<Int2>& v) const
{
	v.clear();
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels))
			&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			int level = min(levels, levels2);
			for(int i = 0; i < level; ++i)
				v.push_back(Int2(j, i));
		}
	}
}

//=================================================================================================
void Render::SetTarget(RenderTarget* target)
{
	if(target)
	{
		assert(!current_target);

		if(target->surf)
			V(device->SetRenderTarget(0, target->surf));
		else
		{
			V(target->tex->GetSurfaceLevel(0, &current_surf));
			V(device->SetRenderTarget(0, current_surf));
		}

		current_target = target;
	}
	else
	{
		assert(current_target);

		if(current_target->tmp_surf)
		{
			current_target->surf->Release();
			current_target->surf = nullptr;
			current_target->tmp_surf = false;
		}
		else
		{
			// copy to surface if using multisampling
			if(current_target->surf)
			{
				V(current_target->tex->GetSurfaceLevel(0, &current_surf));
				V(device->StretchRect(current_target->surf, nullptr, current_surf, nullptr, D3DTEXF_NONE));
			}
			current_surf->Release();
		}

		// restore old render target
		V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &current_surf));
		V(device->SetRenderTarget(0, current_surf));
		current_surf->Release();

		current_target = nullptr;
		current_surf = nullptr;
	}
}

//=================================================================================================
void Render::SetTextureAddressMode(TextureAddressMode mode)
{
	V(device->SetSamplerState(0, D3DSAMP_ADDRESSU, (D3DTEXTUREADDRESS)D3DTADDRESS_WRAP));
	V(device->SetSamplerState(0, D3DSAMP_ADDRESSV, (D3DTEXTUREADDRESS)D3DTADDRESS_WRAP));
}
