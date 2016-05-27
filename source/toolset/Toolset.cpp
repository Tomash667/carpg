#include "Pch.h"
#include "Base.h"
#include "Toolset.h"
#include "Engine.h"
#include "Gui2.h"

enum MenuAction
{
	MA_SAVE,
	MA_RESET,
	MA_RELOAD,
	MA_EXIT_TO_MENU,
	MA_QUIT,
	MA_BUILDINGS,
	MA_BUILDING_GROUPS
};

class MenuItem;

class MenuItemContainer
{
public:
	MenuItem* AddItem(AnyString text) { return nullptr; }
	MenuItem* AddItem(AnyString text, int action) { return nullptr; }
	void AddItems(std::initializer_list<MenuItem*> const & items) {}

	vector<MenuItem*> items;
};

class MenuItem : public MenuItemContainer
{
public:
	MenuItem() {} // hr
	MenuItem(cstring text, int action = -1, cstring tooltip = nullptr) {}
	string text;
	VoidDelegate action;
};

class MenuBar : public MenuItemContainer
{
public:
};

void Toolset::Init(Engine* _engine)
{
	engine = _engine;

	BeginUpdate();

	MenuBar* menu = new MenuBar;
	menu->AddItem("Toolset")->AddItems({
		new MenuItem("Save", MA_SAVE),
		new MenuItem("Reset", MA_RESET, "Reset content to loaded at start."),
		new MenuItem("Reload", MA_RELOAD, "Reload content from files."),
		new MenuItem("Exit to menu", MA_EXIT_TO_MENU),
		new MenuItem("Quit")
	});
	menu->AddItem("Content")->AddItems({
		new MenuItem("Buildings", MA_BUILDINGS),
		new MenuItem("Building groups", MA_BUILDING_GROUPS)
	});

	SetMenu(menu);
	SetSize(GUI.wnd_size);
	EndUpdate();

	visible = false;
	GUI.Add(this);
}

void Toolset::Start()
{
	visible = true;
}

void Toolset::OnDraw()
{
	GUI.Draw(engine->wnd_size);
}

bool Toolset::OnUpdate(float dt)
{
	if(Key.Pressed(VK_ESCAPE))
	{
		visible = false;
		return false;
	}

	return true;
}

void Toolset::Event(GuiEvent e)
{
	if(e == GuiEvent_WindowResize)
	{
		SetSize(GUI.wnd_size);
	}
}
