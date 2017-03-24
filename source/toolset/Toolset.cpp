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
#include "GameTypeManager.h"
#include "ListBox.h"
#include "Label.h"
#include "Button.h"
#include "TextBox.h"
#include "GameTypeProxy.h"

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

enum TreeMenuAction
{
	TMA_ADD,
	TMA_COPY,
	TMA_REMOVE
};

enum ListAction
{
	A_ADD,
	A_DUPLICATE,
	A_REMOVE
};

Toolset::Toolset(GameTypeManager& gt_mgr) : engine(nullptr), gt_mgr(gt_mgr), unsaved_changes(false)
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
		{"Reload", MA_RELOAD},
		{"Exit to menu", MA_EXIT_TO_MENU},
		{"Quit", MA_QUIT}
	});
	menu->AddMenu("Buildings", {
		{"Building groups", MA_BUILDING_GROUPS},
		{"Buildings", MA_BUILDINGS},
		{"Building scripts", MA_BUILDING_SCRIPTS}
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

	vector<SimpleMenuCtor> tree_menu_items = {
		{ "Add", (int)TMA_ADD },
		{ "Copy", (int)TMA_COPY },
		{ "Remove", (int)TMA_REMOVE }
	};
	tree_menu = new MenuStrip(tree_menu_items);
	tree_menu->SetHandler(delegate<void(int)>(this, &Toolset::HandleTreeViewMenuEvent));

	vector<SimpleMenuCtor> menu_strip_items = {
		{ "Add", A_ADD },
		{ "Duplicate", A_DUPLICATE },
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

void Toolset::Update(float dt)
{
	Overlay::Update(dt);

	if(Key.PressedRelease(VK_ESCAPE))
		closing = Closing::Yes;
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
		ShowGameType(GT_BuildingGroup);
		break;
	case MA_BUILDINGS:
		ShowGameType(GT_Building);
		break;
	case MA_BUILDING_SCRIPTS:
		ShowGameType(GT_BuildingScript);
		break;
	case MA_ABOUT:
		SimpleDialog("TOOLSET\n=========\nNothing interesting here yet...");
		break;
	}
}

void Toolset::Save()
{
	if(!unsaved_changes)
		return;

	// saving...

	unsaved_changes = false;
}

void Toolset::Reload()
{

}

void Toolset::ExitToMenu()
{
	closing = Closing::Yes;
}

void Toolset::Quit()
{
	closing = Closing::Shutdown;
}

void Toolset::ShowGameType(GameTypeId id)
{
	GameType& type = gt_mgr.GetGameType(id);
	TabControl::Tab* tab = tab_ctrl->Find(type.GetId().c_str());
	if(tab)
	{
		tab->Select();
		return;
	}

	tab_ctrl->AddTab(type.GetId().c_str(), type.GetFriendlyName().c_str(), GetToolsetItem(id)->window);
}

ToolsetItem* Toolset::GetToolsetItem(GameTypeId id)
{
	auto it = toolset_items.find(id);
	if(it != toolset_items.end())
		return it->second;
	ToolsetItem* toolset_item = CreateToolsetItem(id);
	toolset_items[id] = toolset_item;
	return toolset_item;
}

class TreeItem : public TreeNode
{
public:
	TreeItem(GameType& type, GameTypeItem item) : type(type), item(item)
	{
		SetText(type.GetItemId(item));
	}

	GameType& type;
	GameTypeItem item;
};

struct GameItemElement : public GuiElement
{
	GameTypeItem item;
	string& id;

	GameItemElement(GameTypeItem item, string& id) : item(item), id(id) {}

	cstring ToString() override
	{
		return id.c_str();
	}
};

ToolsetItem* Toolset::CreateToolsetItem(GameTypeId id)
{
	ToolsetItem* toolset_item = new ToolsetItem(gt_mgr.GetGameType(id));
	toolset_item->window = new Window;

	GameType& type = toolset_item->game_type;
	GameTypeHandler* handler = type.GetHandler();

	Window* w = toolset_item->window;

	/*TreeView* tree = new TreeView;
	tree->SetMultiselect(true);
	tree->SetKeyEvent(KeyEvent(this, &Toolset::HandleTreeViewKeyEvent));
	tree->SetMouseEvent(MouseEvent(this, &Toolset::HandleTreeViewMouseEvent));
	for(GameTypeItem item : handler->ForEach())
		tree->AddChild(new TreeItem(type, item));
	w->Add(tree);*/

	Label* label = new Label(Format("%s (%u):", type.GetFriendlyName().c_str(), handler->Count()));
	label->SetPosition(INT2(5, 5));
	w->Add(label);

	ListBox* list_box = new ListBox(true);
	list_box->menu_strip = menu_strip;
	list_box->event_handler2 = DialogEvent2(this, &Toolset::HandleListBoxEvent);
	list_box->SetPosition(INT2(5, 30));
	list_box->SetSize(INT2(200, 500));
	list_box->Init();
	for(auto e : handler->ForEach())
		list_box->Add(new GameItemElement(e, type.GetItemId(e)));
	w->Add(list_box);

	Button* bt = new Button;
	bt->text = "Save";
	bt->SetPosition(INT2(5, 535));
	bt->SetSize(INT2(100, 30));
	w->Add(bt);

	bt = new Button;
	bt->text = "Reload";
	bt->SetPosition(INT2(110, 535));
	bt->SetSize(INT2(100, 30));
	w->Add(bt);

	Panel* panel = new Panel;
	panel->custom_color = 0;
	panel->use_custom_color = true;
	panel->SetPosition(INT2(210, 5));
	panel->SetSize(tab_ctrl->GetAreaSize() - list_box->GetSize());

	label = new Label("Name");
	label->SetPosition(INT2(0, 0));
	panel->Add(label);

	TextBox* text_box = new TextBox(false, true);
	text_box->SetPosition(INT2(0, 30));
	text_box->SetSize(INT2(300, 30));
	panel->Add(text_box);

	w->Add(panel);
	w->SetDocked(true);
	w->Initialize();

	toolset_item->list_box = list_box;
	toolset_item->box = text_box;
	current_toolset_item = toolset_item;


	if(list_box->IsEmpty())
		panel->Disable();
	else
		list_box->Select(0, true);

	return toolset_item;
}

void Toolset::HandleTreeViewKeyEvent(gui::KeyEventData& e)
{
	/*
	lewo - zwiñ, idŸ do parenta (z shift dzia³a)
	prawo - rozwiñ - idŸ do 1 childa (shift)
	góra - z shift
	dó³
	del - usuñ
	spacja - zaznacz
	litera - przejdŸ do nastêpnego o tej literze

	klik - zaznacza, z ctrl dodaje/usuwa zaznaczenie
		z shift - zaznacza wszystkie od poprzedniego focusa do teraz
		z shift zaznacza obszar od 1 klika do X, 1 miejsce sie nie zmienia
		z shift i ctrl nie usuwa nigdy zaznaczenia (mo¿na dodaæ obszary)
	*/
}

void Toolset::HandleTreeViewMouseEvent(gui::MouseEventData& e)
{
	if(e.button == VK_RBUTTON && !e.pressed)
	{
		TreeView* tree = (TreeView*)e.control;
		bool clicked_node = (tree->GetClickedNode() != nullptr);
		tree_menu->FindItem(TMA_COPY)->SetEnabled(clicked_node);
		tree_menu->FindItem(TMA_REMOVE)->SetEnabled(clicked_node);
	}
}

void Toolset::HandleTreeViewMenuEvent(int id)
{
	switch(id)
	{
	case TMA_ADD:
	case TMA_COPY:
	case TMA_REMOVE:
		break;
	}
}

void Toolset::HandleListBoxEvent(int action, int id)
{
	switch(action)
	{
	case ListBox::A_INDEX_CHANGED:
		{
			GameItemElement* e = current_toolset_item->list_box->GetItemCast<GameItemElement>();
			current_toolset_item->box->SetText(e->id.c_str());
		}
		break;
	case ListBox::A_BEFORE_MENU_SHOW:
		{
			bool enabled = (id != -1);
			menu_strip->FindItem(A_DUPLICATE)->SetEnabled(enabled);
			menu_strip->FindItem(A_REMOVE)->SetEnabled(enabled);
		}
		break;
	case ListBox::A_MENU:
		{

		}
		break;
	}
}
