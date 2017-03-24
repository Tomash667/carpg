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

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void AddText(cstring str);
	void OnInput(const string& str);

	static TEX tBackground;
	InputTextBox itb;
	bool added;
};
