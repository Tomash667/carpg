#pragma once

//-----------------------------------------------------------------------------
enum TextureAddressMode
{
	TEX_ADR_WRAP = 1,
	TEX_ADR_MIRROR = 2,
	TEX_ADR_CLAMP = 3,
	TEX_ADR_BORDER = 4,
	TEX_ADR_MIRRORONCE = 5
};

//-----------------------------------------------------------------------------
struct Resolution
{
	Int2 size;
	uint hz;
};

//-----------------------------------------------------------------------------
struct CompileShaderParams
{
	cstring name;
	cstring cache_name;
	string* input;
	FileTime file_time;
	D3DXMACRO* macros;
	ID3DXEffectPool* pool;
};

//-----------------------------------------------------------------------------
class Render
{
public:
	Render();
	~Render();
	void Init(StartupOptions& options);
	bool Reset(bool force);
	void WaitReset();
	void Draw(bool call_present = true);
	bool CheckDisplay(const Int2& size, int& hz); // dla zera zwraca najlepszy hz
	void RegisterShader(ShaderHandler* shader);
	ID3DXEffect* CompileShader(cstring name);
	ID3DXEffect* CompileShader(CompileShaderParams& params);
	TEX CreateTexture(const Int2& size);
	RenderTarget* CreateRenderTarget(const Int2& size);
	TEX CopyToTexture(RenderTarget* target);
	bool IsLostDevice() const { return lost_device; }
	bool IsMultisamplingEnabled() const { return multisampling != 0; }
	bool IsVsyncEnabled() const { return vsync; }
	IDirect3DDevice9* GetDevice() const { return device; }
	ID3DXSprite* GetSprite() const { return sprite; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisampling_quality; }
	void GetResolutions(vector<Resolution>& v) const;
	void GetMultisamplingModes(vector<Int2>& v) const;
	int GetRefreshRate() const { return refresh_hz; }
	vector<ShaderHandler*>& GetShaders() { return shaders; }
	int GetShaderVersion() const { return shader_version; }
	int GetAdapter() const { return used_adapter; }
	void SetAlphaBlend(bool use_alphablend);
	void SetAlphaTest(bool use_alphatest);
	void SetNoCulling(bool use_nocull);
	void SetNoZWrite(bool use_nozwrite);
	void SetVsync(bool vsync);
	int SetMultisampling(int type, int quality);
	void SetRefreshRateInternal(int refresh_hz) { this->refresh_hz = refresh_hz; }
	void SetShaderVersion(int shader_version) { this->shader_version = shader_version; }
	void SetTarget(RenderTarget* target);
	void SetTextureAddressMode(TextureAddressMode mode);

private:
	void GatherParams(D3DPRESENT_PARAMETERS& d3dpp);
	void LogMultisampling();
	void LogAndSelectResolution();
	void SetDefaultRenderState();
	void CreateRenderTargetTexture(RenderTarget* target);
	void BeforeReset();
	void AfterReset();

	IDirect3D9* d3d;
	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	vector<ShaderHandler*> shaders;
	vector<RenderTarget*> targets;
	RenderTarget* current_target;
	SURFACE current_surf;
	int used_adapter, shader_version, refresh_hz, multisampling, multisampling_quality;
	bool vsync, lost_device, res_freed, r_alphatest, r_nozwrite, r_nocull, r_alphablend;
};
