#include "Pch.h"
#include "Base.h"
#include "Scrollbar.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
TEX Scrollbar::tex;
TEX Scrollbar::tex2;

//=================================================================================================
Scrollbar::Scrollbar(bool hscrollbar, bool is_new) : Control(is_new), clicked(false), hscrollbar(hscrollbar), manual_change(false), offset(0.f)
{

}

//=================================================================================================
void Scrollbar::Draw(ControlDrawData* cdd)
{
	GUI.DrawItem(tex, global_pos, size, WHITE, 4, 32);

	int s_pos, s_size;
	if(hscrollbar)
	{
		if(part >= total)
		{
			s_pos = 0;
			s_size = size.x;
		}
		else
		{
			s_pos = int(float(offset)/total*size.x);
			s_size = int(float(part)/total*size.x);
		}
		GUI.DrawItem(tex2, INT2(global_pos.x+s_pos, global_pos.y), INT2(s_size, size.y), WHITE, 4, 32);
	}
	else
	{
		if(part >= total)
		{
			s_pos = 0;
			s_size = size.y;
		}
		else
		{
			s_pos = int(float(offset)/total*size.y);
			s_size = int(float(part)/total*size.y);
		}
		GUI.DrawItem(tex2, INT2(global_pos.x, global_pos.y+s_pos), INT2(size.x, s_size), WHITE, 4, 32);
	}
}

//=================================================================================================
void Scrollbar::Update(float dt)
{
	if(!Key.Focus())
		return;

	change = 0;

	INT2 cpos = GUI.cursor_pos - global_pos;

	if(clicked)
	{
		if(Key.Up(VK_LBUTTON))
			clicked = false;
		else
		{
			if(hscrollbar)
			{
				int dif = cpos.x - click_pt.x;
				float move = float(dif)*total/size.x;
				bool changed = true;
				if(offset + move < 0)
					move = -offset;
				else if(offset+move+float(part) > float(total))
					move = float(max(0,total-part))-offset;
				else
					changed = false;
				offset += move;
				if(changed)
					click_pt.x += int(move/total*size.x);
				else
					click_pt.x = cpos.x;
			}
			else
			{
				int dif = cpos.y - click_pt.y;
				float move = float(dif)*total/size.y;
				bool changed = true;
				if(offset + move < 0)
					move = -offset;
				else if(offset+move+float(part) > float(total))
					move = float(max(0,total-part))-offset;
				else
					changed = false;
				offset += move;
				if(changed)
					click_pt.y += int(move/total*size.y);
				else
					click_pt.y = cpos.y;
			}
		}
	}
	else if((!is_new || mouse_focus) && Key.Pressed(VK_LBUTTON))
	{
		if((is_new && mouse_focus) || (cpos.x >= 0 && cpos.y >= 0 && cpos.x < size.x && cpos.y < size.y))
		{
			int pos_o = hscrollbar ? int(float(cpos.x)*total/size.x) : int(float(cpos.y)*total/size.y);
			if(hscrollbar ? (pos_o >= offset && pos_o < offset+part) : (pos_o+2 >= offset && pos_o+2 < offset+part))
			{
				Key.SetState(VK_LBUTTON, IS_DOWN);
				clicked = true;
				click_pt = cpos;
			}
			else
			{
				Key.SetState(VK_LBUTTON, IS_UP);
				if(pos_o < offset)
				{
					if(!manual_change)
					{
						offset -= part;
						if(offset < 0)
							offset = 0;
					}
					else
						change = -1;
				}
				else
				{
					if(!manual_change)
					{
						offset += part;
						if(offset+part > total)
							offset = float(total-part);
					}
					else
						change = 1;
				}
			}
		}

		if(is_new)
			TakeFocus(true);
	}
}

//=================================================================================================
void Scrollbar::LostFocus()
{
	clicked = false;
}

//=================================================================================================
bool Scrollbar::ApplyMouseWheel()
{
	if(GUI.mouse_wheel != 0.f)
	{
		LostFocus();
		float mod = (!is_new ? (Key.Down(VK_SHIFT) ? 1.f : 0.2f) : 0.2f);
		float prev_offset = offset;
		offset -= part*GUI.mouse_wheel*mod;
		if(offset < 0.f)
			offset = 0.f;
		else if(offset+part > total)
			offset = max(0.f, float(total-part));
		return !equal(offset, prev_offset);
	}
	else
		return false;
}

//=================================================================================================
void Scrollbar::UpdateTotal(int new_total)
{
	total = new_total;
	if(offset + part > total)
		offset = max(0.f, float(total - part));
}

//=================================================================================================
void Scrollbar::UpdateOffset(float _change)
{
	offset += _change;
	if(offset < 0)
		offset = 0;
	else if(offset + part > total)
		offset = max(0.f, float(total - part));
}
