// menu otwierane w grze po wciœniêciu ESC
#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "Button.h"

//-----------------------------------------------------------------------------
class GameMenu : public DialogBox
{
public:
	enum ButtonId
	{
		IdReturnToGame = GuiEvent_Custom,
		IdSaveGame,
		IdLoadGame,
		IdOptions,
		IdExit,
		IdQuit
	};

	explicit GameMenu(const DialogInfo& info);
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	TEX tLogo;
	Button bt[6];
	cstring txSave, txSaveAndExit, txExitToMenuDialog;
	bool prev_can_save, prev_can_load, prev_hardcore_mode;
};
