#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "InputTextBox.h"

//-----------------------------------------------------------------------------
class MpBox : public GamePanel
{
public:
	MpBox();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return itb.focus; }

	void Reset();
	void OnInput(const string& str);

	InputTextBox itb;
	bool have_focus;
};
