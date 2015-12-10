#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "TextBox.h"
#include "GuiContainer.h"

//-----------------------------------------------------------------------------
class CreateServerPanel : public Dialog
{
public:
	explicit CreateServerPanel(const DialogInfo& info);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	void Show();

	TextBox textbox[3];
	cstring txCreateServer, textbox_text[3], txEnterServerName, txInvalidPlayersCount;
	GuiContainer cont;
};
