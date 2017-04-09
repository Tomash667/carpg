#include "Pch.h"
#include "Base.h"
#include "Toolset.h"
#include "Engine.h"
#include "Gui2.h"
#include "Overlay.h"
#include "Window.h"
#include "MenuBar.h"
#include "ToolStrip.h"
#include "TabControl.h"
#include "TreeView.h"
#include "TypeManager.h"
#include "ListBox.h"
#include "Label.h"
#include "Button.h"
#include "TextBox.h"
#include "TypeProxy.h"
#include "Dialog2.h"
#include "CheckBoxGroup.h"
#include "MenuList.h"

using namespace gui;

enum MenuAction
{
	MA_NEW,
	MA_LOAD,
	MA_SAVE,
	MA_SAVE_AS,
	MA_SETTINGS,
	MA_RELOAD,
	MA_EXIT_TO_MENU,
	MA_QUIT,
	MA_BUILDING_GROUPS,
	MA_BUILDINGS,
	MA_BUILDING_SCRIPTS,
	MA_ABOUT
};

enum ListAction
{
	A_NONE,
	A_ADD,
	A_ADD_DIR,
	A_DUPLICATE,
	A_RENAME,
	A_REMOVE
};

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

Toolset::Toolset(TypeManager& type_manager) : engine(nullptr), type_manager(type_manager), empty_item_id("(none)"), adding_item(false)
{

}

Toolset::~Toolset()
{
	for(auto& toolset_item : toolset_items)
		delete toolset_item.second;
	delete tree_menu;
}

void Toolset::Init(Engine* _engine)
{
	engine = _engine;
	
	Window* window = new Window(true);

	MenuBar* menu = new MenuBar;
	menu->SetHandler(delegate<void(int)>(this, &Toolset::HandleMenuEvent));
	menu->AddMenu("Module", {
		//{"New", MA_NEW},
		//{"Load", MA_LOAD},
		{"Save", MA_SAVE},
		//{"Save As", MA_SAVE_AS},
		//{"Settings", MA_SETTINGS},
		//{"Reload", MA_RELOAD},
		{"Exit to menu", MA_EXIT_TO_MENU},
		{"Quit", MA_QUIT}
	});
	menu->AddMenu("Buildings", {
		{"Building groups", MA_BUILDING_GROUPS},
		{"Buildings", MA_BUILDINGS},
		//{"Building scripts", MA_BUILDING_SCRIPTS}
	});
	menu->AddMenu("Info", {
		{"About", MA_ABOUT}
	});

	tab_ctrl = new TabControl(false);
	tab_ctrl->SetDocked(true);

	window->SetMenu(menu);
	window->Add(tab_ctrl);

	window->Initialize();

	Add(window);
	GUI.Add(this);

	vector<SimpleMenuCtor> menu_strip_items = {
		{ "Add", A_ADD },
		{ "Add dir", A_ADD_DIR },
		{ "Duplicate", A_DUPLICATE },
		{ "Rename", A_RENAME },
		{ "Remove", A_REMOVE }
	};
	menu_strip = new MenuStrip(menu_strip_items);
}

void Toolset::Start()
{
	GUI.SetOverlay(this);
	Show();
	closing = Closing::No;
}

void Toolset::OnDraw()
{
	GUI.Draw(engine->wnd_size);
}

bool Toolset::OnUpdate(float dt)
{
	if(closing != Closing::No)
	{
		tab_ctrl->Clear();
		GUI.SetOverlay(nullptr);
		Hide();
		if(closing == Closing::Shutdown)
			engine->EngineShutdown();
		return false;
	}

	return true;
}

void Toolset::Event(GuiEvent event)
{
	switch(event)
	{
	case Event_Save:
		SaveEntity();
		break;
	case Event_Restore:
		RestoreEntity();
		break;
	default:
		Overlay::Event(event);
		break;
	}
}

