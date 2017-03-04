#pragma once

#include "GameTypeId.h"
#include "Overlay.h"
#include "Event.h"

#undef CreateWindow

class Engine;
class GameTypeManager;

namespace gui
{
	class TabControl;
	class Window;
}

class Toolset : public gui::Overlay
{
public:
	Toolset(GameTypeManager& gt_mgr);
	~Toolset();

	void Update(float dt) override;

	void Init(Engine* engine);
	void Start();
	void OnDraw();
	bool OnUpdate(float dt);

private:
	void HandleMenuEvent(int id);
	void ShowGameType(GameTypeId id);
	gui::Window* GetWindow(GameTypeId id);
	gui::Window* CreateWindow(GameTypeId id);
	void HandleTreeViewKeyEvent(gui::KeyEventData& e);
	void HandleTreeViewMouseEvent(gui::MouseEventData& e);
	void HandleTreeViewMenuEvent(int id);

	GameTypeManager& gt_mgr;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<GameTypeId, gui::Window*> wnds;
	gui::MenuStrip* tree_menu;

	enum class Closing
	{
		No,
		Yes,
		Shutdown
	};
	Closing closing;
};
