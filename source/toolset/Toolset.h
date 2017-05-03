#pragma once

#include "TypeId.h"
#include "Overlay.h"
#include "Event.h"
#include "Type.h"
#include "GuiElement.h"

#undef CreateWindow

class Button;
class Engine;
class ListBox;
class TextBox;
class Toolset;
class TypeManager;
struct TypeEntity;

namespace gui
{
	class CheckBoxGroup;
	class Label;
	class Panel;
	class TabControl;
	class TreeNode;
	class TreeView;
	class Window;
}

struct ListItem : public GuiElement
{
	TypeItem* item;
	string& id;

	ListItem(TypeItem* item, string& id, int value) : GuiElement(value), item(item), id(id) {}

	cstring ToString() override
	{
		return id.c_str();
	}
};

struct ToolsetItem
{
	struct Field
	{
		Type::Field* field;
		union
		{
			TextBox* text_box;
			gui::CheckBoxGroup* check_box_group;
			ListBox* list_box;
		};
	};

	Type& type;
	gui::Window* window;
	gui::TreeView* tree_view;
	gui::Panel* panel;
	vector<Field> fields;
	std::map<string, TypeEntity*> items;
	vector<TypeEntity*> removed_items;
	Button* bt_save, *bt_restore;
	gui::Label* label_counter;
	uint counter, changes;
	TypeEntity* current;
	Toolset& toolset;
	bool is_open;

	ToolsetItem(Toolset& toolset, Type& type) : toolset(toolset), type(type), changes(0) {}
	~ToolsetItem();

	bool AnyEntityChanges();
	void ApplyView(TypeEntity* entity);
	cstring GenerateEntityName(cstring name, bool dup);
	void RemoveEntity(gui::TreeNode* node);
	void RestoreEntity();
	bool SaveEntity();
	void UpdateEntityState(TypeEntity* entity);
	void UpdateCounter(int change);
	bool ValidateEntity();
	cstring ValidateEntityId(const string& id, TypeEntity* e);
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
	bool HandleTabControlEvent(int action, int id);
	bool HandleTreeViewEvent(int action, int id);
	void OpenPickMeshDialog(int field_index);

	void Save();
	void MergeChanges(ToolsetItem* toolset_item);
	void Reload();
	void ExitToMenu();
	void Quit();

	void RemoveEntity();
	bool AnyUnsavedChanges();

	TypeManager& type_manager;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<TypeId, ToolsetItem*> toolset_items;
	vector<TypeEntity*> to_merge;
	gui::MenuStrip* tree_menu;
	ToolsetItem* current;
	gui::TreeNode* clicked_node;
	string empty_item_id;
	bool adding_item;

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
