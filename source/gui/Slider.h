#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Button.h"

//-----------------------------------------------------------------------------
class Slider2 : public Control
{
public:
	Slider2();
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	void SetHold(bool hold);

	int minv, maxv, val, id;
	string text;
	Button bt[2];
	float hold_val;

private:
	bool hold, minstep;
	float hold_tmp;
	int hold_state;
};
