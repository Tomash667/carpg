#pragma once

#include "GameTypeId.h"
#include "Overlay.h"
#include "Event.h"

#undef CreateWindow

class Button;
class Engine;
class GameType;
class GameTypeManager;
class TextBox;
class ListBox;
typedef void* GameTypeItem;

namespace gui
{
	class TabControl;
	class Window;
}

struct ToolsetItem
{
	struct Entity
	{
		GameTypeItem old, current;
		string* id;
		bool deattached;

		Entity(GameTypeItem old, GameTypeItem current = nullptr) : old(old), current(current)
		{
			deattached = (old == nullptr);
		}

		GameTypeItem GetItem()
		{
			if(current)
				return current;
			else
				return old;
		}
	};

	GameType& game_type;
	gui::Window* window;
	TextBox* box;
	ListBox* list_box;
	vector<Entity*> items;
	Button* bt_save, *bt_restore;

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

	void Event(GuiEvent e) override;
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
	bool HandleListBoxEvent(int action, int id);


	void Save();
	void Reload();
	void ExitToMenu();
	void Quit();

	bool AnyEntityChanges();
	bool SaveEntity();
	void RestoreEntity();
	bool ValidateEntity();
	void ForgetEntity();
	cstring GenerateEntityName(cstring name, bool dup);
	bool AnyUnsavedChanges();

	GameTypeManager& gt_mgr;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<GameTypeId, ToolsetItem*> toolset_items;
	gui::MenuStrip* tree_menu;
	ToolsetItem* current_toolset_item; // UPDATE
	ToolsetItem::Entity* current_entity;
	gui::MenuStrip* menu_strip;
	int context_index;

	enum class Closing
	{
		No,
		Yes,
		Shutdown
	};
	Closing closing;

	enum EventId
	{
		Event_Save = GuiEvent_Custom,
		Event_Restore
	};
};
