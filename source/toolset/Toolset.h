#pragma once

#include "GameTypeId.h"
#include "Overlay.h"

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

	GameTypeManager& gt_mgr;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<GameTypeId, gui::Window*> wnds;

	enum class Closing
	{
		No,
		Yes,
		Shutdown
	};
	Closing closing;
};
