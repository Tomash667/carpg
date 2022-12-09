#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <TextBox.h>

//-----------------------------------------------------------------------------
// Panel to select multiplayer game (create server, join server, load game)
class MultiplayerPanel : public DialogBox
{
public:
	explicit MultiplayerPanel(const DialogInfo& info);
	void LoadLanguage();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show();

	enum ButtonId
	{
		IdJoinLan = GuiEvent_Custom,
		IdJoinIp,
		IdCreate,
		IdLoad,
		IdCancel
	};

	TextBox textbox;
	cstring txMultiplayerGame, txNick, txNeedEnterNick, txEnterValidNick;
};
