#include "Pch.h"
#include "Core.h"
#include "Container.h"

//=================================================================================================
Container::~Container()
{
	DeleteElements(ctrls);
}

//=================================================================================================
void Container::Add(Control* ctrl)
{
	assert(ctrl);
	ctrl->parent = this;
	ctrls.push_back(ctrl);
	inside_loop = false;

	if(IsInitialized())
		ctrl->Initialize();

	if(ctrl->IsDocked())
		Dock(ctrl);
}

//=================================================================================================
bool Container::AnythingVisible() const
{
	for(vector<Control*>::const_iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->visible)
			return true;
	}
	return false;
}

//=================================================================================================
void Container::Draw(ControlDrawData* cdd)
{
	for(Control* c : ctrls)
	{
		if(!c->visible)
			continue;

		c->Draw(cdd);
	}
}

//=================================================================================================
void Container::Update(float dt)
{
	inside_loop = true;

	for(vector<Control*>::reverse_iterator it = ctrls.rbegin(), end = ctrls.rend(); it != end; ++it)
	{
		Control& c = **it;
		if(!c.visible)
			continue;
		UpdateControl(&c, dt);
		if(!inside_loop)
			return;
	}

	inside_loop = false;
}

//=================================================================================================
void Container::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		for(Control* c : ctrls)
			c->Initialize();
		break;
	case GuiEvent_WindowResize:
	case GuiEvent_Show:
	case GuiEvent_Hide:
		for(Control* c : ctrls)
			c->Event(e);
		break;
	case GuiEvent_Moved:
		for(Control* c : ctrls)
		{
			c->global_pos = c->pos + global_pos;
			c->Event(GuiEvent_Moved);
		}
		break;
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
void Container::SetDisabled(bool new_disabled)
{
	if(new_disabled == disabled)
		return;

	for(Control* c : ctrls)
		c->SetDisabled(new_disabled);
	disabled = new_disabled;
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
