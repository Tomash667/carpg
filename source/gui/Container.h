#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Container : public Control
{
	friend class IGUI;
public:
	Container() : auto_focus(false), focus_top(false), dont_focus(false)
	{
		focusable = true;
	}
	inline void Add(Control* ctrl)
	{
		assert(ctrl);
		ctrl->parent = this;
		ctrls.push_back(ctrl);
		inside_loop = false;
	}
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	inline bool Empty() const
	{
		return ctrls.empty();
	}
	inline Control* Top()
	{
		assert(!Empty());
		return ctrls.back();
	}
	void Remove(Control* ctrl);
	inline vector<Control*>& GetControls()
	{
		return ctrls;
	}
	inline bool AnythingVisible() const
	{
		for(vector<Control*>::const_iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible)
				return true;
		}
		return false;
	}
	inline virtual bool NeedCursor() const
	{
		for(vector<Control*>::const_iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible && (*it)->NeedCursor())
				return true;
		}
		return false;
	}
	void DeleteItems();

	bool auto_focus, focus_top, dont_focus;

protected:
	vector<Control*> ctrls;
	bool inside_loop;
};
