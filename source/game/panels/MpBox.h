#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "InputTextBox.h"

//-----------------------------------------------------------------------------
class MpBox : public GamePanel
{
public:
	MpBox();
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);
	void Event(GuiEvent e);
	bool NeedCursor() const { return itb.focus || GamePanel::menu.visible; }

	void Reset();
	void OnInput(const string& str);

	InputTextBox itb;
	bool have_focus;
};
