#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include <InputTextBox.h>

//-----------------------------------------------------------------------------
// Multiplayer chat
class MpBox : public GamePanel
{
public:
	MpBox();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return itb.focus; }
	void Reset();
	void OnInput(const string& str);
	void Add(cstring text) { itb.Add(text); }

	InputTextBox itb;
	bool haveFocus;
};
