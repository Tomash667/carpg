#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
struct GetTextDialogParams
{
	GetTextDialogParams(cstring text, string& _input) : text(text), input(&_input), parent(nullptr), event(nullptr), limit(0), lines(1), width(300), custom_names(nullptr), multiline(false)
	{

	}

	Control* parent;
	DialogEvent event;
	cstring text;
	string* input;
	int limit, lines, width;
	cstring* custom_names;
	bool multiline;
};

//-----------------------------------------------------------------------------
class GetTextDialog : public Dialog
{
public:
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

	explicit GetTextDialog(const DialogInfo& info);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	static GetTextDialog* Show(const GetTextDialogParams& params);

	static GetTextDialog* self;

private:
	void Create(const GetTextDialogParams& params);

	string* input;
	TextBox textBox;
	bool singleline;
};
