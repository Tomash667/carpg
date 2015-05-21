#include "Pch.h"
#include "Base.h"
#include "Button.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
TEX Button::tex[4];

//=================================================================================================
Button::Button() : state(NONE), img(NULL), hold(false), force_img_size(0,0)
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

	if(img)
	{
		INT2 pt(r.left, r.top);
		if(state == PRESSED)
		{
			++pt.x;
			++pt.y;
		}

		MATRIX mat;
		INT2 size = force_img_size, img_size;
		VEC2 scale;
		Control::ResizeImage(img, size, img_size, scale);
		D3DXMatrixTransformation2D(&mat, &VEC2(float(img_size.x) / 2, float(img_size.y) / 2), 0.f, &scale, NULL, 0.f, &VEC2((float)r.left, float(r.top + (size.y - img_size.y) / 2)));
		GUI.DrawSprite2(img, &mat, NULL, &r, WHITE);
		r.left += img_size.x;
	}

	GUI.DrawText(GUI.default_font, text, DT_CENTER | DT_VCENTER, BLACK, r, &r);
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
