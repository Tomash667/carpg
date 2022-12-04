#include "Pch.h"
#include "GameMenu.h"

#include "Game.h"
#include "GameGui.h"
#include "Language.h"
#include "SaveLoadPanel.h"

#include <Input.h>
#include <ResourceManager.h>

//=================================================================================================
GameMenu::GameMenu(const DialogInfo& info) : DialogBox(info), prevCanSave(true), prevCanLoad(true), prevHardcoreMode(false)
{
	visible = false;
}

//=================================================================================================
void GameMenu::LoadLanguage()
{
	Language::Section s = Language::GetSection("GameMenu");
	txSave = s.Get("saveGame");
	txSaveAndExit = s.Get("saveAndExit");
	txExitToMenuDialog = s.Get("exitToMenuDialog");
	txExitToMenuDialogHardcore = s.Get("exitToMenuDialogHardcore");

	cstring names[] = {
		"returnToGame",
		"saveGame",
		"loadGame",
		"options",
		"exitToMenu",
		"quit"
	};

	Int2 maxsize(0, 0);

	for(int i = 0; i < 6; ++i)
	{
		bt[i].id = IdReturnToGame + i;
		bt[i].parent = this;
		bt[i].text = s.Get(names[i]);
		bt[i].size = GameGui::font->CalculateSize(bt[i].text) + Int2(24, 24);

		maxsize = Int2::Max(maxsize, bt[i].size);
	}

	size = Int2(272, 104 + (maxsize.y + 8) * 6);

	Int2 offset((size.x - maxsize.x) / 2, 92);

	// ustaw przyciski
	for(int i = 0; i < 6; ++i)
	{
		bt[i].pos = offset;
		bt[i].size = maxsize;
		offset.y += maxsize.y + 8;
	}
}

//=================================================================================================
void GameMenu::LoadData()
{
	tLogo = resMgr->Load<Texture>("logo_small.png");
}

//=================================================================================================
void GameMenu::Draw()
{
	DrawPanel();

	gui->DrawSprite(tLogo, globalPos + Int2(8, -24));

	for(int i = 0; i < 6; ++i)
		bt[i].Draw();
}

//=================================================================================================
void GameMenu::Update(float dt)
{
	CheckButtons();

	for(int i = 0; i < 6; ++i)
	{
		bt[i].mouseFocus = focus;
		bt[i].Update(dt);
	}

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		gui->CloseDialog(this);
}

//=================================================================================================
void GameMenu::CheckButtons()
{
	bool canSave = game->CanSaveGame(),
		canLoad = game->CanLoadGame(),
		hardcoreMode = game->hardcoreMode;

	if(canSave != prevCanSave)
	{
		bt[1].state = (canSave ? Button::NONE : Button::DISABLED);
		prevCanSave = canSave;
	}

	if(canLoad != prevCanLoad)
	{
		bt[2].state = (canLoad ? Button::NONE : Button::DISABLED);
		prevCanLoad = canLoad;
	}

	if(hardcoreMode != prevHardcoreMode)
	{
		bt[1].text = (hardcoreMode ? txSaveAndExit : txSave);
		prevHardcoreMode = hardcoreMode;
	}
}

//=================================================================================================
void GameMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			CheckButtons();
			visible = true;
		}
		pos = globalPos = (gui->wndSize - size) / 2;
		for(int i = 0; i < 6; ++i)
			bt[i].globalPos = bt[i].pos + globalPos;
	}
	else if(e == GuiEvent_Close)
		visible = false;
	else if(e >= GuiEvent_Custom)
	{
		switch((ButtonId)e)
		{
		case IdReturnToGame:
			CloseDialog();
			break;
		case IdSaveGame:
			gameGui->saveload->ShowSavePanel();
			break;
		case IdLoadGame:
			gameGui->saveload->ShowLoadPanel();
			break;
		case IdOptions:
			gameGui->ShowOptions();
			break;
		case IdExit:
			{
				DialogInfo info;
				info.event = [](int id)
				{
					if(id == BUTTON_YES)
						game->ExitToMenu();
				};
				info.name = "exit_to_menu";
				info.parent = nullptr;
				info.pause = true;
				info.text = game->hardcoreMode ? txExitToMenuDialogHardcore : txExitToMenuDialog;
				info.order = DialogOrder::Top;
				info.type = DIALOG_YESNO;

				gui->ShowDialog(info);
			}
			break;
		case IdQuit:
			gameGui->ShowQuitDialog();
			break;
		}
	}
}