void Toolset::Update(float dt)
{
	Overlay::Update(dt);

	auto tab = tab_ctrl->GetCurrentTab();
	if(tab)
	{
		bool have_changes = AnyEntityChanges();
		tab->SetHaveChanges(have_changes);
		current_toolset_item->bt_save->SetDisabled(!have_changes);
		current_toolset_item->bt_restore->SetDisabled(!have_changes);
	}

	if(Key.PressedRelease(VK_ESCAPE))
	{
		DialogInfo dialog;
		dialog.text = "Do you really want to exit?";
		dialog.parent = this;
		dialog.type = DIALOG_YESNO;
		dialog.event = [this](int id) { if(id == BUTTON_YES) closing = Closing::Yes; };
		GUI.ShowDialog(dialog);
	}
}

void Toolset::HandleMenuEvent(int id)
{
	switch(id)
	{
	case MA_SAVE:
		Save();
		break;
	case MA_RELOAD:
		Reload();
		break;
	case MA_EXIT_TO_MENU:
		ExitToMenu();
		break;
	case MA_QUIT:
		Quit();
		break;
	case MA_BUILDING_GROUPS:
		ShowType(TypeId::BuildingGroup);
		break;
	case MA_BUILDINGS:
		ShowType(TypeId::Building);
		break;
	case MA_BUILDING_SCRIPTS:
		ShowType(TypeId::BuildingScript);
		break;
	case MA_ABOUT:
		SimpleDialog("TOOLSET\n=========\nNothing interesting here yet...");
		break;
	}
}

void Toolset::Save()
{
	if(!SaveEntity())
		return;

	MergeChanges(current_toolset_item);

	if(!type_manager.Save())
		SimpleDialog("Failed to save all changes. Check LOG for details.");
}

void Toolset::MergeChanges(ToolsetItem* toolset_item)
{
	to_merge.clear();
	toolset_item->tree_view->RecalculatePath();
	for(auto node : toolset_item->tree_view->ForEachNotDir())
	{
		TypeEntity* e = node->GetData<TypeEntity>();
		e->item->toolset_path = node->GetPath();
		to_merge.push_back(e);
	}

	toolset_item->type.GetContainer()->Merge(to_merge, toolset_item->removed_items);
}

void Toolset::Reload()
{

}

void Toolset::ExitToMenu()
{
	if(AnyUnsavedChanges())
	{
		DialogInfo dialog;
		dialog.type = DIALOG_YESNO;
		dialog.text = "Discard unsaved changes and exit to menu?";
		dialog.event = [this](int id) { if(id == BUTTON_YES) closing = Closing::Yes; };
		GUI.ShowDialog(dialog);
		return;
	}

	closing = Closing::Yes;
}

void Toolset::Quit()
{
	if(AnyUnsavedChanges())
	{
		DialogInfo dialog;
		dialog.type = DIALOG_YESNO;
		dialog.text = "Discard unsaved changes and quit?";
		dialog.event = [this](int id) { if(id == BUTTON_YES) closing = Closing::Shutdown; };
		GUI.ShowDialog(dialog);
		return;
	}

	closing = Closing::Shutdown;
}

void Toolset::ShowType(TypeId type_id)
{
	Type& type = type_manager.GetType(type_id);
	TabControl::Tab* tab = tab_ctrl->Find(type.GetToken().c_str());
	if(tab)
	{
		tab->Select();
		return;
	}

	tab_ctrl->AddTab(type.GetToken().c_str(), type.GetName().c_str(), GetToolsetItem(type_id)->window);
}

ToolsetItem* Toolset::GetToolsetItem(TypeId type_id)
{
	auto it = toolset_items.find(type_id);
	if(it != toolset_items.end())
		return it->second;
	ToolsetItem* toolset_item = CreateToolsetItem(type_id);
	toolset_items[type_id] = toolset_item;
	return toolset_item;
}

