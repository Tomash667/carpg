#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "TextBox.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
class GetNumberDialog : public Dialog
{
public:
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

	static GetNumberDialog* Show(Control* parent, DialogEvent event, cstring text, int min_value, int max_value, int* value);

	explicit GetNumberDialog(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	static GetNumberDialog* self;

private:
	int min_value, max_value;
	int* value;
	TextBox textBox;
	Scrollbar scrollbar;
};
