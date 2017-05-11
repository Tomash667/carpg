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
#include "PickFileDialog.h"

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

ToolsetItem::~ToolsetItem()
{
	delete window;
	for(auto& m : items)
		m.second->Remove(type);
	for(auto e : removed_items)
		e->Remove(type);
}

Toolset::Toolset(TypeManager& type_manager) : engine(nullptr), type_manager(type_manager), empty_item_id("(none)"), adding_item(false), current(nullptr)
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
	
	Window* window = new Window(true, true);

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
	tab_ctrl->SetHandler(TabControl::Handler(this, &Toolset::HandleTabControlEvent));

	window->SetMenu(menu);
	window->Add(tab_ctrl);

	window->Initialize();

	Add(window);
	GUI.Add(this);

	vector<SimpleMenuCtor> tree_menu_items = {
		{ "Add", A_ADD },
		{ "Add dir", A_ADD_DIR },
		{ "Duplicate", A_DUPLICATE },
		{ "Rename", A_RENAME },
		{ "Remove", A_REMOVE }
	};
	tree_menu = new MenuStrip(tree_menu_items);
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
		current->SaveEntity();
		break;
	case Event_Restore:
		current->RestoreEntity();
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
		bool have_changes = current->AnyEntityChanges();
		tab->SetHaveChanges(have_changes);
		current->bt_save->SetDisabled(!have_changes);
		current->bt_restore->SetDisabled(!have_changes);
	}

	if(Key.PressedRelease(VK_ESCAPE))
	{
		DialogInfo dialog;
		if(AnyUnsavedChanges())
			dialog.text = "Do you really want to discard any unsaved changes and exit to menu?";
		else
			dialog.text = "Do you really want to exit to menu?";
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
	// validate pending changes
	for(auto& m : toolset_items)
	{
		ToolsetItem* item = m.second;
		if(item->is_open)
		{
			if(!item->SaveEntity())
			{
				ShowType(item->type.GetTypeId());
				return;
			}
			else
				tab_ctrl->Find(item->type.GetToken().c_str())->SetHaveChanges(false);
		}
	}

	// merge changes
	for(auto& m : toolset_items)
		MergeChanges(m.second);

	// save
	if(!type_manager.Save())
		SimpleDialog("Failed to save all changes. Check LOG for details.");
}

