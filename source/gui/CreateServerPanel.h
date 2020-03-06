#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <TextBox.h>
#include <GuiContainer.h>
#include <CheckBox.h>

//-----------------------------------------------------------------------------
class CreateServerPanel : public DialogBox
{
	enum Id
	{
		IdOk = GuiEvent_Custom,
		IdCancel,
		IdHidden
	};
public:
	explicit CreateServerPanel(const DialogInfo& info);
	void LoadLanguage();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show();

	TextBox textbox[3];
	CheckBox checkbox;
	cstring txCreateServer, textbox_text[3], txEnterServerName, txInvalidPlayersCount;
	GuiContainer cont;
};
