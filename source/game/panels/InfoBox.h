#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"

//-----------------------------------------------------------------------------
class InfoBox : public Dialog
{
public:
	InfoBox(const DialogInfo& info);
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Event(GuiEvent e);

	void Show(cstring text);
};
