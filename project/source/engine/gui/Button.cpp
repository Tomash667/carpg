#include "Pch.h"
#include "Core.h"
#include "Button.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
TEX Button::tex[4];

//=================================================================================================
Button::Button() : state(NONE), img(nullptr), hold(false), force_img_size(0, 0), custom(nullptr)
{
}

//=================================================================================================
void Button::Draw(ControlDrawData*)
{
	State real_state = state;
	if(disabled)
		real_state = DISABLED;

	if(!custom)
	{
		GUI.DrawItem(tex[real_state], global_pos, size, WHITE, 16);

		Rect r = {
			global_pos.x + 4,
			global_pos.y + 4,
			global_pos.x + size.x - 8,
			global_pos.y + size.y - 8
		};

		if(state == DOWN)
			r += Int2(1, 1);

		if(img)
		{
			Int2 pt = r.LeftTop();
			if(state == DOWN)
			{
				++pt.x;
				++pt.y;
			}

			Matrix mat;
			Int2 required_size = force_img_size, img_size;
			Vec2 scale;
			Control::ResizeImage(img, required_size, img_size, scale);
			mat = Matrix::Transform2D(&Vec2(float(img_size.x) / 2, float(img_size.y) / 2), 0.f, &scale, nullptr, 0.f,
				&Vec2((float)r.Left(), float(r.Top() + (required_size.y - img_size.y) / 2)));
			GUI.DrawSprite2(img, mat, nullptr, &r, WHITE);
			r.Left() += img_size.x;
		}

		GUI.DrawText(GUI.default_font, text, DT_CENTER | DT_VCENTER, BLACK, r, &r);
	}
	else
		GUI.DrawItem(custom->tex[real_state], global_pos, size, WHITE, 16);
}

//=================================================================================================
void Button::Update(float dt)
{
	if(state == DISABLED || disabled)
		return;

	if(Key.Focus() && mouse_focus && IsInside(GUI.cursor_pos))
	{
		GUI.cursor_mode = CURSOR_HAND;
		if(state == DOWN)
		{
			bool apply = false;
			if(Key.Up(VK_LBUTTON))
			{
				state = HOVER;
				apply = true;
			}
			else if(hold)
				apply = true;
			if(apply)
			{
				if(handler)
					handler(id);
				else
					parent->Event((GuiEvent)id);
			}
		}
		else if(Key.Pressed(VK_LBUTTON))
			state = DOWN;
		else
			state = HOVER;
	}
	else
		state = NONE;
}
