#include "Pch.h"
#include "Base.h"
#include "Button.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
TEX Button::tex[4];

//=================================================================================================
Button::Button() : state(NONE), img(NULL), hold(false)
{

}

//=================================================================================================
void Button::Draw(ControlDrawData*)
{
	GUI.DrawItem(tex[state], global_pos, size, WHITE, 16);

	RECT r = {
		global_pos.x+4,
		global_pos.y+4,
		global_pos.x+size.x-8,
		global_pos.y+size.y-8
	};

	if(state == PRESSED)
	{
		++r.left;
		++r.right;
		++r.bottom;
		++r.top;
	}

	GUI.DrawText(GUI.default_font, text, DT_CENTER|DT_VCENTER, BLACK, r, &r);

	if(img)
	{
		INT2 pt(global_pos.x+6,global_pos.y+6);
		if(state == PRESSED)
		{
			++pt.x;
			++pt.y;
		}

		GUI.DrawSprite(img, pt);
	}
}

//=================================================================================================
void Button::Update(float dt)
{
	if(state == DISABLED)
		return;

	if(Key.Focus() && mouse_focus && IsInside(GUI.cursor_pos))
	{
		GUI.cursor_mode = CURSOR_HAND;
		if(state == PRESSED)
		{
			if(Key.Up(VK_LBUTTON))
			{
				state = FLASH;
				parent->Event((GuiEvent)id);
			}
			else if(hold)
				parent->Event((GuiEvent)id);
		}
		else if(Key.Pressed(VK_LBUTTON))
			state = PRESSED;
		else
			state = FLASH;
	}
	else
		state = NONE;
}
