#include "Pch.h"
#include "Base.h"
#include "FlowContainer2.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
ObjectPool<FlowItem2> FlowItem2::Pool;

//=================================================================================================
FlowContainer2::FlowContainer2() : id(-1), group(-1), on_button(nullptr), button_size(0, 0), word_warp(true), allow_select(false), selected(nullptr), batch_changes(false)
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
						if(allow_select && fi->state != Button::DISABLED)
						{
							GUI.cursor_mode = CURSOR_HAND;
							if(on_select && Key.Pressed(VK_LBUTTON))
							{
								selected = fi;
								on_select();
								return;
							}
						}
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
			if(fi->type == FlowItem2::Button && fi->state != Button::DISABLED)
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

	DWORD flags = (word_warp ? 0 : DT_SINGLELINE);

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

			if(fi->state == Button::DOWN)
			{
				RECT rs = { global_pos.x + 2, rect.top, global_pos.x + sizex, rect.bottom };
				RECT out;
				if(IntersectRect(&out, &rs, &clip))
					GUI.DrawSpriteRect(GUI.tPix, out, COLOR_RGBA(0, 255, 0, 128));
			}

			if(!GUI.DrawText(fi->type == FlowItem2::Section ? GUI.fBig : GUI.default_font, fi->text, 0, (fi->state != Button::DISABLED ? BLACK : COLOR_RGB(64, 64, 64)), rect, &clip))
				break;
		}
		else
		{
			// button above or below box
			if(global_pos.y + fi->pos.y - offset + fi->size.y < global_pos.y ||
				global_pos.y + fi->pos.y - offset > global_pos.y + size.y)
				continue;

			GUI.DrawSprite(button_tex[fi->tex_id].tex[fi->state], global_pos + fi->pos - INT2(0, offset), WHITE, &clip);
		}
	}
}

//=================================================================================================
FlowItem2* FlowContainer2::Add()
{
	FlowItem2* item = FlowItem2::Pool.Get();
	items.push_back(item);
	return item;
}

//=================================================================================================
void FlowContainer2::Clear()
{
	FlowItem2::Pool.Free(items);
	batch_changes = false;
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
	if(size.y != _size.y && _visible)
	{
		size = _size;
		Reposition();
	}
	else
		size = _size;
	scroll.global_pos = scroll.pos = global_pos + INT2(size.x - 17, 0);
	scroll.size = INT2(16, size.y);
	scroll.part = size.y;
}

//=================================================================================================
void FlowContainer2::UpdatePos(const INT2& parent_pos)
{
	global_pos = pos + parent_pos;
	scroll.global_pos = scroll.pos = global_pos + INT2(size.x - 17, 0);
	scroll.size = INT2(16, size.y);
	scroll.part = size.y;
}

//=================================================================================================
void FlowContainer2::Reposition()
{
	int sizex = (word_warp ? size.x - 20 : 10000);
	int y = 2;
	bool have_button = false;

	for(FlowItem2* fi : items)
	{
		if(fi->type != FlowItem2::Button)
		{
			if(fi->type != FlowItem2::Section)
			{
				if(have_button)
				{
					fi->size = GUI.default_font->CalculateSize(fi->text, sizex-2-button_size.x);
					fi->pos = INT2(4+button_size.x, y);
				}
				else
				{
					fi->size = GUI.default_font->CalculateSize(fi->text, sizex);
					fi->pos = INT2(2, y);
				}				
			}
			else
			{
				fi->size = GUI.fBig->CalculateSize(fi->text, sizex);
				fi->pos = INT2(2, y);
			}
			have_button = false;
			y += fi->size.y;
		}
		else
		{
			fi->size = button_size;
			fi->pos = INT2(2, y);
			have_button = true;
		}
	}

	UpdateScrollbar(y);
}

//=================================================================================================
FlowItem2* FlowContainer2::Find(int _group, int _id)
{
	for(FlowItem2* fi : items)
	{
		if(fi->group == _group && fi->id == _id)
			return fi;
	}

	return nullptr;
}

//=================================================================================================
void FlowContainer2::SetItems(vector<FlowItem2*>& _items)
{
	Clear();
	items = std::move(_items);
	Reposition();
	ResetScrollbar();
}

//=================================================================================================
void FlowContainer2::UpdateText(FlowItem2* item, cstring text, bool batch)
{
	assert(item && text);

	item->text = text;
	
	int sizex = (word_warp ? size.x - 20 : 10000);
	INT2 new_size = GUI.default_font->CalculateSize(text, (item->pos.x == 2 ? sizex : sizex - 2 - button_size.x));

	if(new_size.y != item->size.y)
	{
		item->size = new_size;

		if(batch)
			batch_changes = true;
		else
			UpdateText();
	}
	else
	{
		// only width changed, no need to recalculate positions
		item->size = new_size;
	}
}

//=================================================================================================
void FlowContainer2::UpdateText()
{
	batch_changes = false;

	int y = 2;
	bool have_button = false;

	for(FlowItem2* fi : items)
	{
		if(fi->type != FlowItem2::Button)
		{
			if(fi->type != FlowItem2::Section && have_button)
				fi->pos = INT2(4 + button_size.x, y);
			else
				fi->pos = INT2(2, y);
			have_button = false;
			y += fi->size.y;
		}
		else
		{
			fi->pos = INT2(2, y);
			have_button = true;
		}
	}

	scroll.total = y;
}

//=================================================================================================
void FlowContainer2::UpdateScrollbar(int size)
{
	scroll.total = size;
	if(scroll.offset + scroll.part > scroll.total)
		scroll.offset = max(0.f, float(scroll.total - scroll.part));
}
