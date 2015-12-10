#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class StaticText : public Control
{
public:
	StaticText(cstring str=nullptr, Font* font=GUI.default_font, DWORD color=BLACK) : font(font), color(color), text(str ? str : "")
	{
	}
	void Draw(ControlDrawData* cdd);

	Font* font;
	string text;
	DWORD color;
};
