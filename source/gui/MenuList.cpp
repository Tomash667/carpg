#include "Pch.h"
#include "Base.h"
#include "MenuList.h"
#include "KeyStates.h"

//=================================================================================================
MenuList::MenuList() : event_handler(nullptr), w(0), selected(-1), items_owner(true)
{

}

//=================================================================================================
MenuList::~MenuList()
{
	if(items_owner)
		DeleteElements(items);
}

//=================================================================================================
void MenuList::Draw(ControlDrawData*)
{
	GUI.DrawItem(GUI.tBox2, global_pos, size, WHITE, 8, 32);

	RECT rect = {global_pos.x+5, global_pos.y+5, global_pos.x+size.x-5, global_pos.y+25};
	for(GuiElement* e : items)
	{
		GUI.DrawText(GUI.default_font, e->ToString(), DT_SINGLELINE, BLACK, rect, &rect);
		rect.top += 20;
		rect.bottom += 20;
	}

	if(selected != -1)
	{
		RECT r2 = {global_pos.x+4, global_pos.y+4+selected*20, global_pos.x+size.x-4, global_pos.y+24+selected*20};
		GUI.DrawSpriteRect(GUI.tPix, r2, COLOR_RGBA(0,148,255,128));
	}
}

//=================================================================================================
void MenuList::Update(float dt)
{
	selected = -1;
	if(IsInside(GUI.cursor_pos))
	{
		selected = (GUI.cursor_pos.y - global_pos.y)/20;
		if(selected >= (int)items.size())
			selected = -1;
	}
	if(Key.Focus())
	{
		if(Key.PressedRelease(VK_LBUTTON))
		{
			if(selected != -1 && event_handler)
				event_handler(selected);
			LostFocus();
		}
		else if(Key.PressedRelease(VK_RBUTTON) || Key.PressedRelease(VK_ESCAPE))
			LostFocus();
	}
}

//=================================================================================================
void MenuList::Event(GuiEvent e)
{
	if(e == GuiEvent_LostFocus)
		visible = false;
}

//=================================================================================================
void MenuList::Init()
{
	size = INT2(w+10,20*items.size()+10);
}

//=================================================================================================
void MenuList::AddItem(GuiElement* e)
{
	assert(e);
	items.push_back(e);
	PrepareItem(e->ToString());
}

//=================================================================================================
void MenuList::AddItems(vector<GuiElement*>& new_items, bool is_owner)
{
	items_owner = is_owner;
	for(GuiElement* e : new_items)
	{
		assert(e);
		items.push_back(e);
		PrepareItem(e->ToString());
	}
}

//=================================================================================================
void MenuList::PrepareItem(cstring text)
{
	assert(text);
	int w2 = GUI.default_font->CalculateSize(text).x;
	if(w2 > w)
		w = w2;
}
