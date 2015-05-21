#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Button : public Control
{
public:
	enum State
	{
		NONE,
		FLASH,
		PRESSED,
		DISABLED
	};

	Button();
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);

	string text;
	State state;
	int id;
	TEX img;
	bool hold;
	INT2 force_img_size;

	static TEX tex[4];
};
