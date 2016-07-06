#include "Pch.h"
#include "Base.h"
#include "MenuBar.h"
#include "KeyStates.h"

using namespace gui;

void MenuBar::AddAction(cstring text, int action)
{
	assert(text);
	Item* item = new Item;
	item->action = action;
	item->menu = nullptr;
	item->state = NONE;
	item->text = text.s;
	items.push_back(item);
}

void MenuBar::AddMenu(cstring text, Menu* menu)
{
	assert(text && menu);
	Item* item = new Item;
	item->action = -1;
	item->menu = menu;
	item->size = NONE;
	item->text = text.s;
	items.push_back(item);
}

void MenuBar::AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & _items)
{
	assert(text && _items.size() >= 1u);
	Menu* menu = new Menu;
	menu->
	Item* item = new Item;
	item->action = -1;
	
	items.reserve(items.size() + _items.size());
	for(const SimpleMenuCtor& it : _items)
	{
		Item* item = new Item;
		item->action
	}
}

MenuItem* MenuItemContainer::AddItem(AnyString text, int action)
{
	MenuItem* item = new MenuItem(text, action);
	items.push_back(item);
	return item;
}

void MenuItemContainer::AddItems(std::initializer_list<MenuItem*> const & _items)
{
	for(MenuItem* item : _items)
		items.push_back(item);
}

MenuBar::MenuBar() : handler(nullptr), selected(nullptr), clicked(false)
{
	font = GUI.default_font;
	height = font->height + 2;
	bkg_color = COLOR_RGB(211, 129, 74);
	font_color = BLACK;
	hover_color = COLOR_RGB(142, 243, 255);
	click_color = COLOR_RGB(56, 235, 255);
}

void MenuBar::Recalculate(bool size_change, bool pos_change)
{
	if(size_change)
	{
		INT2 offset(0, 0);

		for(MenuItem* item : items)
		{
			INT2 text_size = font->CalculateSize(item->text);
			item->pos = offset;
			item->size = text_size + INT2(20, 2);
			offset.x += item->size.x;
		}
	}

	if(pos_change)
	{
		for(MenuItem* item : items)
			item->global_pos = global_pos + item->pos;
	}
}

void MenuBar::Draw(ControlDrawData*)
{
	// bar
	GUI.DrawArea(bkg_color, global_pos, size);

	// selected
	if(selected)
		GUI.DrawArea(selected->state == HOVER ? hover_color : click_color, selected->global_pos, selected->size);

	// text
	RECT r;
	const INT2 offset(10, 1);
	for(MenuItem* item : items)
	{
		item->FillRect(r, offset);
		GUI.DrawText(font, item->text, DT_NOCLIP, font_color, r);
	}
}

void MenuBar::Event(GuiEvent e)
{
	if(e == GuiEvent_Resize)
		Recalculate(true, false);
	else if(e == GuiEvent_Moved)
		Recalculate(false, true);
}

void MenuBar::Update(float dt)
{
	// focus = clicked on one of items
	bool have_selection = false;
	if(mouse_focus)
	{
		bool pressed = Key.PressedRelease(VK_LBUTTON);
		for(Item* item : items)
		{
			if(PointInRect(GUI.cursor_pos, item->global_pos, item->size))
			{
				if(!focus)
				{
					// not clicked yet, remove old hover, add new one
					if(selected)
						selected->state = NONE;
					selected = item;
					selected->state = HOVER;
					have_selection = true;
					if(pressed)
					{
						if(selected->menu)
						{
							// show item menu
							selected->menu->Show();
						}
						else
						{
							// item don't have menu, activate menubar and wait for release
							GUI.GainFocus(this);
						}
					}
				}
				else
				{
					// one of buttons clicked
				}
				selected = item;
				selected->state = HOVER;
				if(pressed)
				{
					pressed = false;
					if(clicked)
					{
						// unclick
					}
					else
					{
						// clicked on menu item
						clicked = true;
						selected->state = PRESSED;
					}
				}
			}
			break;
		}

		if(pressed && focus)
		{
			// clicked outside of items, remove focus
			GUI.LostFocus();
		}
	}
	else
	{
		if(Key.Released(VK_LBUTTON) && focus)
		{
			// released key outside of menubar or menu
			GUI.LostFocus();
		}
	}

	if(!have_selection && selected)
	{
		selected->state = NONE;
		selected = nullptr;
	}
}
