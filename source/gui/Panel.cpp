#include "Pch.h"
#include "Base.h"
#include "Panel.h"

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