ToolsetItem* Toolset::CreateToolsetItem(TypeId type_id)
{
	ToolsetItem* toolset_item = new ToolsetItem(type_manager.GetType(type_id));
	toolset_item->window = new Window;


	Type& type = toolset_item->type;
	Type::Container* container = type.GetContainer();
	toolset_item->counter = container->Count();

	for(auto e : container->ForEach())
	{
		auto e_dup = type.Duplicate(e);
		auto new_e = new TypeEntity(e_dup, e, TypeEntity::UNCHANGED, type.GetItemId(e_dup));
		toolset_item->items[new_e->id] = new_e;
	}

	Window* w = toolset_item->window;

	Label* label = new Label(Format("%s (%u):", type.GetName().c_str(), container->Count()));
	label->SetPosition(INT2(5, 5));
	toolset_item->label_counter = label;
	w->Add(label);

	TreeView* tree_view = new TreeView;
	tree_view->SetPosition(INT2(5, 30));
	tree_view->SetSize(INT2(250, 500));
	tree_view->SetMenu(menu_strip);
	tree_view->SetHandler(DialogEvent2(this, &Toolset::HandleListBoxEvent));
	tree_view->SetText(Format("All %ss", type.GetName().c_str()));
	for(auto& e : toolset_item->items)
	{
		TreeNode* node = new TreeNode;
		node->SetText(e.second->id);
		node->SetData(e.second);
		tree_view->Add(node, e.second->item->toolset_path, false);
	}
	w->Add(tree_view);

	Button* bt = new Button;
	bt->text = "Save";
	bt->id = Event_Save;
	bt->SetPosition(INT2(5, 535));
	bt->SetSize(INT2(100, 30));
	w->Add(bt);
	toolset_item->bt_save = bt;

	bt = new Button;
	bt->text = "Restore";
	bt->id = Event_Restore;
	bt->SetPosition(INT2(110, 535));
	bt->SetSize(INT2(100, 30));
	w->Add(bt);
	toolset_item->bt_restore = bt;

	Panel* panel = new Panel;
	panel->custom_color = 0;
	panel->use_custom_color = true;
	panel->SetPosition(INT2(260, 5));
	panel->SetSize(tab_ctrl->GetAreaSize() - INT2(tree_view->GetSize().x, 0));

	int offset = 0;

	for(auto field : type.fields)
	{
		label = new Label(Format("%s:", field->GetFriendlyName().c_str()));
		label->SetPosition(INT2(0, offset));
		panel->Add(label);
		offset += 20;

		switch(field->type)
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			{
				TextBox* text_box = new TextBox(false, true);
				text_box->SetPosition(INT2(0, offset));
				text_box->SetSize(INT2(300, 30));
				panel->Add(text_box);

				ToolsetItem::Field f;
				f.field = field;
				f.text_box = text_box;
				toolset_item->fields.push_back(f);
				offset += 35;
			}
			break;
		case Type::Field::FLAGS:
			{
				auto cbg = new CheckBoxGroup;
				cbg->SetPosition(INT2(0, offset));
				cbg->SetSize(INT2(300, 100));
				for(auto& flag : *field->flags)
					cbg->Add(flag.name.c_str(), flag.value);
				panel->Add(cbg);

				ToolsetItem::Field f;
				f.field = field;
				f.check_box_group = cbg;
				toolset_item->fields.push_back(f);
				offset += 105;
			}
			break;
		case Type::Field::REFERENCE:
			{
				auto& ref_type = type_manager.GetType(field->type_id);
				auto list_box = new ListBox(true);
				list_box->Add(new ListItem(nullptr, empty_item_id, 0));
				int index = 1;
				for(auto e : ref_type.GetContainer()->ForEach())
				{
					list_box->Add(new ListItem(e, ref_type.GetItemId(e), index));
					++index;
				}
				list_box->SetPosition(INT2(0, offset));
				list_box->SetSize(INT2(300, 30));
				list_box->Init(true);
				panel->Add(list_box);

				ToolsetItem::Field f;
				f.field = field;
				f.list_box = list_box;
				toolset_item->fields.push_back(f);
				offset += 35;
			}
			break;
		}
	}
	
	w->Add(panel);
	w->SetDocked(true);
	w->SetEventProxy(this);
	w->Initialize();

	toolset_item->tree_view = tree_view;
	// TMP
	tree_view->ExpandAll();
	toolset_item->panel = panel;
	current_toolset_item = toolset_item;
	panel->visible = false;

	return toolset_item;
}

