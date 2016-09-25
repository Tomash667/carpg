#include "Pch.h"
#include "Base.h"
#include "Window.h"
#include "MenuBar.h"
#include "ToolStrip.h"

using namespace gui;

Window::Window(bool fullscreen) : Container(true), menu(nullptr), toolstrip(nullptr), fullscreen(fullscreen)
{
	size = INT2(300, 200);
}

Window::~Window()
{
}

void Window::Draw(ControlDrawData*)
{
	GUI.DrawArea(body_rect, layout->window.background);

	if(!fullscreen)
	{
		GUI.DrawArea(header_rect, layout->window.header);
		if(!text.empty())
		{
			RECT r = header_rect.ToRect(layout->window.padding);
			GUI.DrawText(layout->window.font, text, DT_LEFT | DT_VCENTER, layout->window.font_color, r, &r);
		}
	}
	
	RECT r = body_rect.ToRect();
	ControlDrawData cdd = { &r };
	Container::Draw(&cdd);
}

void Window::Update(float dt)
{
	Container::Update(dt);

	if(mouse_focus && IsInside(GUI.cursor_pos))
		TakeFocus();
}

void Window::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		{
			if(fullscreen)
			{
				size = GUI.wnd_size;
				pos = INT2(0, 0);
			}
			body_rect = BOX2D(float(pos.x), float(pos.y), float(pos.x + size.x), float(pos.y + size.y));
			
			for(Control* c : ctrls)
				c->Initialize();
		}
		break;
	case GuiEvent_WindowResize:
		if(fullscreen)
		{
			size = GUI.wnd_size;
			if(menu)
				menu->Event(GuiEvent_Resize);
			if(toolstrip)
				toolstrip->Event(GuiEvent_Resize);
		}
		break;
	default:
		Container::Event(e);
		break;
	}
}

void Window::SetMenu(MenuBar* _menu)
{
	assert(_menu && !menu && !initialized);
	menu = _menu;
	Add(menu);
}

void Window::SetToolStrip(ToolStrip* _toolstrip)
{
	assert(_toolstrip && !toolstrip && !initialized);
	toolstrip = _toolstrip;
	Add(toolstrip);
}
