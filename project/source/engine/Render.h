#pragma once

class RenderTarget;

class Render
{
public:
	Render();
	~Render();
	void Init(EngineSettings& settings);
	RenderTarget* CreateRenderTarget(const Int2& size);
	void DestroyRenderTarget(RenderTarget* target);

private:
	void CreateRenderTargetTexture(RenderTarget* target);
	void BeforeDeviceReset();
	void AfterDeviceReset();

	IDirect3D9* d3d;
	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	int adapter, shader_version, multisampling, multisampling_quality;
	vector<RenderTarget*> targets;
};
