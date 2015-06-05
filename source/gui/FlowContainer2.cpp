#include "Pch.h"
#include "Base.h"
#include "FlowContainer2.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
ObjectPool<FlowItem2> FlowItemPool;

//=================================================================================================
FlowContainer2::FlowContainer2() : id(-1), group(-1), redo_select(true)
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
	if(mouse_focus)
	{
		if(IsInside(GUI.cursor_pos))
		{
			if(Key.Focus())
				scroll.ApplyMouseWheel();

			if(redo_select || GUI.cursor_pos != last_pos)
			{
				redo_select = false;
				last_pos = GUI.cursor_pos;

				INT2 off(0, (int)scroll.offset);

				for(FlowItem2* fi : items)
				{
					INT2 p = fi->pos - off;
					if(PointInRect(GUI.cursor_pos, p, fi->size))
					{
						if(!fi->section && fi->group != -1)
						{
							group = fi->group;
							id = fi->id;
						}
						break;
					}
					else if(GUI.cursor_pos.y < p.y)
						break;
				}
			}
		}
		scroll.Update(dt);
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

	for(FlowItem2* fi : items)
	{
		rect.left = global_pos.x + fi->pos.x;
		rect.right = rect.left + fi->size.x;
		rect.top = global_pos.y + fi->pos.y - (int)scroll.offset;
		rect.bottom = rect.top + fi->size.y;

		if(rect.bottom < global_pos.y)
			continue;

		if(!GUI.DrawTextA(fi->section ? GUI.fBig : GUI.default_font, fi->text, 0, BLACK, rect, &clip))
			break;
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
void FlowContainer2::Reposition()
{
	INT2 s;
	int sizex = size.x - 20;
	int y = 2;

	for(FlowItem2* fi : items)
	{
		Font* font = (fi->section ? GUI.fBig : GUI.default_font);

		fi->size = font->CalculateSize(fi->text, sizex);
		fi->pos = INT2(2, y);

		y += fi->size.y;
	}

	scroll.total = y;
	redo_select = true;
}
