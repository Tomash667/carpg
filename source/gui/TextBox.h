#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Scrollbar;

//-----------------------------------------------------------------------------
class TextBox : public Control, public OnCharHandler
{
public:
	explicit TextBox(bool v2 = false);
	~TextBox();
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	void OnChar(char c);
	
	void ValidateNumber();
	void AddScrollbar();
	void Move(const INT2& global_pos);
	void Add(cstring str);
	void Reset();
	void UpdateScrollbar();
	void UpdateSize(const INT2& pos, const INT2& size);

	string text;
	float kursor_mig;
	int limit, num_min, num_max;
	bool added, numeric, multiline, readonly;
	static TEX tBox;
	cstring label;
	Scrollbar* scrollbar;

private:
	bool v2;
};
