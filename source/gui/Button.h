#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
struct CustomButton
{
	TEX tex[4];
};

//-----------------------------------------------------------------------------
class Button : public Control
{
public:
	enum State
	{
		NONE,
		HOVER,
		DOWN,
		DISABLED
	};

	Button();
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);

	string text;
	State state;
	int id;
	TEX img;
	bool hold;
	INT2 force_img_size;
	CustomButton* custom;

	static TEX tex[4];
};
