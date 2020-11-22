#include "Pch.h"
#include "Guild.h"

#include "DialogContext.h"
#include "GameGui.h"
#include "GuildPanel.h"
#include "LevelGui.h"
#include "PlayerController.h"
#include "SaveState.h"
#include "Unit.h"

#include <GetTextDialog.h>

Guild* guild;

void Guild::Clear()
{
	created = false;
}

void Guild::Create()
{
	GetTextDialogParams params(game_gui->guild->txEnterName, tmpName);
	DialogContext* ctx = DialogContext::current;
	params.event = [=](int id)
	{
		if(id == BUTTON_OK)
		{
			created = true;
			name = tmpName;
			reputation = 0;
			master = ctx->pc->unit;
			members.push_back(master);
		}
		game_gui->level_gui->SetDialogBox(nullptr);
		ctx->mode = DialogContext::NONE;
	};
	DialogBox* dialogBox = GetTextDialog::Show(params);
	game_gui->level_gui->SetDialogBox(dialogBox);
	ctx->mode = DialogContext::WAIT_INPUT;
}

void Guild::Save(GameWriter& f)
{
	f << created;
	if(!created)
		return;

	f << name;
	f << reputation;
	f << master;
	f << members;
}

void Guild::Load(GameReader& f)
{
	if(LOAD_VERSION < V_DEV)
	{
		created = false;
		return;
	}

	f >> created;
	if(!created)
		return;

	f >> name;
	f >> reputation;
	f >> master;
	f >> members;
}
