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

	// from Control
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;

	void ValidateNumber();
	void AddScrollbar();
	void Move(const INT2& global_pos);
	void Add(cstring str);
	void Reset();
	void UpdateScrollbar();
	void UpdateSize(const INT2& pos, const INT2& size);

	static TEX tBox;
	string text;
	float kursor_mig;
	int limit, num_min, num_max;
	cstring label;
	Scrollbar* scrollbar;
	bool added, numeric, multiline, readonly;

private:
	bool v2;
};
