#include "Pch.h"
#include "Base.h"
#include "MenuBar.h"
#include "MenuStrip.h"
#include "KeyStates.h"

using namespace gui;

MenuBar::MenuBar() : selected(nullptr), handler(nullptr)
{

}

MenuBar::~MenuBar()
{
	DeleteElements(items);
}

void MenuBar::Draw(ControlDrawData*)
{
	// backgroud
	GUI.DrawArea(rect, layout->menubar.background);
		
	// items
	RECT rect;
	for(Item* item : items)
	{
		AreaLayout* area_layout;
		DWORD font_color;
		switch(item->mode)
		{
		case Item::Up:
		default:
			area_layout = &layout->menubar.button;
			font_color = layout->menubar.font_color;
			break;
		case Item::Hover:
			area_layout = &layout->menubar.button_hover;
			font_color = layout->menubar.font_color_hover;
			break;
		case Item::Down:
			area_layout = &layout->menubar.button_down;
			font_color = layout->menubar.font_color_down;
			break;
		}

		// item background
		GUI.DrawArea(item->rect, *area_layout);

		// item text
		rect = item->rect.ToRect();
		GUI.DrawText(layout->menubar.font, item->text, DT_CENTER | DT_VCENTER, font_color, rect);
	}
}

void MenuBar::Update(float dt)
{
	bool down = false;
	if(selected)
	{
		if(selected->mode != Item::Down)
			selected->mode = Item::Up;
		else
			down = true;
	}
	
	if(!mouse_focus || !rect.IsInside(GUI.cursor_pos))
		return;

	for(Item* item : items)
	{				
		if(item->rect.IsInside(GUI.cursor_pos))
		{
			if(down || Key.Pressed(VK_LBUTTON))
			{
				if((item != selected || item->mode != Item::Down) && (Key.Pressed(VK_LBUTTON) || GUI.MouseMoved()))
				{
					EnsureMenu(item);
					item->menu->ShowMenu(INT2(item->rect.LeftBottom()));
					selected = item;
					selected->mode = Item::Down;
				}
			}
			else
			{
				selected = item;
				selected->mode = Item::Hover;
			}
			break;
		}
	}
}

void MenuBar::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		// control initialization, menubar must be attacked to other control (like Window)
		Update(true, true);
		break;
	case GuiEvent_Moved:
		// parent control moved
		Update(true, false);
		break;
	case GuiEvent_Resize:
		// parent window resized
		Update(false, true);
		break;
	case GuiEvent_Show:
		// control is shown, forget old selected item
		selected = nullptr;
		for(Item* item : items)
			item->mode = Item::Up;
		break;
	}
}

void MenuBar::AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & _items)
{
	assert(text);

	float item_height = (float)layout->menubar.font->height + layout->menubar.item_padding.y * 2;
	float item_width = (float)layout->menubar.font->CalculateSize(text).x + layout->menubar.item_padding.x * 2;

	Item* item = new Item;
	item->text = text;
	item->rect = BOX2D(0, 0, item_width, item_height);
	item->rect += layout->menubar.padding.ToVEC2() / 2;
	if(!items.empty())
		item->rect += VEC2(items.back()->rect.v2.x, 0);
	item->index = items.size();
	item->items = _items;
	items.push_back(item);
}

void MenuBar::Update(bool move, bool resize)
{
	assert(parent);
	INT2 prev_pos = global_pos;
	if(move)
		global_pos = parent->global_pos;
	if(resize)
		size = INT2(parent->size.x, layout->menubar.font->height + layout->menubar.padding.y + layout->menubar.item_padding.y * 2);
	rect = BOX2D::Create(global_pos, size);
	if(move)
	{
		VEC2 offset = (global_pos - prev_pos).ToVEC2();
		for(Item* item : items)
			item->rect += offset;
	}
}

void MenuBar::OnCloseMenuStrip()
{
	if(selected)
		selected->mode = Item::Up;
}

// offset must be -1/+1, menu must be open
void MenuBar::ChangeMenu(int offset)
{
	assert(offset == -1 || offset == 1);
	assert(selected);

	bool prev = (offset == -1);
	int index = modulo(selected->index + (prev ? -1 : 1), items.size());
	Item* item = items[index];

	// showing new menu should close the old one
	EnsureMenu(item);
	item->menu->ShowMenu(INT2(item->rect.LeftBottom()));
	item->menu->SetSelectedIndex(0);
	selected = item;
	selected->mode = Item::Down;
}

void MenuBar::EnsureMenu(Item* item)
{
	if(item->menu)
		return;
	MenuStrip* menu = new MenuStrip(item->items, (int)item->rect.SizeX());
	menu->SetOwner(this, item->index);
	menu->SetHandler(handler);
	menu->SetOnCloseHandler(MenuStrip::OnCloseHandler(this, &MenuBar::OnCloseMenuStrip));
	menu->Initialize();
	item->menu = menu;
}
