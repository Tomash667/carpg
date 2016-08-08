#include "Pch.h"
#include "Base.h"
#include "Toolset.h"
#include "Engine.h"
#include "Gui2.h"
#include "MenuBar.h"
#include "Overlay.h"

using namespace gui;

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

void Toolset::Init(Engine* _engine)
{
	engine = _engine;

	// gui
	overlay = new Overlay;

	//BeginUpdate();

	/*MenuBar* menu = new MenuBar;
	menu->handler = fastdelegate::FastDelegate1<int>(this, &Toolset::HandleMenuEvent);
	menu->AddMenu("Toolset", {
		{"Save", MA_SAVE},
		{"Reset", MA_RESET},
		{"Reload", MA_RELOAD},
		{"Exit to menu", MA_EXIT_TO_MENU},
		{"Quit", MA_QUIT}
	});
	menu->AddMenu("Content", {
		{"Buildings", MA_BUILDINGS},
		{"building groups", MA_BUILDING_GROUPS}
	});
	overlay->menu = menu;
	menu->parent = overlay;
	overlay->visible = false;
	overlay->SetSize(GUI.wnd_size, true);
	overlay->SetPosition(INT2(0, 0), true);*/

	//GUI.Add(overlay);
}

void Toolset::Start()
{
	/*overlay->visible = true;
	GUI.current_overlay = overlay;*/
}

void Toolset::End()
{
	/*overlay->visible = false;
	GUI.current_overlay = nullptr;*/
}

void Toolset::OnDraw()
{
	//GUI.Draw(engine->wnd_size);
}

bool Toolset::OnUpdate(float dt)
{
	if(Key.Pressed(VK_ESCAPE))
	{
		//overlay->visible = false;
		return false;
	}

	return true;
}

void Toolset::HandleMenuEvent(int id)
{

}
