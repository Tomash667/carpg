#include "Pch.h"
#include "Base.h"
#include "TabControl.h"
#include "Window.h"
#include "KeyStates.h"

using namespace gui;

static ObjectPool<TabControl::Tab> tab_pool;

TabControl::TabControl(bool own_wnds) : selected(nullptr), hover(nullptr), own_wnds(own_wnds)
{

}

TabControl::~TabControl()
{
	Clear();
}

void TabControl::Draw(ControlDrawData*)
{
	BOX2D body_rect(global_pos, size);
	GUI.DrawArea(body_rect, layout->tabctrl.background);

	RECT rect;
	for(Tab* tab : tabs)
	{
		AreaLayout* button;
		AreaLayout* close;
		DWORD color;
		switch(tab->mode)
		{
		default:
		case Tab::Up:
			button = &layout->tabctrl.button;
			color = layout->tabctrl.font_color;
			break;
		case Tab::Hover:
			button = &layout->tabctrl.button_hover;
			color = layout->tabctrl.font_color_hover;
			break;
		case Tab::Down:
			button = &layout->tabctrl.button_down;
			color = layout->tabctrl.font_color_down;
			break;
		}
		if(tab->close_hover)
			close = &layout->tabctrl.close_hover;
		else
			close = &layout->tabctrl.close;

		GUI.DrawArea(tab->rect, *button);
		rect = tab->rect.ToRect(layout->tabctrl.padding);
		GUI.DrawText(layout->tabctrl.font, tab->text, DT_LEFT | DT_VCENTER, color, rect);
		GUI.DrawArea(tab->close_rect, *close);
	}
}

void TabControl::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		Update(true, true);
		break;
	case GuiEvent_Moved:
		Update(true, false);
		break;
	case GuiEvent_Resize:
		Update(false, true);
		break;
	}
}

void TabControl::Update(float dt)
{
	if(hover)
	{
		hover->mode = Tab::Up;
		hover->close_hover = false;
		hover = nullptr;
	}
	if(selected)
		selected->close_hover = false;

	if(mouse_focus && IsInside(GUI.cursor_pos))
	{
		for(Tab* tab : tabs)
		{
			if(tab->rect.IsInside(GUI.cursor_pos))
			{
				if(tab != selected)
				{
					hover = tab;
					hover->mode = Tab::Hover;
				}
				if(tab->close_rect.IsInside(GUI.cursor_pos))
				{
					tab->close_hover = true;
					if(Key.Pressed(VK_LBUTTON))
						tab->Close();
				}
				else if(Key.Pressed(VK_LBUTTON) && tab != selected)
				{
					if(selected)
						selected->mode = Tab::Up;
					selected = tab;
					selected->mode = Tab::Down;
					hover = nullptr;
				}
				break;
			}
		}

		TakeFocus();
	}
}

TabControl::Tab* TabControl::AddTab(cstring id, cstring text, Window* window, bool select)
{
	assert(id && text && window);
	Tab* tab = new Tab;
	tab->parent = this;
	tab->id = id;
	tab->text = text;
	tab->window = window;
	tab->mode = Tab::Up;
	tab->close_hover = false;
	tab->size = layout->tabctrl.font->CalculateSize(text) + layout->tabctrl.padding * 2 + INT2(layout->tabctrl.close_size.x + layout->tabctrl.padding.x, 0);
	CalculateRect(*tab);
	tabs.push_back(tab);
	if(select || tabs.size() == 1u)
	{
		if(selected)
			selected->mode = Tab::Up;
		selected = tab;
		selected->mode = Tab::Down;
	}
	return tab;
}

void TabControl::Clear()
{
	if(own_wnds)
	{
		for(Tab* tab : tabs)
			delete tab->window;
	}
	tab_pool.Free(tabs);
	selected = nullptr;
	hover = nullptr;
	offset = 0;
}

TabControl::Tab* TabControl::Find(cstring id)
{
	for(Tab* tab : tabs)
	{
		if(tab->id == id)
			return tab;
	}
	return nullptr;
}

void TabControl::Close(Tab* tab)
{
	assert(tab);
	if(tab == hover)
		hover = nullptr;
	int index = GetIndex(tabs, tab);
	if(tab == selected)
	{
		// select next tab or previous if not exists
		if(index == tabs.size() - 1)
		{
			if(index == 0)
				selected = nullptr;
			else
				selected = tabs[index - 1];
		}
		else
			selected = tabs[index + 1];
		if(selected)
			selected->mode = Tab::Down;
	}
	tab_pool.Free(tab);
	tabs.erase(tabs.begin() + index);
	CalculateRect();
}

void TabControl::Select(Tab* tab)
{
	assert(tab);
	if(tab == selected)
		return;
	if(selected)
		selected->mode = Tab::Up;
	selected = tab;
	selected->mode = Tab::Down;
	if(selected == hover)
		hover = nullptr;
}

void TabControl::Update(bool move, bool resize)
{
	if(move)
	{
		if(!IsDocked())
			global_pos = parent->global_pos + pos;
		CalculateRect();
	}
}

void TabControl::CalculateRect()
{
	offset = 0;
	for(Tab* tab : tabs)
		CalculateRect(*tab);
}

void TabControl::CalculateRect(Tab& tab)
{
	tab.rect.v1 = VEC2((float)global_pos.x + offset, (float)global_pos.y);
	tab.rect.v2 = tab.rect.v1 + tab.size.ToVEC2();
	offset += tab.size.x;

	tab.close_rect.v1 = VEC2(tab.rect.v2.x - layout->tabctrl.padding.x - layout->tabctrl.close_size.x,
		tab.rect.v1.y + (tab.size.y - layout->tabctrl.close_size.y) / 2);
	tab.close_rect.v2 = tab.close_rect.v1 + layout->tabctrl.close_size.ToVEC2();
}
