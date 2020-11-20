#include "Pch.h"
#include "Guild.h"

#include "DialogContext.h"
#include "GameGui.h"
#include "LevelGui.h"
#include "SaveState.h"

#include <GetTextDialog.h>

Guild* guild;

void Guild::Clear()
{
	created = false;
}

void Guild::Create()
{
	GetTextDialogParams params("Guild name:", tmpName);
	DialogContext* ctx = DialogContext::current;
	params.event = [=](int id)
	{
		if(id == BUTTON_OK)
		{
			created = true;
			name = tmpName;
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
}