bool Toolset::HandleListBoxEvent(int action, int id)
{
	switch(action)
	{
	case TreeView::A_BEFORE_CURRENT_CHANGE:
		if(!SaveEntity())
			return false;
		break;
	case TreeView::A_CURRENT_CHANGED:
		{
			auto node = (TreeNode*)id;
			if(node == nullptr || node->IsDir())
			{
				current_toolset_item->panel->visible = false;
				current_entity = nullptr;
			}
			else
			{
				current_toolset_item->panel->visible = true;
				auto e = node->GetData<TypeEntity>();
				ApplyView(e);
				if(adding_item)
				{
					adding_item = false;
					current_toolset_item->fields[0].text_box->SelectAll();
				}
				current_entity = e;
			}
			clicked_node = node;
		}
		break;
	case TreeView::A_BEFORE_MENU_SHOW:
		{
			// click on nothing, don't show menu
			auto node = (TreeNode*)id;
			if(node == nullptr)
				return false;

			// if failed to save, don't show menu
			if(!SaveEntity())
				return false;

			// set what to allow in menu
			const bool is_dir = node->IsDir();
			const bool is_root = node->IsRoot();
			const bool is_single = !current_toolset_item->tree_view->IsMultipleNodesSelected();
			menu_strip->FindItem(A_ADD)->SetEnabled(is_dir && is_single);
			menu_strip->FindItem(A_ADD_DIR)->SetEnabled(is_dir && is_single);
			menu_strip->FindItem(A_DUPLICATE)->SetEnabled(!is_dir && is_single);
			menu_strip->FindItem(A_REMOVE)->SetEnabled(!is_root && !current_toolset_item->tree_view->IsSelected());
			menu_strip->FindItem(A_RENAME)->SetEnabled(!is_root && is_single);
			clicked_node = node;
		}
		break;
	case TreeView::A_MENU:
		switch(id)
		{
		case A_ADD:
			{
				// create new item
				auto new_item = current_toolset_item->type.Create();
				auto& item_id = current_toolset_item->type.GetItemId(new_item);
				item_id = GenerateEntityName(Format("new %s", current_toolset_item->type.GetToken().c_str()), false);
				auto new_e = new TypeEntity(new_item, nullptr, TypeEntity::NEW, item_id);
				current_toolset_item->items[item_id] = new_e;
				adding_item = true;

				// create new tree node
				auto node = new TreeNode;
				node->SetData(new_e);
				node->SetText(new_e->id);
				clicked_node->AddChild(node);
				node->Select();
				UpdateCounter(+1);
			}
			break;
		case A_ADD_DIR:
			{
				auto node = new TreeNode(true);
				clicked_node->GenerateDirName(node, "new dir");
				clicked_node->AddChild(node);
				node->EditName();
			}
			break;
		case A_DUPLICATE:
			{
				// create new item
				auto new_item = current_toolset_item->type.Duplicate(current_entity->item);
				auto& item_id = current_toolset_item->type.GetItemId(new_item);
				item_id = GenerateEntityName(current_entity->id.c_str(), true);
				auto new_e = new TypeEntity(new_item, nullptr, TypeEntity::NEW, item_id);
				current_toolset_item->items[item_id] = new_e;

				// insert
				auto node = new TreeNode;
				node->SetText(item_id);
				node->SetData(new_e);
				clicked_node->GetParent()->AddChild(node);
				node->Select();
				UpdateCounter(+1);
			}
			break;
		case A_REMOVE:
			RemoveEntity();
			break;
		case A_RENAME:
			clicked_node->EditName();
			break;
		}
		break;
	case TreeView::A_BEFORE_RENAME:
		{
			auto node = (TreeNode*)id;
			if(!node->IsDir())
			{
				string& new_name = current_toolset_item->tree_view->GetNewName();
				if(current_toolset_item->type.GetContainer()->Find(new_name))
				{
					SimpleDialog("Id must be unique.");
					return false;
				}
			}
		}
		break;
	case TreeView::A_RENAMED:
		{
			auto node = (TreeNode*)id;
			current_toolset_item->items.erase(current_entity->id);
			current_entity->id = node->GetText();
			current_toolset_item->items[node->GetText()] = current_entity;
			current_toolset_item->fields[0].text_box->SetText(node->GetText().c_str());
		}
		break;
	case TreeView::A_SHORTCUT:
		{
			if(id != TreeView::S_REMOVE && current_toolset_item->tree_view->IsMultipleNodesSelected())
				break;
			
			auto current = current_toolset_item->tree_view->GetCurrentNode();
			ListAction action = A_NONE;
			switch(id)
			{
			case TreeView::S_ADD:
				if(current->IsDir())
					action = A_ADD;
				break;
			case TreeView::S_ADD_DIR:
				if(current->IsDir())
					action = A_ADD_DIR;
				break;
			case TreeView::S_DUPLICATE:
				action = A_DUPLICATE;
				break;
			case TreeView::S_REMOVE:
				action = A_REMOVE;
				break;
			case TreeView::S_RENAME:
				action = A_RENAME;
				break;
			default:
				assert(0);
				break;
			}
			HandleListBoxEvent(TreeView::A_MENU, action);
		}
		break;
	}

	return true;
}

