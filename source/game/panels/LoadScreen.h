#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class LoadScreen : public Control
{
public:
	void Draw(ControlDrawData* cdd=nullptr);

	static TEX tBackground;
};
