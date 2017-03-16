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

	// from Control
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;

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
	InputEvent event;
	TEX* background;
	DWORD background_color;
	bool added, lose_focus, esc_clear;

private:
	float caret_blink;
};
