#pragma once

#include "GameTypeId.h"
#include "Overlay.h"
#include "Event.h"

#undef CreateWindow

class Engine;
class GameType;
class GameTypeManager;
class TextBox;
class ListBox;

namespace gui
{
	class TabControl;
	class Window;
}

struct ToolsetItem
{
	GameType& game_type;
	gui::Window* window;
	TextBox* box;
	ListBox* list_box;
	bool unsaved;

	ToolsetItem(GameType& game_type) : game_type(game_type) {}
	/*~ToolsetItem()
	{
		delete window;
	}*/
};

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
	ToolsetItem* GetToolsetItem(GameTypeId id);
	ToolsetItem* CreateToolsetItem(GameTypeId id);
	void HandleTreeViewKeyEvent(gui::KeyEventData& e);
	void HandleTreeViewMouseEvent(gui::MouseEventData& e);
	void HandleTreeViewMenuEvent(int id);
	void HandleListBoxEvent(int action, int id);


	void Save();
	void Reload();
	void ExitToMenu();
	void Quit();

	GameTypeManager& gt_mgr;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<GameTypeId, ToolsetItem*> toolset_items;
	gui::MenuStrip* tree_menu;
	ToolsetItem* current_toolset_item; // UPDATE
	bool unsaved_changes;
	gui::MenuStrip* menu_strip;

	enum class Closing
	{
		No,
		Yes,
		Shutdown
	};
	Closing closing;
};
