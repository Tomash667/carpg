#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <TextBox.h>
#include <GuiContainer.h>
#include <CheckBox.h>

//-----------------------------------------------------------------------------
// Panel shown when creating new server
class CreateServerPanel : public DialogBox
{
	enum Id
	{
		IdOk = GuiEvent_Custom,
		IdOkConfirm,
		IdCancel,
		IdHidden
	};
public:
	explicit CreateServerPanel(const DialogInfo& info);
	void LoadLanguage();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show();

	TextBox textbox[3];
	CheckBox checkbox;
	cstring txCreateServer, textbox_text[3], txEnterServerName, txInvalidPlayersCount, txConfirmMaxPlayers;
	GuiContainer cont;
};
