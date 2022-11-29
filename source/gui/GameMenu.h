#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <Button.h>

//-----------------------------------------------------------------------------
// Ingame menu opened by ESC key
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
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	void CheckButtons();

	Texture* tLogo;
	Button bt[6];
	cstring txSave, txSaveAndExit, txExitToMenuDialog, txExitToMenuDialogHardcore;
	bool prev_can_save, prev_can_load, prev_hardcore_mode;
};