bool Toolset::AnyEntityChanges()
{
	if(!current_entity)
		return false;

	if(current_entity->state == TypeEntity::NEW)
		return true;

	for(auto& field : current_toolset_item->fields)
	{
		switch(field.field->type)
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			if(field.text_box->GetText() != offset_cast<string>(current_entity->item, field.field->offset))
				return true;
			break;
		case Type::Field::FLAGS:
			if(field.check_box_group->GetValue() != offset_cast<int>(current_entity->item, field.field->offset))
				return true;
			break;
		case Type::Field::REFERENCE:
			{
				ListItem* item = field.list_box->GetItemCast<ListItem>();
				if(item->item != offset_cast<TypeItem*>(current_entity->item, field.field->offset))
					return true;
			}
			break;
		}
	}

	return false;
}

bool Toolset::SaveEntity()
{
	if(!AnyEntityChanges())
		return true;

	if(!ValidateEntity())
		return false;
	
	TypeProxy proxy(current_toolset_item->type, current_entity->item);

	const string& new_id = current_toolset_item->fields[0].text_box->GetText();
	if(new_id != current_entity->id)
	{
		current_toolset_item->items.erase(current_entity->id);
		current_entity->id = new_id;
		current_toolset_item->items[new_id] = current_entity;
		clicked_node->SetText(new_id);
	}

	for(uint i = 1, count = current_toolset_item->fields.size(); i < count; ++i)
	{
		auto& field = current_toolset_item->fields[i];
		switch(field.field->type)
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			offset_cast<string>(current_entity->item, field.field->offset) = field.text_box->GetText();
			break;
		case Type::Field::FLAGS:
			offset_cast<int>(current_entity->item, field.field->offset) = field.check_box_group->GetValue();
			break;
		case Type::Field::REFERENCE:
			{
				ListItem* item = field.list_box->GetItemCast<ListItem>();
				offset_cast<TypeItem*>(current_entity->item, field.field->offset) = item->item;
			}
			break;
		}
	}

	switch(current_entity->state)
	{
	case TypeEntity::UNCHANGED:
	case TypeEntity::CHANGED:
		{
			bool equal = current_toolset_item->type.Compare(current_entity->item, current_entity->old);
			if(equal)
				current_entity->state = TypeEntity::UNCHANGED;
			else
				current_entity->state = TypeEntity::CHANGED;
		}
		break;
	case TypeEntity::NEW:
		current_entity->state = TypeEntity::NEW_ATTACHED;
		break;
	case TypeEntity::NEW_ATTACHED:
		break;
	}

	return true;
}

void Toolset::RestoreEntity()
{
	if(!AnyEntityChanges())
		return;

	if(current_entity->state == TypeEntity::NEW)
	{
		current_toolset_item->items.erase(current_entity->id);
		current_toolset_item->type.Destroy(current_entity->item);
		delete current_entity;
		current_entity = nullptr;
		current_toolset_item->tree_view->GetCurrentNode()->Remove();
		UpdateCounter(-1);
		return;
	}

	ApplyView(current_entity);
}

