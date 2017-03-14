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
	Container(bool new_mode = false) : Control(new_mode), auto_focus(false), focus_top(false), dont_focus(false), new_mode(new_mode)
	{
		focusable = true;
	}
	~Container();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override;
	void SetDisabled(bool new_disabled) override;

	void Add(Control* ctrl);
	bool AnythingVisible() const;
	bool Empty() const { return ctrls.empty(); }
	vector<Control*>& GetControls() { return ctrls; }
	void Remove(Control* ctrl);
	Control* Top()
	{
		assert(!Empty());
		return ctrls.back();
	}
	
	bool auto_focus, focus_top, dont_focus;

protected:
	vector<Control*> ctrls;
	bool inside_loop, new_mode;
};
