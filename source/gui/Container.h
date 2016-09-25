#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
// Gui controls container
// new_mode - delete childs, different auto focus
class Container : public Control
{
	friend class IGUI;
public:
	Container(bool new_mode = false) : auto_focus(false), focus_top(false), dont_focus(false), new_mode(new_mode)
	{
		focusable = true;
	}
	~Container();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override;

	inline void Add(Control* ctrl)
	{
		assert(ctrl);
		ctrl->parent = this;
		ctrls.push_back(ctrl);
		inside_loop = false;
	}
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

	bool auto_focus, focus_top, dont_focus;

protected:
	vector<Control*> ctrls;
	bool inside_loop, new_mode;
};
