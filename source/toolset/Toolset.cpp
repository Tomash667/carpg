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

Toolset::Toolset(GameTypeManager& gt_mgr) : engine(nullptr), gt_mgr(gt_mgr)
{

}

Toolset::~Toolset()
{
	for(auto& w : wnds)
		delete w.second;
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
	tab_ctrl->Dock();

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
	case MA_RELOAD:
		break;
	case MA_EXIT_TO_MENU:
		closing = Closing::Yes;
		break;
	case MA_QUIT:
		closing = Closing::Shutdown;
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
		break;
	}
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

	cstring name;
	switch(id)
	{
	case GT_Building:
		name = "Buildings";
		break;
	case GT_BuildingGroup:
		name = "Building groups";
		break;
	case GT_BuildingScript:
		name = "Building scripts";
		break;
	default:
		assert(0);
		name = "(missing)";
		break;
	}
	tab_ctrl->AddTab(type.GetId().c_str(), name, GetWindow(id));
}

Window* Toolset::GetWindow(GameTypeId id)
{
	auto it = wnds.find(id);
	if(it != wnds.end())
		return it->second;
	Window* w = CreateWindow(id);
	wnds[id] = w;
	return w;
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

Window* Toolset::CreateWindow(GameTypeId id)
{
	GameType& type = gt_mgr.GetGameType(id);
	GameTypeHandler* handler = type.GetHandler();

	Window* w = new Window;

	TreeView* tree = new TreeView;
	tree->SetMultiselect(true);
	tree->SetKeyEvent(KeyEvent(this, &Toolset::HandleTreeViewKeyEvent));
	tree->SetMouseEvent(MouseEvent(this, &Toolset::HandleTreeViewMouseEvent));
	for(GameTypeItem item : handler->ForEach())
		tree->AddChild(new TreeItem(type, item));
	w->Add(tree);

	return w;
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
