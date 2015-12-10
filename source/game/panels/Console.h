// konsola w grze, otwierana tyld¹
#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "InputTextBox.h"

//-----------------------------------------------------------------------------
class Console : public Dialog, public OnCharHandler
{
public:
	explicit Console(const DialogInfo& info);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	void AddText(cstring str);
	void OnInput(const string& str);

	static TEX tBackground;

	InputTextBox itb;
	bool added;
};