void Toolset::MergeChanges(ToolsetItem* toolset_item)
{
	if(toolset_item->changes == 0)
		return;

	to_merge.clear();
	for(auto node : toolset_item->tree_view->ForEachNotDir())
	{
		TypeEntity* e = node->GetData<TypeEntity>();
		e->item->toolset_path = node->GetPath();
		to_merge.push_back(e);
	}

	toolset_item->type.GetContainer()->Merge(to_merge, toolset_item->removed_items);
	toolset_item->type.changes = true;
	toolset_item->changes = 0;
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

	ToolsetItem* item = GetToolsetItem(type_id);
	item->is_open = true;
	tab_ctrl->AddTab(type.GetToken().c_str(), type.GetName().c_str(), item->window);
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
	ToolsetItem* toolset_item = new ToolsetItem(*this, type_manager.GetType(type_id));
	toolset_item->window = new Window(false, true);

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
	tree_view->SetMenu(tree_menu);
	tree_view->SetHandler(DialogEvent2(this, &Toolset::HandleTreeViewEvent));
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

	int offset = 0, index = 0;

	for(auto field : type.fields)
	{
		label = new Label(Format("%s:", field->GetFriendlyName().c_str()));
		label->SetPosition(INT2(0, offset));
		panel->Add(label);
		offset += 20;

		switch(field->type)
		{
		case Type::Field::STRING:
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
		case Type::Field::MESH:
			{
				TextBox* text_box = new TextBox(false, true);
				text_box->SetPosition(INT2(0, offset));
				text_box->SetSize(INT2(300, 30));
				panel->Add(text_box);

				Button* bt = new Button;
				bt->text = ">>>";
				bt->id = index;
				bt->SetPosition(INT2(310, offset));
				bt->SetSize(INT2(50, 30));
				bt->SetHandler(DialogEvent(this, &Toolset::OpenPickMeshDialog));
				panel->Add(bt);

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

		++index;
	}
	
	w->Add(panel);
	w->SetDocked(true);
	w->SetEventProxy(this);
	w->Initialize();

	toolset_item->tree_view = tree_view;
	toolset_item->panel = panel;
	panel->visible = false;

	return toolset_item;
}

bool Toolset::HandleTabControlEvent(int action, int id)
{
	TabControl::Tab* tab = (TabControl::Tab*)id;
	switch(action)
	{
	case TabControl::A_BEFORE_CHANGE:
		break;
	case TabControl::A_CHANGED:
		{
			const string& id = tab->GetId();
			ToolsetItem* item = nullptr;
			for(auto& m : toolset_items)
			{
				if(m.second->type.GetToken() == id)
				{
					item = m.second;
					break;
				}
			}
			assert(item);
			current = item;
			auto node = item->tree_view->GetCurrentNode();
			current->current = node ? node->GetData<TypeEntity>() : nullptr;
		}
		break;
	case TabControl::A_BEFORE_CLOSE:
		{
			TabControl::Tab* prev = tab_ctrl->GetCurrentTab();
			if(prev != tab)
				tab_ctrl->Select(tab, false);
			if(current->SaveEntity())
			{
				current->is_open = false;
				if(prev != tab)
					tab_ctrl->Select(prev, false);
				return true;
			}
			else
				return false;
		}
		break;
	}

	return true;
}

bool Toolset::HandleTreeViewEvent(int action, int id)
{
	switch(action)
	{
	case TreeView::A_BEFORE_CURRENT_CHANGE:
		if(!current->SaveEntity())
			return false;
		break;
	case TreeView::A_CURRENT_CHANGED:
		{
			auto node = (TreeNode*)id;
			if(node == nullptr || node->IsDir())
			{
				current->panel->visible = false;
				current->current = nullptr;
			}
			else
			{
				current->panel->visible = true;
				auto e = node->GetData<TypeEntity>();
				current->ApplyView(e);
				if(adding_item)
				{
					adding_item = false;
					current->fields[0].text_box->SelectAll();
				}
				current->current = e;
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
			if(!current->SaveEntity())
				return false;

			// set what to allow in menu
			const bool is_dir = node->IsDir();
			const bool is_root = node->IsRoot();
			const bool is_single = !current->tree_view->IsMultipleNodesSelected();
			tree_menu->FindItem(A_ADD)->SetEnabled(is_dir && is_single);
			tree_menu->FindItem(A_ADD_DIR)->SetEnabled(is_dir && is_single);
			tree_menu->FindItem(A_DUPLICATE)->SetEnabled(!is_dir && is_single);
			tree_menu->FindItem(A_REMOVE)->SetEnabled(!is_root && !current->tree_view->IsSelected());
			tree_menu->FindItem(A_RENAME)->SetEnabled(!is_root && is_single);
			clicked_node = node;
		}
		break;
	case TreeView::A_MENU:
		switch(id)
		{
		case A_ADD:
			{
				// create new item
				auto new_item = current->type.Create();
				auto& item_id = current->type.GetItemId(new_item);
				item_id = current->GenerateEntityName(Format("new %s", current->type.GetToken().c_str()), false);
				auto new_e = new TypeEntity(new_item, nullptr, TypeEntity::NEW, item_id);
				current->items[item_id] = new_e;
				adding_item = true;

				// create new tree node
				auto node = new TreeNode;
				node->SetData(new_e);
				node->SetText(new_e->id);
				clicked_node->AddChild(node);
				node->Select();
				current->UpdateCounter(+1);
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
				auto new_item = current->type.Duplicate(current->current->item);
				auto& item_id = current->type.GetItemId(new_item);
				item_id = current->GenerateEntityName(current->current->id.c_str(), true);
				auto new_e = new TypeEntity(new_item, nullptr, TypeEntity::NEW, item_id);
				current->items[item_id] = new_e;

				// insert
				auto node = new TreeNode;
				node->SetText(item_id);
				node->SetData(new_e);
				clicked_node->GetParent()->AddChild(node);
				node->Select();
				current->UpdateCounter(+1);
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
				cstring err_msg = current->ValidateEntityId(current->tree_view->GetNewName(), node->GetData<TypeEntity>());
				if(err_msg)
				{
					SimpleDialog(err_msg);
					return false;
				}
			}
		}
		break;
	case TreeView::A_RENAMED:
		{
			auto node = (TreeNode*)id;
			if(!node->IsDir())
			{
				current->items.erase(current->current->id);
				current->current->id = node->GetText();
				current->items[node->GetText()] = current->current;
				current->fields[0].text_box->SetText(node->GetText().c_str());
			}
		}
		break;
	case TreeView::A_SHORTCUT:
		{
			if(id != TreeView::S_REMOVE && current->tree_view->IsMultipleNodesSelected())
				break;
			
			auto entity = current->tree_view->GetCurrentNode();
			ListAction action = A_NONE;
			switch(id)
			{
			case TreeView::S_ADD:
				if(entity->IsDir())
					action = A_ADD;
				break;
			case TreeView::S_ADD_DIR:
				if(entity->IsDir())
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
			HandleTreeViewEvent(TreeView::A_MENU, action);
		}
		break;
	case TreeView::A_PATH_CHANGED:
		{
			auto node = (TreeNode*)id;
			if(!node->IsDir())
			{
				auto e = node->GetData<TypeEntity>();
				e->item->toolset_path = node->GetPath();
				current->UpdateEntityState(e);
			}
		}
		break;
	}

	return true;
}

void Toolset::RemoveEntity()
{
	// can't delete if root selected
	if(current->tree_view->IsSelected())
		return;

	cstring msg;
	if(current->tree_view->IsMultipleNodesSelected())
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
			for(auto node : current->tree_view->GetSelectedNodes())
				current->RemoveEntity(node);
			current->tree_view->RemoveSelected();
		}
	};
	GUI.ShowDialog(dialog);
}

bool Toolset::AnyUnsavedChanges()
{
	for(auto& m : toolset_items)
	{
		if(m.second->is_open && m.second->AnyEntityChanges())
			return true;
	}
	return false;
}

void Toolset::OpenPickMeshDialog(int field_index)
{
	PickFileDialogOptions options;
	options.title = "Pick mesh file";
	//options.preview = ? ;
	options.filters = "Mesh (qmsh);qmsh;All files;*";
	options.root_dir = "data";
	options.handler = [](PickFileDialog* dialog) {};
	options.Show();
}
