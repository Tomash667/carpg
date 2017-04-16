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

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Show();

	TextBox textbox[3];
	cstring txCreateServer, textbox_text[3], txEnterServerName, txInvalidPlayersCount;
	GuiContainer cont;
};
