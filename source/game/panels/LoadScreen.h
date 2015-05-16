#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class LoadScreen : public Control
{
public:
	void Draw(ControlDrawData* cdd=NULL);

	static TEX tBackground;
};
