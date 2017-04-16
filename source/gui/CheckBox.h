#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class CheckBox : public Control
{
public:
	enum State
	{
		NONE,
		FLASH,
		PRESSED,
		DISABLED
	};

	CheckBox(StringOrCstring text = "", bool checked = false);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	static TEX tTick;
	string text;
	int id;
	State state;
	INT2 bt_size;
	bool checked, radiobox;
};
