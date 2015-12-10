#include "Pch.h"
#include "Base.h"
#include "Container.h"

//=================================================================================================
void Container::Draw(ControlDrawData* cdd)
{
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->visible)
		{
			if(auto_focus)
				(*it)->focus = focus;
			(*it)->Draw(nullptr);
		}
	}
}

//=================================================================================================
void Container::Update(float dt)
{
	inside_loop = true;

	if(dont_focus)
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
	if(e == GuiEvent_WindowResize)
	{
		for each(Control* ctrl in ctrls)
			ctrl->Event(GuiEvent_WindowResize);
	}
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

//=================================================================================================
void Container::DeleteItems()
{
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		delete *it;
}
