#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
class MultiplayerPanel : public GameDialogBox
{
public:
	explicit MultiplayerPanel(const DialogInfo& info);
	void LoadLanguage();
	void Draw(ControlDrawData* cdd = nullptr) override;
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
