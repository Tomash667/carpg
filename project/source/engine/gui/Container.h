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
	Container()
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

protected:
	void OnInitialize() override;

	vector<Control*> ctrls;
	bool inside_loop;
};
