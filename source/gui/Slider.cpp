#include "Pch.h"
#include "Base.h"
#include "Slider.h"

//=================================================================================================
Slider2::Slider2() : hold(false), minstep(false)
{
	bt[0].text = '<';
	bt[0].id = GuiEvent_Custom;
	bt[0].parent = this;
	bt[0].size = INT2(32,32);

	bt[1].text = '>';
	bt[1].id = GuiEvent_Custom+1;
	bt[1].parent = this;
	bt[1].size = INT2(32,32);
}

//=================================================================================================
void Slider2::Draw(ControlDrawData*)
{
	const int D = 150;

	bt[0].global_pos = bt[0].pos = global_pos;
	bt[1].global_pos = bt[1].pos = bt[0].pos + INT2(D,0);

	for(int i=0; i<2; ++i)
		bt[i].Draw();

	RECT r0 = {global_pos.x+32, global_pos.y-16, global_pos.x+D, global_pos.y+48};
	GUI.DrawText(GUI.default_font, text, DT_CENTER|DT_VCENTER, BLACK, r0);
}

//=================================================================================================
void Slider2::Update(float dt)
{
	for(int i=0; i<2; ++i)
	{
		bt[i].mouse_focus = mouse_focus;
		bt[i].Update(dt);
	}

	if(hold)
	{
		if(hold_state == -1)
		{
			if(bt[0].state == Button::NONE)
				hold_state = 0;
			else
			{
				if(hold_tmp > 0.f)
				hold_tmp = 0.f;
				hold_tmp -= hold_val*dt;
				int ile = (int)ceil(hold_tmp);
				if(ile)
				{
					val += ile;
					if(val < minv)
						val = minv;
					hold_tmp -= ile;
					parent->Event((GuiEvent)id);
					minstep = true;
				}
			}
		}
		else if(hold_state == +1)
		{
			if(bt[1].state == Button::NONE)
				hold_state = 0;
			else
			{
				if(hold_tmp < 0.f)
				   hold_tmp = 0.f;
				hold_tmp += hold_val*dt;
				int ile = (int)floor(hold_tmp);
				if(ile)
				{
					val += ile;
					if(val > maxv)
						val = maxv;
					hold_tmp -= ile;
					parent->Event((GuiEvent)id);
					minstep = true;
				}
			}
		}
	}
}

//=================================================================================================
void Slider2::Event(GuiEvent e)
{
	if(e == GuiEvent_Custom)
	{
		if(val != minv)
		{
			if(hold)
			{
				if(bt[0].state == Button::DOWN)
					hold_state = -1;
				else
				{
					hold_state = 0;
					if(!minstep)
					{
						--val;
						parent->Event((GuiEvent)id);
					}
					minstep = false;
				}
			}
			else
			{
				--val;
				parent->Event((GuiEvent)id);
			}
		}
	}
	else if(e == GuiEvent_Custom+1)
	{
		if(val != maxv)
		{
			if(hold)
			{
				if(bt[1].state == Button::DOWN)
					hold_state = +1;
				else
				{
					hold_state = 0;
					if(!minstep)
					{
						++val;
						parent->Event((GuiEvent)id);
					}
					minstep = false;
				}
			}
			else
			{
				++val;
				parent->Event((GuiEvent)id);
			}
		}
	}
}

//=================================================================================================
void Slider2::SetHold(bool _hold)
{
	hold = _hold;
	for(int i=0; i<2; ++i)
		bt[i].hold = hold;
	hold_tmp = 0.f;
	hold_state = 0;
}
