#include "Pch.h"
#include "Base.h"
#include "Overlay.h"
#include "MenuBar.h"
#include "KeyStates.h"

using namespace gui;

Overlay::Overlay() : menu(nullptr), focused(nullptr), top_focus(nullptr)
{

}

void Overlay::Draw(ControlDrawData*)
{
	if(menu && menu->visible)
		menu->Draw();
	for(Control* ctrl : ctrls)
	{
		if(ctrl->visible)
			ctrl->Draw();
	}
}

void Overlay::Event(GuiEvent e)
{
	if(e == GuiEvent_WindowResize)
		SetSize(GUI.wnd_size);
	else if(e == GuiEvent_Resize)
	{
		if(menu)
			menu->SetSize(INT2(size.x, menu->GetHeight()));
	}
	else if(e == GuiEvent_Moved)
	{
		if(menu)
			menu->Event(GuiEvent_Moved);
	}
}

void Overlay::Update(float dt)
{
	if(!visible)
		return;

	mouse_focus = true;

	if(menu && menu->visible)
		menu->UpdateWithMouseFocus(dt);

	for(Control* ctrl : ctrls)
	{
		if(ctrl->visible)
			menu->UpdateWithMouseFocus(dt);
	}

	if(Key.Pressed(VK_LBUTTON))
	{
		// clicked on nothing, lose focus
	}

	mouse_focus = false;
}
