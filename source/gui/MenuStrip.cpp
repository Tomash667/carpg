#include "Pch.h"
#include "Base.h"
#include "MenuStrip.h"

using namespace gui;

void MenuStrip::AddAction(cstring text, int action)
{
	assert(text);
	MenuItem* item = new MenuItem;
	item->action = action;
	item->hr = false;
	item->menu = nullptr;
	item->text = text;
	items.push_back(item);
}

void MenuStrip::AddLine()
{
	MenuItem* item = new MenuItem;
	item->action = -1;
	item->hr = true;
	item->menu = nullptr;
	items.push_back(item);
}

void MenuStrip::AddMenu(cstring text, MenuStrip* menu)
{
	assert(text && menu);
	MenuItem* item = new MenuItem;
	item->action = -1;
	item->hr = false;
	item->menu = menu;
	menu->handler = handler;
	menu->parent = this;
	items.push_back(item);
}

void MenuStrip::AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & _items)
{
	assert(text);
	MenuStrip* menu = new MenuStrip;
	menu->handler = handler;
	menu->parent = this;
	menu->AddItems(_items);
	MenuItem* item = new MenuItem;
	item->action = -1;
	item->hr = false;
	item->menu = menu;
	item->text = text;
	items.push_back(item);
}

void MenuStrip::AddItems(std::initializer_list<SimpleMenuCtor> const & _items)
{
	items.reserve(items.size() + _items.size());
	for(const SimpleMenuCtor& it : _items)
	{
		assert(it.text);
		MenuItem* item = new MenuItem;
		if(strcmp(it.text, "---") == 0)
		{
			item->action = -1;
			item->hr = true;
		}
		else
		{
			item->action = it.action;
			item->hr = false;
			item->text = it.text;
		}
		item->menu = nullptr;
	}
}
