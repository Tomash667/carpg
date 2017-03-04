#pragma once

#include "Control.h"

namespace gui
{
	struct BaseEventData
	{
		Control* control;

		inline BaseEventData(Control* control) : control(control) {}
	};

	struct KeyEventData : public BaseEventData
	{
		byte key;
		bool pressed;

		inline KeyEventData(Control* control, byte key, bool pressed) : BaseEventData(control), key(key), pressed(pressed) {}
	};

	struct MouseEventData : public BaseEventData
	{
		byte button;
		bool pressed;

		inline MouseEventData(Control* control, byte button, bool pressed) : BaseEventData(control), button(button), pressed(pressed) {}
	};

	typedef delegate<void(KeyEventData&)> KeyEvent;
	typedef delegate<void(MouseEventData&)> MouseEvent;
}
