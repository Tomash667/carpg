#include "Pch.h"
#include "Base.h"
#include "GuiElement.h"
#include "KeyStates.h"
#include "MenuBar.h"
#include "MenuStrip.h"
#include "Overlay.h"

using namespace gui;

MenuStrip::MenuStrip(vector<SimpleMenuCtor>& _items, int min_width) : items(items), selected(nullptr)
{
	int width, max_width = 0;
	Font* font = layout->menustrip.font;

	items.resize(_items.size());
	for(uint i = 0, size = _items.size(); i < size; ++i)
	{
		SimpleMenuCtor& item1 = _items[i];
		Item& item2 = items[i];
		item2.text = item1.text;
		item2.action = item1.id;
		item2.hover = false;
		item2.index = i;
		item2.enabled = true;

		width = font->CalculateSize(item2.text).x;
		if(width > max_width)
			max_width = width;
	}
	
	size = INT2(max_width + (layout->menustrip.padding.x + layout->menustrip.item_padding.x ) * 2,
		(font->height + (layout->menustrip.item_padding.y) * 2) * items.size() + layout->menustrip.padding.y * 2);
}

MenuStrip::MenuStrip(vector<GuiElement*>& _items, int min_width) : items(items), selected(nullptr)
{
	int width, max_width = 0;
	Font* font = layout->menustrip.font;

	items.resize(_items.size());
	for(uint i = 0, size = _items.size(); i < size; ++i)
	{
		GuiElement* e = _items[i];
		Item& item2 = items[i];
		item2.text = e->ToString();
		item2.action = e->value;
		item2.hover = false;
		item2.index = i;
		item2.enabled = true;

		width = font->CalculateSize(item2.text).x;
		if(width > max_width)
			max_width = width;
	}

	size = INT2(max_width + (layout->menustrip.padding.x + layout->menustrip.item_padding.x) * 2,
		(font->height + (layout->menustrip.item_padding.y) * 2) * items.size() + layout->menustrip.padding.y * 2);
}

MenuStrip::~MenuStrip()
{

}

void MenuStrip::Draw(ControlDrawData*)
{
	BOX2D area(global_pos.ToVEC2(), (global_pos + size).ToVEC2());
	GUI.DrawArea(area, layout->menustrip.background);

	VEC2 item_size((float)size.x - (layout->menustrip.padding.x) * 2,
		(float)layout->menustrip.font->height + layout->menustrip.item_padding.y * 2);
	area.v1 = (global_pos + layout->menustrip.padding).ToVEC2();
	area.v2 = area.v1 + item_size;
	float offset = item_size.y;
	RECT r;

	for(Item& item : items)
	{
		if(item.hover)
			GUI.DrawArea(area, layout->menustrip.button_hover);

		DWORD color;
		if(!item.enabled)
			color = layout->menustrip.font_color_disabled;
		else if(item.hover)
			color = layout->menustrip.font_color_hover;
		else
			color = layout->menustrip.font_color;
		r = area.ToRect(layout->menustrip.item_padding);
		GUI.DrawText(layout->menustrip.font, item.text, DT_LEFT, color, r);

		area += VEC2(0, offset);
	}
}

void MenuStrip::Update(float dt)
{
	UpdateMouse();
	UpdateKeyboard();
}

void MenuStrip::UpdateMouse()
{
	if(!mouse_focus)
	{
		if(!focus)
		{
			if(selected)
				selected->hover = false;
			selected = nullptr;
		}
		return;
	}

	BOX2D area(global_pos.ToVEC2(), (global_pos + size).ToVEC2());
	if(!area.IsInside(GUI.cursor_pos))
	{
		if(GUI.MouseMoved())
		{
			if(selected)
			{
				selected->hover = false;
				selected = nullptr;
			}
		}
		return;
	}

	VEC2 item_size((float)size.x - (layout->menustrip.padding.x) * 2,
		(float)layout->menustrip.font->height + layout->menustrip.item_padding.y * 2);
	area.v1 = (global_pos + layout->menustrip.padding).ToVEC2();
	area.v2 = area.v1 + item_size;
	float offset = item_size.y;

	for(Item& item : items)
	{
		if(area.IsInside(GUI.cursor_pos))
		{
			if(item.enabled && (GUI.MouseMoved() || Key.Pressed(VK_LBUTTON)))
			{
				if(selected)
					selected->hover = false;
				selected = &item;
				selected->hover = true;
				if(Key.PressedRelease(VK_LBUTTON))
				{
					if(handler)
						handler(item.action);
					GUI.GetOverlay()->CloseMenu(this);
				}
			}
			break;
		}

		area += VEC2(0, offset);
	}
}

void MenuStrip::UpdateKeyboard()
{
	if(!focus)
		return;

	if(Key.PressedRelease(VK_DOWN))
		ChangeIndex(+1);
	else if(Key.PressedRelease(VK_UP))
		ChangeIndex(-1);
	else if(Key.PressedRelease(VK_LEFT))
	{
		if(parent_menu_bar)
			parent_menu_bar->ChangeMenu(-1);
	}
	else if(Key.PressedRelease(VK_RIGHT))
	{
		if(parent_menu_bar)
			parent_menu_bar->ChangeMenu(+1);
	}
	else if(Key.PressedRelease(VK_RETURN))
	{
		if(selected)
		{
			if(handler)
				handler(selected->action);
			GUI.GetOverlay()->CloseMenu(this);
		}
	}
	else if(Key.PressedRelease(VK_ESCAPE))
		GUI.GetOverlay()->CloseMenu(this);
}

void MenuStrip::ShowAt(const INT2& _pos)
{
	if(selected)
		selected->hover = false;
	selected = nullptr;
	pos = _pos;
	global_pos = pos;
	Show();
}

void MenuStrip::ShowMenu(const INT2& _pos)
{
	GUI.GetOverlay()->ShowMenu(this, _pos);
}

void MenuStrip::SetSelectedIndex(int index)
{
	assert(index >= 0 && index < (int)items.size());
	if(!items[index].enabled)
		return;
	if(selected)
		selected->hover = false;
	selected = &items[index];
	selected->hover = true;
}

MenuStrip::Item* MenuStrip::FindItem(int action)
{
	for(Item& item : items)
	{
		if(item.action == action)
			return &item;
	}
	return nullptr;
}

void MenuStrip::ChangeIndex(int dif)
{
	if(items.empty())
		return;

	int start;
	if(!selected)
	{
		selected = &items[0];
		start = 0;
	}
	else
	{
		selected->hover = false;
		selected = &items[modulo(selected->index + dif, items.size())];
		start = selected->index;
	}

	while(!selected->enabled)
	{
		selected = &items[modulo(selected->index + dif, items.size())];
		if(selected->index == start)
		{
			// all items disabled
			selected = nullptr;
			return;
		}
	}

	selected->hover = true;
}

bool MenuStrip::IsOpen()
{
	return GUI.GetOverlay()->IsOpen(this);
}
