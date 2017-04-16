#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Button.h"

//-----------------------------------------------------------------------------
class Slider : public Control
{
public:
	Slider();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void SetHold(bool hold);

	int minv, maxv, val, id;
	string text;
	Button bt[2];
	float hold_val;

private:
	float hold_tmp;
	int hold_state;
	bool hold, minstep;
};
