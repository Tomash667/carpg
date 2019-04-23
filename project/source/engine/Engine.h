#pragma once

//-----------------------------------------------------------------------------
#include "Timer.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
class Engine
{
	friend class Render;
public:
	Engine();
	virtual ~Engine();

	static Engine& Get() { return *engine; }

	bool ChangeMode(bool fullscreen);
	bool ChangeMode(const Int2& size, bool fullscreen, int hz = 0);
	void DoPseudotick(bool msg_only = false);
	void EngineShutdown();
	void FatalError(cstring err);
	void ShowError(cstring msg, Logger::Level level = Logger::L_ERROR);
	bool Start(StartupOptions& options);
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

	void SetTitle(cstring title);
	void SetUnlockPoint(const Int2& pt) { unlock_point = pt; }

	// ----- ZMIENNE -----
	// directx
	Color clear_color;

	CustomCollisionWorld* phy_world;
	std::unique_ptr<SoundManager> sound_mgr;

	// constants
	static const Int2 MIN_WINDOW_SIZE;
	static const Int2 DEFAULT_WINDOW_SIZE;

	CameraBase* cam_base;

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
	uint frames;
	float frame_time;
	Timer timer;
	bool engine_shutdown, cursor_visible, replace_cursor, locked_cursor, lock_on_focus;

	// window
	HWND hwnd;
	Int2 wnd_size, real_size, unlock_point, activation_point;
	float fps;
	bool active, fullscreen;

	std::unique_ptr<Render> render;
};
