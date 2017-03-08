#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
typedef delegate<void(const string&)> InputEvent;

//-----------------------------------------------------------------------------
class InputTextBox : public Control, public OnCharHandler
{
public:
	InputTextBox();
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	void OnChar(char c);

	void Init();
	void Reset();
	void Add(StringOrCstring str);
	void CheckLines();

	string text, input;
	vector<TextLine> lines;
	vector<string> cache;
	int max_lines, max_cache, input_counter, last_input_counter;
	Scrollbar scrollbar;
	INT2 textbox_size, inputbox_size, inputbox_pos;
	float kursor_mig;
	bool added, lose_focus, esc_clear;
	InputEvent event;
	TEX* background;
	DWORD background_color;
};
