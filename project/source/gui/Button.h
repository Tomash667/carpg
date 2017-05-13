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

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	DialogEvent GetHandler() const { return handler; }

	void SetHandler(DialogEvent new_handler) { handler = new_handler; }

	static TEX tex[4];
	string text;
	State state;
	int id;
	TEX img;
	INT2 force_img_size;
	CustomButton* custom;
	bool hold;

private:
	DialogEvent handler;
};