void Toolset::RemoveEntity()
{
	// can't delete if root selected
	if(current_toolset_item->tree_view->IsSelected())
		return;

	cstring msg;
	if(current_toolset_item->tree_view->IsMultipleNodesSelected())
		msg = "Do you really want to remove multiple items/dirs?";
	else if(clicked_node->IsDir())
	{
		if(clicked_node->IsEmpty())
			msg = "Do you really want to remove dir '%s'?";
		else
			msg = "Do you really want to remove dir '%s' and all child items?";
	}
	else
		msg = "Do you really want to remove item '%s'?";

	DialogInfo dialog;
	dialog.text = Format(msg, clicked_node->GetText().c_str());
	dialog.type = DIALOG_YESNO;
	dialog.parent = this;
	dialog.event = [this](int id) {
		if(id == BUTTON_YES)
		{
			for(auto node : current_toolset_item->tree_view->GetSelectedNodes())
				RemoveEntity(node);
			current_toolset_item->tree_view->RemoveSelected();
		}
	};
	GUI.ShowDialog(dialog);
}

void Toolset::RemoveEntity(gui::TreeNode* node)
{
	if(node->IsDir())
	{
		for(auto child : node->GetChilds())
			RemoveEntity(child);
	}
	else
	{
		auto entity = node->GetData<TypeEntity>();
		if(entity)
		{
			current_toolset_item->items.erase(entity->id);
			if(entity->state == TypeEntity::NEW || entity->state == TypeEntity::NEW_ATTACHED)
				delete entity;
			else
				current_toolset_item->removed_items.push_back(entity);
			UpdateCounter(-1);
			node->SetData(nullptr);
		}
	}
}

bool Toolset::ValidateEntity()
{
	cstring err_msg = nullptr;
	const string& new_id = current_toolset_item->fields[0].text_box->GetText();
	if(new_id.length() < 1)
		err_msg = "Id must not be empty.";
	else if(new_id.length() > 100)
		err_msg = "Id max length is 100.";
	else
	{
		auto it = current_toolset_item->items.find(new_id);
		if(it != current_toolset_item->items.end() && it->second != current_entity)
			err_msg = "Id must be unique.";
	}

	if(err_msg)
	{
		GUI.SimpleDialog(Format("Validation failed\n----------------------\n%s", err_msg), this);
		return false;
	}
	else
		return true;
}

cstring Toolset::GenerateEntityName(cstring name, bool dup)
{
	string old_name = name;
	int index = 1;

	if(!dup)
	{
		auto it = current_toolset_item->items.find(old_name);
		if(it == current_toolset_item->items.end())
			return name;
	}
	else
	{
		cstring pos = strrchr(name, '(');
		if(pos)
		{
			int new_index;
			if(sscanf(pos, "(%d)", &new_index) == 1)
			{
				index = new_index + 1;
				if(name != pos)
					old_name = string(name, pos - name - 1);
				else
					old_name = string(name, pos - name);
			}
		}
	}

	string new_name;
	while(true)
	{
		new_name = Format("%s (%d)", old_name.c_str(), index);
		auto it = current_toolset_item->items.find(new_name);
		if(it == current_toolset_item->items.end())
			return Format("%s", new_name.c_str());
		++index;
	}
}

bool Toolset::AnyUnsavedChanges()
{
	return AnyEntityChanges();
}

void Toolset::ApplyView(TypeEntity* entity)
{
	for(auto& field : current_toolset_item->fields)
	{
		switch(field.field->type)
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			field.text_box->SetText(offset_cast<string>(entity->item, field.field->offset).c_str());
			break;
		case Type::Field::FLAGS:
			field.check_box_group->SetValue(offset_cast<int>(entity->item, field.field->offset));
			break;
		case Type::Field::REFERENCE:
			{
				TypeItem* ref_item = offset_cast<TypeItem*>(entity->item, field.field->offset);
				if(ref_item == nullptr)
					field.list_box->Select(0);
				else
					field.list_box->Select([ref_item](GuiElement* ge)
				{
					ListItem* item = (ListItem*)ge;
					return item->item == ref_item;
				});
			}
			break;
		}
	}
}

void Toolset::UpdateCounter(int change)
{
	current_toolset_item->counter += change;
	current_toolset_item->label_counter->SetText(Format("%s (%u)", current_toolset_item->type.GetName().c_str(), current_toolset_item->counter));
}
