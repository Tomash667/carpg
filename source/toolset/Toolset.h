#pragma once

#include "TypeId.h"
#include "Overlay.h"
#include "Event.h"
#include "Type.h"

#undef CreateWindow

class Button;
class Engine;
class ListBox;
class TypeManager;
class TextBox;
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
	uint counter;

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
	bool HandleListBoxEvent(int action, int id);


	void Save();
	void MergeChanges(ToolsetItem* toolset_item);
	void Reload();
	void ExitToMenu();
	void Quit();

	bool AnyEntityChanges();
	bool SaveEntity();
	void RestoreEntity();
	bool ValidateEntity();
	cstring GenerateEntityName(cstring name, bool dup);
	bool AnyUnsavedChanges();
	void ApplyView(TypeEntity* entity);
	void UpdateCounter(int change);

	TypeManager& type_manager;
	Engine* engine;
	gui::TabControl* tab_ctrl;
	std::map<TypeId, ToolsetItem*> toolset_items;
	vector<TypeEntity*> to_merge;
	gui::MenuStrip* tree_menu;
	ToolsetItem* current_toolset_item; // UPDATE
	TypeEntity* current_entity;
	gui::MenuStrip* menu_strip;
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
