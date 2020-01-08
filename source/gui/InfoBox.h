#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"

//-----------------------------------------------------------------------------
class InfoBox : public DialogBox
{
public:
	explicit InfoBox(const DialogInfo& info);
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show(cstring text);
};
