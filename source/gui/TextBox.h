#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Scrollbar;

//-----------------------------------------------------------------------------
class TextBox : public Control, public OnCharHandler
{
public:
	TextBox();
	~TextBox();
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);
	void Event(GuiEvent e);
	void OnChar(char c);
	
	void ValidateNumber();
	void AddScrollbar();
	void Move(const INT2& global_pos);
	void Add(cstring str);
	void Reset();
	void UpdateScrollbar();

	string text;
	float kursor_mig;
	int limit, num_min, num_max;
	bool added, numeric, multiline, readonly;
	static TEX tBox;
	cstring label;
	Scrollbar* scrollbar;
};
