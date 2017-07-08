#include "Pch.h"
#include "Base.h"
#include "DrawBox.h"
#include "KeyStates.h"
#include "ResourceManager.h"
#include "SceneManager.h"

using namespace gui;

DrawBox::DrawBox() : Control(true), mesh(nullptr), tex(nullptr), clicked(false)
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
	else if(mesh)
	{
		auto tex = scene->RenderToTexture();
		GUI.DrawSprite(tex, global_pos, WHITE, &r);
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

	if(focus)
	{
		if(Key.PressedRelease('R'))
		{
			if(tex)
			{
				move = VEC2(0, 0);
				scale = default_scale;
				clicked = false;
			}
		}
	}
}

void DrawBox::SetTexture(TEX t)
{
	assert(t);
	tex = t;
	mesh = nullptr;

	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);

	tex_size = INT2(desc.Width, desc.Height);
	VEC2 sizef = size.ToVEC2();
	VEC2 scale2 = VEC2(sizef.x / tex_size.x, sizef.y / tex_size.y);
	scale = min(scale2.x, scale2.y);
	default_scale = scale;
	move = VEC2(0, 0);
}

void DrawBox::SetMesh(Mesh* m)
{
	assert(m);
	mesh = m;
	tex = nullptr;

	if(!scene)
	{
		scene = SceneManager::Get().CreateScene();
		node = new SceneNode2;
		node->mesh = m;
		node->pos = VEC3(0, 0, 0);
		scene->Add(node);
		scene->SetForRenderTarget(size);

		auto& camera = scene->GetCamera();
		camera.fov = PI / 4;
		camera.aspect = float(size.x) / size.y;
		camera.zmin = 0.05f;
		camera.zmax = 50.f;
		camera.up = VEC3(0, 1, 0);
	}

	node->mesh = m;
	auto& camera = scene->GetCamera();
	camera.from = VEC3(5, 5, 5);
	camera.to = VEC3(0, 0, 0);
}
