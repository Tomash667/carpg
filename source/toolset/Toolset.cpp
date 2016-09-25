#include "Pch.h"
#include "Base.h"
#include "Toolset.h"
#include "Engine.h"
#include "Gui2.h"
#include "Overlay.h"
#include "Window.h"
#include "MenuBar.h"
#include "ToolStrip.h"

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

Toolset::Toolset() : engine(nullptr)
{

}

Toolset::~Toolset()
{
}

void Toolset::Init(Engine* _engine)
{
	engine = _engine;
	
	Window* window = new Window(true);

	MenuBar* menu = new MenuBar;
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

	ToolStrip* toolstrip = new ToolStrip;

	window->SetMenu(menu);
	window->SetToolStrip(toolstrip);

	window->Initialize();

	Add(window);
	GUI.Add(this);
}

void Toolset::Start()
{
	GUI.SetOverlay(this);
	Show();
	closing = false;
}

void Toolset::End()
{
	GUI.SetOverlay(nullptr);
	Hide();
	closing = true;
}

void Toolset::OnDraw()
{
	GUI.Draw(engine->wnd_size);
}

bool Toolset::OnUpdate(float dt)
{
	if(closing)
		return false;

	return true;
}

void Toolset::Update(float dt)
{
	Overlay::Update(dt);

	if(Key.PressedRelease(VK_ESCAPE))
		End();
}

void Toolset::HandleMenuEvent(int id)
{

}
