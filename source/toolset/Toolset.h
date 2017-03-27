#pragma once

#include "TypeId.h"
#include "Overlay.h"
#include "Event.h"
#include "Type.h"

#undef CreateWindow

class Button;
class Engine;
class TypeManager;
class TextBox;
class ListBox;
struct TypeEntity;

namespace gui
{
	class TabControl;
	class Window;
}

struct ToolsetItem
{
	Type& type;
	gui::Window* window;
	TextBox* box;
	ListBox* list_box;
	std::map<string, TypeEntity*> items;
	vector<TypeEntity*> removed_items;
	Button* bt_save, *bt_restore;

	ToolsetItem(Type& type) : type(type) {}
	/*~ToolsetItem()
	{
		delete window;
	}*/
};

class Toolset : public gui::Overlay
{
public:
	Toolset(TypeManager& type_manager);
	~Toolset();

	void Event(GuiEvent e) override;
	void Update(float dt) override;

	void Init(Engine* engine);
	void Start();
	void OnDraw();
	bool OnUpdate(float dt);

private:
	void HandleMenuEvent(int id);
	void ShowType(TypeId id);
	ToolsetItem* GetToolsetItem(TypeId id);
	ToolsetItem* CreateToolsetItem(TypeId id);
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
	cstring GenerateEntityName(cstring name, bool dup);
	bool AnyUnsavedChanges();

	TypeManager& type_manager;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<TypeId, ToolsetItem*> toolset_items;
	gui::MenuStrip* tree_menu;
	ToolsetItem* current_toolset_item; // UPDATE
	TypeEntity* current_entity;
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
