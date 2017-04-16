#include "Pch.h"
#include "Base.h"
#include "Overlay.h"
#include "MenuStrip.h"
#include "KeyStates.h"

using namespace gui;

Overlay::Overlay() : Container(true), focused(nullptr)
{

}

Overlay::~Overlay()
{
	// prevent deleting twice
	for(MenuStrip* menu : menus)
		RemoveElement(ctrls, (Control*)menu);
}

void Overlay::Update(float dt)
{
	mouse_focus = true;
	if(GUI.HaveDialog())
	{
		mouse_focus = false;
		for(MenuStrip* menu : menus)
		{
			menu->OnClose();
			RemoveElement(ctrls, (Control*)menu);
		}
		menus.clear();
	}
	clicked = nullptr;
	Container::Update(dt);

	if(clicked)
	{
		MenuStrip* leftover = nullptr;
		for(MenuStrip* menu : menus)
		{
			if(menu != clicked)
			{
				menu->OnClose();
				to_close.push_back(menu);
			}
			else
				leftover = menu;
		}
		menus.clear();
		if(leftover)
			menus.push_back(leftover);
	}

	for(MenuStrip* menu : to_close)
		RemoveElement(ctrls, (Control*)menu);
	to_close.clear();

	if(to_add)
	{
		Add(to_add);
		menus.push_back(to_add);
		if(focused)
			focused->focus = false;
		to_add->focus = true;
		focused = to_add;
		to_add = nullptr;
	}
}

void Overlay::ShowMenu(MenuStrip* menu, const INT2& _pos)
{
	assert(menu);
	for(MenuStrip* menu : menus)
	{
		menu->OnClose();
		to_close.push_back(menu);
	}
	menus.clear();
	if(to_add)
		to_add->OnClose();
	to_add = menu;
	menu->ShowAt(_pos);
}

void Overlay::CloseMenu(MenuStrip* menu)
{
	assert(menu);
	menu->OnClose();
	RemoveElement(menus, menu);
	to_close.push_back(menu);
}

void Overlay::CheckFocus(Control* ctrl, bool pressed)
{
	assert(ctrl);
	if(!ctrl->mouse_focus)
		return;

	ctrl->mouse_focus = false;

	if(Key.PressedRelease(VK_LBUTTON)
		|| Key.PressedRelease(VK_RBUTTON)
		|| Key.PressedRelease(VK_MBUTTON)
		|| pressed)
	{
		assert(!clicked);
		clicked = ctrl;
		SetFocus(ctrl);
	}
}

void Overlay::SetFocus(Control* ctrl)
{
	assert(ctrl);
	if(focused)
	{
		if(focused == ctrl)
			return;
		focused->focus = false;
		focused->Event(GuiEvent_LostFocus);
	}
	ctrl->focus = true;
	ctrl->Event(GuiEvent_GainFocus);
	focused = ctrl;
}

bool Overlay::IsOpen(MenuStrip* menu)
{
	assert(menu);

	for(auto m : menus)
	{
		if(m == menu)
			return true;
	}

	return false;
}
