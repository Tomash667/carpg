#include "Pch.h"
#include "Core.h"
#include "Control.h"
#include "Overlay.h"

void Control::Dock(Control* c)
{
	assert(c);
	if(c->global_pos != global_pos)
	{
		c->global_pos = global_pos;
		if(c->IsInitialized())
			c->Event(GuiEvent_Moved);
	}
	if(c->size != size)
	{
		c->size = size;
		if(c->IsInitialized())
			c->Event(GuiEvent_Resize);
	}
}

void Control::Initialize()
{
	if(initialized)
		return;
	initialized = true;
	Event(GuiEvent_Initialize);
}

void Control::SetSize(const Int2& new_size)
{
	if(size == new_size || IsDocked())
		return;
	size = new_size;
	if(initialized)
		Event(GuiEvent_Resize);
}

void Control::SetPosition(const Int2& new_pos)
{
	if(pos == new_pos || IsDocked())
		return;
	global_pos -= pos;
	pos = new_pos;
	global_pos += pos;
	if(initialized)
		Event(GuiEvent_Moved);
}

void Control::SetDocked(bool new_docked)
{
	if(IsDocked() == new_docked)
		return;
	SET_BIT_VALUE(flags, F_DOCKED, new_docked);
	if(new_docked)
	{
		if(parent)
			parent->Dock(this);
	}
	else
	{
		Int2 new_global_pos = parent->global_pos + pos;
		if(new_global_pos != global_pos)
		{
			global_pos = new_global_pos;
			Event(GuiEvent_Moved);
		}
	}
}

void Control::TakeFocus(bool pressed)
{
	GUI.GetOverlay()->CheckFocus(this, pressed);
}

void Control::SetFocus()
{
	GUI.GetOverlay()->SetFocus(this);
}

void Control::UpdateControl(Control* ctrl, float dt)
{
	if(mouse_focus && ctrl->IsInside(GUI.cursor_pos))
	{
		ctrl->mouse_focus = true;
		ctrl->Update(dt);
		if(!ctrl->mouse_focus)
			mouse_focus = false;
		else
			ctrl->TakeFocus();
	}
	else
	{
		ctrl->mouse_focus = false;
		ctrl->Update(dt);
	}
}

void Control::ResizeImage(TEX t, Int2& new_size, Int2& img_size, Vec2& scale)
{
	D3DSURFACE_DESC desc;
	t->GetLevelDesc(0, &desc);
	img_size = Int2(desc.Width, desc.Height);
	if(new_size == Int2(0, 0))
	{
		new_size = img_size;
		scale = Vec2(1, 1);
	}
	else if(new_size == img_size)
		scale = Vec2(1, 1);
	else
		scale = Vec2(float(new_size.x) / img_size.x, float(new_size.y) / img_size.y);
}