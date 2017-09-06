#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"

//-----------------------------------------------------------------------------
struct Game;

//-----------------------------------------------------------------------------
class GameDialogBox : public DialogBox
{
public:
	explicit GameDialogBox(const DialogInfo& info) : DialogBox(info)
	{
	}

	static Game* game;
};
