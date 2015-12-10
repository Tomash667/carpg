#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"

//-----------------------------------------------------------------------------
class InfoBox : public Dialog
{
public:
	explicit InfoBox(const DialogInfo& info);
	void Draw(ControlDrawData* cdd = nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	void Show(cstring text);
};
