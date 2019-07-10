#pragma once

//-----------------------------------------------------------------------------
#include "Timer.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
class Engine
{
	friend class Render;
public:
	static const Int2 MIN_WINDOW_SIZE;
	static const Int2 DEFAULT_WINDOW_SIZE;

	Engine();

	static Engine& Get() { return *engine; }

	bool ChangeMode(bool fullscreen);
	bool ChangeMode(const Int2& size, bool fullscreen, int hz = 0);
	void DoPseudotick(bool msg_only = false);
	void EngineShutdown();
	void FatalError(cstring err);
	void ShowError(cstring msg, Logger::Level level = Logger::L_ERROR);
	bool Start(App* app, StartupOptions& options);
	void UnlockCursor(bool lock_on_focus = true);
	void LockCursor();

	bool IsActive() const { return active; }
	bool IsCursorLocked() const { return locked_cursor; }
	bool IsCursorVisible() const { return cursor_visible; }
	bool IsEngineShutdown() const { return engine_shutdown; }
	bool IsFullscreen() const { return fullscreen; }

	float GetFps() const { return fps; }
	float GetWindowAspect() const { return float(wnd_size.x) / wnd_size.y; }
	HWND GetWindowHandle() const { return hwnd; }
	const Int2& GetWindowSize() const { return wnd_size; }
	Render* GetRender() { return render.get(); }
	SoundManager* GetSoundManager() { return sound_mgr.get(); }
	CustomCollisionWorld* GetPhysicsWorld() { return phy_world; }

	void SetTitle(cstring title);
	void SetUnlockPoint(const Int2& pt) { unlock_point = pt; }

	CameraBase* cam_base;

private:
	void Init(StartupOptions& options);
	void AdjustWindowSize();
	void ChangeMode();
	void Cleanup();
	void DoTick(bool update_game);
	long HandleEvent(HWND hwnd, uint msg, uint wParam, long lParam);
	bool MsgToKey(uint msg, uint wParam, byte& key, int& result);
	void InitWindow(StartupOptions& options);
	void PlaceCursor();
	void ShowCursor(bool show);
	void UpdateActivity(bool is_active);
	void WindowLoop();
	bool IsWindowActive();
	void SetWindowSizeInternal(const Int2& size);

	static Engine* engine;
	App* app;
	CustomCollisionWorld* phy_world;
	std::unique_ptr<Render> render;
	std::unique_ptr<SoundManager> sound_mgr;
	HWND hwnd;
	Timer timer;
	Int2 wnd_size, real_size, unlock_point, activation_point;
	float frame_time, fps;
	uint frames;
	bool engine_shutdown, cursor_visible, replace_cursor, locked_cursor, lock_on_focus, active, fullscreen;
};
