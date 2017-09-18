#pragma once

//-----------------------------------------------------------------------------
#include "Timer.h"
#include "KeyStates.h"
#include "Physics.h"

//-----------------------------------------------------------------------------
#define DISPLAY_FORMAT D3DFMT_X8R8G8B8
#define BACKBUFFER_FORMAT D3DFMT_A8R8G8B8
#define ZBUFFER_FORMAT D3DFMT_D24S8

//-----------------------------------------------------------------------------
typedef delegate<void(int)> KeyDownCallback;

//-----------------------------------------------------------------------------
struct CompileShaderParams
{
	cstring name;
	cstring cache_name;
	string* input;
	FILETIME file_time;
	D3DXMACRO* macros;
	ID3DXEffectPool* pool;
};

//-----------------------------------------------------------------------------
// Silnik
class Engine
{
public:
	struct StartupOptions
	{
		cstring title;
		Int2 size, force_size, force_pos;
		bool fullscreen, vsync;

		StartupOptions() : title(nullptr), size(0, 0), force_size(-1, -1), force_pos(-1, -1), fullscreen(false), vsync(true)
		{
		}
	};

	Engine();
	virtual ~Engine() {}

	static Engine& Get() { return *engine; }

	bool ChangeMode(bool fullscreen);
	bool ChangeMode(const Int2& size, bool fullscreen, int hz = 0);
	int ChangeMultisampling(int type, int level);
	bool CheckDisplay(const Int2& size, int& hz); // dla zera zwraca najlepszy hz
	ID3DXEffect* CompileShader(cstring name);
	ID3DXEffect* CompileShader(CompileShaderParams& params);
	void DoPseudotick();
	void EngineShutdown();
	void FatalError(cstring err);
	void PlayMusic(FMOD::Sound* music);
	void PlaySound2d(FMOD::Sound* sound);
	void PlaySound3d(FMOD::Sound* sound, const Vec3& pos, float smin, float smax = 0.f); // smax jest nieu¿ywane
	void Render(bool dont_call_present = false);
	bool Reset(bool force);
	void ShowError(cstring msg, Logger::Level level = Logger::L_ERROR);
	bool Start(StartupOptions& options);
	void StopSounds();
	void UnlockCursor(bool lock_on_focus = true);
	void LockCursor();
	void UpdateMusic(float dt);

	bool IsActive() const { return active; }
	bool IsCursorLocked() const { return locked_cursor; }
	bool IsCursorVisible() const { return cursor_visible; }
	bool IsEngineShutdown() const { return engine_shutdown; }
	bool IsFullscreen() const { return fullscreen; }
	bool IsLostDevice() const { return lost_device; }
	bool IsMultisamplingEnabled() const { return multisampling != 0; }

	float GetFps() const { return fps; }
	const Int2& GetMouseDif() const { return mouse_dif; }
	float GetMouseWheel() const { return mouse_wheel; }
	void GetMultisampling(int& ms, int& msq) const { ms = multisampling; msq = multisampling_quality; }
	float GetWindowAspect() const { return float(wnd_size.x) / wnd_size.y; }
	HWND GetWindowHandle() const { return hwnd; }
	bool GetVsync() const { return vsync; }
	const Int2& GetWindowSize() const { return wnd_size; }

	void SetAlphaBlend(bool use_alphablend);
	void SetAlphaTest(bool use_alphatest);
	void SetNoCulling(bool use_nocull);
	void SetNoZWrite(bool use_nozwrite);
	void SetStartingMultisampling(int multisampling, int multisampling_quality);
	void SetTitle(cstring title);
	void SetUnlockPoint(const Int2& pt) { unlock_point = pt; }
	void SetVsync(bool vsync);

	// ----- ZMIENNE -----
	KeyDownCallback key_callback;

	// directx
	IDirect3D9* d3d;
	IDirect3DDevice9* device;
	ID3DXSprite* sprite;
	DWORD clear_color;
	int wnd_hz, used_adapter, shader_version;

	// FMOD
	FMOD::System* fmod_system;
	FMOD::ChannelGroup* group_default, *group_music;
	FMOD::Channel* current_music;
	vector<FMOD::Channel*> playing_sounds;
	vector<FMOD::Channel*> fallbacks;
	bool music_ended, disabled_sound;

	// bullet physics
	btDefaultCollisionConfiguration* phy_config;
	btCollisionDispatcher* phy_dispatcher;
	btDbvtBroadphase* phy_broadphase;
	CustomCollisionWorld* phy_world;

	// constants
	static const Int2 MIN_WINDOW_SIZE;
	static const Int2 DEFAULT_WINDOW_SIZE;

protected:
	// funkcje implementowane przez Game
	virtual bool InitGame() = 0;
	virtual void OnChar(char c) = 0;
	virtual void OnCleanup() = 0;
	virtual void OnDraw() = 0;
	virtual void OnReload() = 0;
	virtual void OnReset() = 0;
	virtual void OnResize() = 0;
	virtual void OnTick(float dt) = 0;
	virtual void OnFocus(bool focus, const Int2& activation_point) = 0;

private:
	void AdjustWindowSize();
	void ChangeMode();
	void Cleanup();
	void DoTick(bool update_game);
	void GatherParams(D3DPRESENT_PARAMETERS& d3dpp);
	LRESULT HandleEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void MsgToKey(UINT msg, WPARAM wParam, byte& key, int& result);
	void InitPhysics();
	void InitRender();
	void InitSound();
	void InitWindow(StartupOptions& options);
	void LogMultisampling();
	void PlaceCursor();
	void SelectResolution();
	void SetDefaultRenderState();
	void ShowCursor(bool show);
	void UpdateActivity(bool is_active);
	void WindowLoop();

	static Engine* engine;
	int multisampling, multisampling_quality;
	uint frames;
	float frame_time;
	Timer timer;
	bool engine_shutdown, lost_device, res_freed, cursor_visible, replace_cursor, locked_cursor, lock_on_focus;
	bool r_alphatest, r_nozwrite, r_nocull, r_alphablend;

	// window
	HWND hwnd;
	Int2 wnd_size, real_size, mouse_dif, unlock_point, activation_point;
	float fps, mouse_wheel;
	bool active, fullscreen;

	// render
	bool vsync;
};
