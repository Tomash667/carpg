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

	CheckBox(StringOrCstring text="", bool checked=false);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);

	string text;
	int id;
	State state;
	INT2 bt_size;
	bool checked, radiobox;

	static TEX tTick;
};
