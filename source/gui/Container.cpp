#include "Pch.h"
#include "Base.h"
#include "Container.h"

//=================================================================================================
Container::~Container()
{
	if(new_mode)
		DeleteElements(ctrls);
}

//=================================================================================================
void Container::Draw(ControlDrawData* cdd)
{
	for(Control* c : ctrls)
	{
		if(!c->visible)
			continue;

		if(auto_focus)
			c->focus = focus;
		c->Draw(cdd);
	}
}

//=================================================================================================
void Container::Update(float dt)
{
	inside_loop = true;

	if(new_mode)
	{
		for(vector<Control*>::reverse_iterator it = ctrls.rbegin(), end = ctrls.rend(); it != end; ++it)
		{
			Control& c = **it;
			if(!c.visible)
				continue;
			c.mouse_focus = mouse_focus;
			c.Update(dt);
			if(!c.mouse_focus)
				mouse_focus = false;
			if(!inside_loop)
				return;
		}
	}
	else if(dont_focus)
	{
		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible)
			{
				(*it)->LostFocus();
				(*it)->Update(dt);
				if(!inside_loop)
					return;
			}
		}
	}
	else if(focus_top)
	{
		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible)
			{
				if(focus && (*it)->focusable && it == end-1)
					(*it)->GainFocus();
				else
					(*it)->LostFocus();
				(*it)->Update(dt);
				if(!inside_loop)
					return;
			}
		}
	}
	else
	{
		Control* top = nullptr;

		for(Control* c : ctrls)
		{
			if(c->visible)
			{
				if(focus && c->focusable && !top)
					top = c;
			}
		}

		for(Control* c : ctrls)
		{
			__assume(c != nullptr);
			if(c->visible)
			{
				if(c == top)
					c->GainFocus();
				else
					c->LostFocus();
				c->Update(dt);
				if(!inside_loop)
					return;
			}
		}
	}

	inside_loop = false;
}

//=================================================================================================
void Container::Event(GuiEvent e)
{
	if(new_mode)
	{
		switch(e)
		{
		case GuiEvent_Initialize:
			for(Control* c : ctrls)
				c->Initialize();
			break;
		case GuiEvent_WindowResize:
		case GuiEvent_Show:
			for(Control* c : ctrls)
				c->Event(e);
			break;
		}
	}
	else if(e == GuiEvent_WindowResize)
	{
		for(Control* ctrl : ctrls)
			ctrl->Event(GuiEvent_WindowResize);
	}
}

//=================================================================================================
bool Container::NeedCursor() const
{
	for(const Control* c : ctrls)
	{
		if(c->visible && c->NeedCursor())
			return true;
	}
	return false;
}

//=================================================================================================
void Container::Remove(Control* ctrl)
{
	assert(ctrl);

	if(ctrls.back() == ctrl)
	{
		ctrls.pop_back();
		if(!ctrls.empty())
		{
			ctrls.back()->focus = true;
			ctrls.back()->Event(GuiEvent_GainFocus);
		}
	}
	else
		RemoveElementOrder(ctrls, ctrl);

	inside_loop = false;
}
