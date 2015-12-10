// menu otwierane w grze po wciœniêciu ESC
#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "Button.h"

//-----------------------------------------------------------------------------
class GameMenu : public Dialog
{
public:
	explicit GameMenu(const DialogInfo& info);
	void Draw(ControlDrawData* cdd/* =nullptr */);
	void Update(float dt);
	void Event(GuiEvent e);

	void Set(bool can_save, bool can_load, bool hardcore_mode);

	enum ButtonId
	{
		IdReturnToGame = GuiEvent_Custom,
		IdSaveGame,
		IdLoadGame,
		IdOptions,
		IdExit,
		IdQuit
	};

	Button bt[6];
	cstring txSave, txSaveAndExit, txExitToMenuDialog;
	bool prev_can_save, prev_can_load, prev_hardcore_mode;

	static TEX tLogo;
};
