#include "Pch.h"
#include "EngineCore.h"
#include "EngineSettings.h"
#include "Render.h"
#include "RenderTarget.h"
#include "DirectX.h"

Render::Render() : d3d(nullptr), device(nullptr), sprite(nullptr)
{
}

Render::~Render()
{

}

void Render::Init(EngineSettings& settings)
{
	HRESULT hr;

	// copy settings
	adapter = settings.adapter;
	shader_version = settings.shader_version;

	// create direct3d object
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(!d3d)
		throw "Render: Failed to create direct3d object.";

	// get adapters count
	uint adapters = d3d->GetAdapterCount();
	Info("Render: Adapters count: %u", adapters);

	// get adapters info
	D3DADAPTER_IDENTIFIER9 adapter_info;
	for(uint i = 0; i < adapters; ++i)
	{
		hr = d3d->GetAdapterIdentifier(i, 0, &adapter_info);
		if(FAILED(hr))
			Warn("Render: Can't get info about adapter %d (%d).", i, hr);
		else
		{
			Info("Render: Adapter %d: %s, version %d.%d.%d.%d", i, adapter_info.Description, HIWORD(adapter_info.DriverVersion.HighPart),
				LOWORD(adapter_info.DriverVersion.HighPart), HIWORD(adapter_info.DriverVersion.LowPart), LOWORD(adapter_info.DriverVersion.LowPart));
		}
	}
	if(adapter > (int)adapters)
	{
		Warn("Render: Invalid adapter %d, defaulting to 0.", adapter);
		adapter = 0;
		settings.adapter = 0;
	}

	// check shaders version
	D3DCAPS9 caps;
	V(d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps));
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
		if(shader_version == -1 || shader_version > version)
		{
			shader_version = version;
			settings.shader_version = shader_version;
		}

		Info("Using shader version %d.", shader_version);
	}

	// check texture types
	hr = d3d->CheckDeviceType(adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, BACKBUFFER_FORMAT, fullscreen ? FALSE : TRUE);
	if(FAILED(hr))
		throw Format("Render: Unsupported backbuffer type %s for display %s! (%d)", STRING(BACKBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDeviceFormat(adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported depth buffer type %s for display %s! (%d)", STRING(ZBUFFER_FORMAT), STRING(DISPLAY_FORMAT), hr);

	hr = d3d->CheckDepthStencilMatch(adapter, D3DDEVTYPE_HAL, DISPLAY_FORMAT, D3DFMT_A8R8G8B8, ZBUFFER_FORMAT);
	if(FAILED(hr))
		throw Format("Render: Unsupported render target D3DFMT_A8R8G8B8 with display %s and depth buffer %s! (%d)",
			STRING(DISPLAY_FORMAT), STRING(BACKBUFFER_FORMAT), hr);

	// check multisampling
	DWORD levels, levels2;
	if(SUCCEEDED(d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels))
		&& SUCCEEDED(d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, D3DFMT_D24S8, fullscreen ? FALSE : TRUE,
		(D3DMULTISAMPLE_TYPE)multisampling, &levels2)))
	{
		levels = min(levels, levels2);
		if(multisampling_quality < 0 || multisampling_quality >= (int)levels)
		{
			Warn("Render: Unavailable multisampling quality, changed to 0.");
			multisampling_quality = 0;
		}
	}
	else
	{
		Warn("Render: Your graphic card don't support multisampling x%d. Maybe it's only available in fullscreen mode. "
			"Multisampling was turned off.", multisampling);
		multisampling = 0;
		multisampling_quality = 0;
	}
	LogMultisampling();

	// select resolution
	SelectResolution();

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
		hr = d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, sel_mode, &d3dpp, &device);

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

RenderTarget* Render::CreateRenderTarget(const Int2& size)
{
	assert(size.x > 0 && size.y > 0 && IsPow2(size.x) && IsPow2(size.y));
	RenderTarget* target = new RenderTarget;
	target->size = size;
	CreateRenderTargetTexture(target);
	targets.push_back(target);
	return target;
}

void Render::DestroyRenderTarget(RenderTarget* target)
{
	assert(target);
	RemoveElement(targets, target);
	SafeRelease(target->tex);
	SafeRelease(target->surf);
	delete target;
}

void Render::CreateRenderTargetTexture(RenderTarget* target)
{
	V(device->CreateTexture(target->size.x, target->size.y, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &target->tex, nullptr));
	D3DMULTISAMPLE_TYPE type = (D3DMULTISAMPLE_TYPE)multisampling;
	if(type != D3DMULTISAMPLE_NONE)
		V(device->CreateRenderTarget(target->size.x, target->size.y, D3DFMT_A8R8G8B8, type, multisampling_quality, FALSE, &target->surf, nullptr));
	else
		target->surf = nullptr;
}

void Render::BeforeDeviceReset()
{
	for(RenderTarget* target : targets)
	{
		SafeRelease(target->tex);
		SafeRelease(target->surf);
	}
}

void Render::AfterDeviceReset()
{
	for(RenderTarget* target : targets)
		CreateRenderTargetTexture(target);
}
