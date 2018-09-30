#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"

//-----------------------------------------------------------------------------
class InfoBox : public GameDialogBox
{
public:
	explicit InfoBox(const DialogInfo& info);
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show(cstring text);
};
