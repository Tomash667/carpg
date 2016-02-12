#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "InputTextBox.h"

//-----------------------------------------------------------------------------
class MpBox : public GamePanel
{
public:
	MpBox();

	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	bool NeedCursor() const { return itb.focus; }
	void Reset();
	void OnInput(const string& str);

	InputTextBox itb;
	bool have_focus;
};
