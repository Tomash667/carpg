#include "Pch.h"
#include "Base.h"
#include "Panel2.h"
#include "MenuBar.h"

using namespace gui;
/*
Panel2::Panel2() : menu(nullptr)
{
	bkg_color = COLOR_RGB(100, 200, 255);
}

void Panel2::SetMenu(MenuBar* _menu)
{
	menu = _menu;
}

void Panel2::Draw2()
{
	GUI.DrawArea(bkg_color, pos, size);

	if(menu)
		menu->Draw();
}

void Panel2::SetSize(const INT2& _size)
{
	if(size == _size)
		return;
	size = _size;
	if(menu)
		menu->SetSize(INT2(size.x, menu->GetHeight()));
}*/

/*void Panel2::SetPosition(const INT2& _pos)
{
	
	pos = _pos;
}*/

/*
Panel::Panel() : menu(nullptr), in_update(false), need_refresh(false), need_reposition(false)
{

}

void Panel::SetMenu(MenuBar* _menu)
{
	if(menu == _menu)
		return;

	menu = _menu;
	NeedRefresh();
}

void Panel::SetSize(const INT2& _size)
{
	if(size == _size)
		return;

	size = _size;
	NeedRefresh();
}

void Panel::SetPosition(const INT2& _pos)
{
	// pos is relative to parent
	if(pos == _pos)
		return;

	pos = _pos;
	if(parent)
		global_pos = parent->global_pos + pos;
	else
		global_pos = pos;
	NeedReposition();
}

void Panel::BeginUpdate()
{
	assert(!in_update);
	in_update = true;
}

void Panel::EndUpdate()
{
	assert(in_update);
	in_update = false;
	if(need_refresh)
	{
		need_refresh = false;
		Refresh();
	}
	if(need_reposition)
	{
		need_reposition = false;
		NeedReposition();
	}
}

void Panel::Refresh()
{
	
}

void Panel::Reposition()
{

}

void Panel::Draw(ControlDrawData* cdd)
{
	assert(!in_update);
}

void Panel::Update(float dt)
{
	assert(!in_update);
}
*/