#include "Pch.h"
#include "Base.h"
#include "FlowContainer2.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
ObjectPool<FlowItem2> FlowItemPool;

//=================================================================================================
FlowContainer2::FlowContainer2() : id(-1), group(-1), on_button(NULL), button_size(0,0)
{
	size = INT2(-1, -1);
}

//=================================================================================================
FlowContainer2::~FlowContainer2()
{
	Clear();
}

//=================================================================================================
void FlowContainer2::Update(float dt)
{
	bool ok = false;
	group = -1;
	id = -1;
	
	if(mouse_focus)
	{
		if(IsInside(GUI.cursor_pos))
		{
			ok = true;

			if(Key.Focus())
				scroll.ApplyMouseWheel();

			INT2 off(0, (int)scroll.offset);

			for(FlowItem2* fi : items)
			{
				INT2 p = fi->pos - off + global_pos;
				if(fi->type == FlowItem2::Item)
				{
					if(fi->group != -1 && PointInRect(GUI.cursor_pos, p, fi->size))
					{
						group = fi->group;
						id = fi->id;
					}
				}
				else if(fi->type == FlowItem2::Button && fi->state != Button::DISABLED)
				{
					if(PointInRect(GUI.cursor_pos, p, fi->size))
					{
						GUI.cursor_mode = CURSOR_HAND;
						if(fi->state == Button::DOWN)
						{
							if(Key.Up(VK_LBUTTON))
							{
								fi->state = Button::HOVER;
								on_button(fi->group, fi->id);
								return;
							}
						}
						else if(Key.Pressed(VK_LBUTTON))
							fi->state = Button::DOWN;
						else
							fi->state = Button::HOVER;
					}
					else
						fi->state = Button::NONE;
				}
			}
		}
		scroll.Update(dt);
	}

	if(!ok)
	{		
		for(FlowItem2* fi : items)
		{
			if(fi->type == FlowItem2::Button)
				fi->state = Button::NONE;
		}
	}
}

//=================================================================================================
void FlowContainer2::Draw(ControlDrawData*)
{
	GUI.DrawItem(GUI.tBox, global_pos, size - INT2(16,0), WHITE, 8, 32);

	scroll.Draw();

	int sizex = size.x - 16;

	RECT rect;

	RECT clip;
	clip.left = global_pos.x + 2;
	clip.right = clip.left + sizex - 2;
	clip.top = global_pos.y + 2;
	clip.bottom = clip.top + size.y - 2;

	int offset = (int)scroll.offset;

	for(FlowItem2* fi : items)
	{
		if(fi->type != FlowItem2::Button)
		{
			rect.left = global_pos.x + fi->pos.x;
			rect.right = rect.left + fi->size.x;
			rect.top = global_pos.y + fi->pos.y - offset;
			rect.bottom = rect.top + fi->size.y;

			// text above box
			if(rect.bottom < global_pos.y)
				continue;

			if(!GUI.DrawTextA(fi->type == FlowItem2::Section ? GUI.fBig : GUI.default_font, fi->text, 0, BLACK, rect, &clip))
				break;
		}
		else
		{
			// button above or below box
			if(global_pos.y + fi->pos.y - offset + fi->size.y < global_pos.y ||
				global_pos.y + fi->pos.y - offset > global_pos.y + size.y)
				continue;

			GUI.DrawItem(button_tex[fi->tex_id].tex[fi->state], global_pos + fi->pos - INT2(0,offset), size, WHITE, 16);
		}
	}
}

//=================================================================================================
FlowItem2* FlowContainer2::Add()
{
	FlowItem2* item = FlowItemPool.Get();
	items.push_back(item);
	return item;
}

//=================================================================================================
void FlowContainer2::Clear()
{
	FlowItemPool.Free(items);
}

//=================================================================================================
void FlowContainer2::GetSelected(int& _group, int& _id)
{
	if(group != -1)
	{
		_group = group;
		_id = id;
	}
}

//=================================================================================================
void FlowContainer2::UpdateSize(const INT2& _pos, const INT2& _size, bool _visible)
{
	global_pos = pos = _pos;
	if(size != _size)
	{
		size = _size;
		if(_visible)
			Reposition();
	}
	scroll.global_pos = scroll.pos = global_pos + INT2(size.x - 17, 0);
	scroll.size = INT2(16, size.y);
	scroll.part = size.y;
}

//=================================================================================================
void FlowContainer2::UpdatePos(const INT2& parent_pos)
{
	global_pos = pos + parent_pos;
	scroll.global_pos = scroll.pos = global_pos + INT2(size.x - 17, 0);
}

//=================================================================================================
void FlowContainer2::Reposition()
{
	int sizex = size.x - 20;
	int y = 2;

	for(FlowItem2* fi : items)
	{
		if(fi->type != FlowItem2::Button)
		{
			Font* font = (fi->type == FlowItem2::Section ? GUI.fBig : GUI.default_font);
			fi->size = font->CalculateSize(fi->text, sizex);
			fi->pos = INT2((fi->type == FlowItem2::Section ? 2 : 4+button_size.x), y);
			y += fi->size.y;
		}
		else
		{
			fi->size = button_size;
			fi->pos = INT2(2, y);
		}
	}

	scroll.total = y;
}

//=================================================================================================
FlowItem2* FlowContainer2::Find(int _group, int _id)
{
	for(FlowItem2* fi : items)
	{
		if(fi->group == _group && fi->id == _id)
			return fi;
	}

	return NULL;
}
