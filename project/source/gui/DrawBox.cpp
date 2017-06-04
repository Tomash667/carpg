#include "Pch.h"
#include "Base.h"
#include "DrawBox.h"
#include "KeyStates.h"

using namespace gui;

DrawBox::DrawBox() : Control(true), tex(nullptr), clicked(false)
{

}

void DrawBox::Draw(ControlDrawData*)
{
	RECT r = Rect::CreateRECT(global_pos, size);
	GUI.DrawArea(COLOR_RGB(150,150,150), r);

	if(tex)
	{
		MATRIX m;
		VEC2 scaled_tex_size = tex_size.ToVEC2() * scale;
		VEC2 max_pos = scaled_tex_size - size.ToVEC2();
		VEC2 p = VEC2(max_pos.x * -move.x / 100, max_pos.y * -move.y / 100) + global_pos.ToVEC2();
		D3DXMatrixTransformation2D(&m, nullptr, 0.f, &VEC2(scale, scale), nullptr, 0.f, &p);
		GUI.DrawSprite2(tex, &m, nullptr, &r);
	}
}

void DrawBox::Update(float dt)
{
	if(mouse_focus)
	{
		float change = GUI.mouse_wheel;
		if(change > 0)
		{
			while(change > 0)
			{
				float prev_scale = scale;
				scale *= 1.1f;
				if(prev_scale < default_scale && scale > default_scale)
					scale = default_scale;
				change -= 1.f;
			}
		}
		else if(change < 0)
		{
			change = -change;
			while(change > 0)
			{
				float prev_scale = scale;
				scale *= 0.9f;
				if(prev_scale > default_scale && scale < default_scale)
					scale = default_scale;
				change -= 1.f;
			}
		}

		if(!clicked && Key.Down(VK_LBUTTON))
		{
			clicked = true;
			click_point = GUI.cursor_pos;
		}
	}

	if(clicked)
	{
		if(Key.Up(VK_LBUTTON))
			clicked = false;
		else
		{
			INT2 dif = click_point - GUI.cursor_pos;
			GUI.cursor_pos = click_point;
			move -= dif.ToVEC2() / 2;
			move.x = clamp(move.x, 0.f, 100.f);
			move.y = clamp(move.y, 0.f, 100.f);
		}
	}
}

void DrawBox::SetTexture(TEX t)
{
	assert(t);
	tex = t;

	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);

	tex_size = INT2(desc.Width, desc.Height);
	VEC2 sizef = size.ToVEC2();
	VEC2 scale2 = VEC2(sizef.x / tex_size.x, sizef.y / tex_size.y);
	scale = min(scale2.x, scale2.y);
	default_scale = scale;
	move = VEC2(0, 0);
}
