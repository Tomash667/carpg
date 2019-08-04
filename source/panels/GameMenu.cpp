#include "Pch.h"
#include "GameCore.h"
#include "GameMenu.h"
#include "Language.h"
#include "Input.h"
#include "ResourceManager.h"
#include "Game.h"
#include "GlobalGui.h"
#include "SaveLoadPanel.h"

//=================================================================================================
GameMenu::GameMenu(const DialogInfo& info) : DialogBox(info), prev_can_save(true), prev_can_load(true), prev_hardcore_mode(false)
{
	visible = false;
}

//=================================================================================================
void GameMenu::LoadLanguage()
{
	Language::Section& s = Language::GetSection("GameMenu");
	txSave = s.Get("saveGame");
	txSaveAndExit = s.Get("saveAndExit");
	txExitToMenuDialog = s.Get("exitToMenuDialog");

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
		bt[i].size = gui->default_font->CalculateSize(bt[i].text) + Int2(24, 24);

		maxsize = Int2::Max(maxsize, bt[i].size);
	}

	size = Int2(256 + 16, 128 + 16 + (maxsize.y + 8) * 6);

	Int2 offset((size.x - maxsize.x) / 2, 128 + 8);

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
	tLogo = ResourceManager::Get().Load<Texture>("logo_small.png");
}

//=================================================================================================
void GameMenu::Draw(ControlDrawData*)
{
	gui->DrawSpriteFull(tBackground, Color::Alpha(128));
	gui->DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

	gui->DrawSprite(tLogo, global_pos + Int2(8, 8));

	for(int i = 0; i < 6; ++i)
		bt[i].Draw();
}

//=================================================================================================
void GameMenu::Update(float dt)
{
	Game& game = Game::Get();
	bool can_save = game.CanSaveGame(),
		can_load = game.CanLoadGame(),
		hardcore_mode = game.hardcore_mode;

	if(can_save != prev_can_save)
	{
		bt[1].state = (can_save ? Button::NONE : Button::DISABLED);
		prev_can_save = can_save;
	}

	if(can_load != prev_can_load)
	{
		bt[2].state = (can_load ? Button::NONE : Button::DISABLED);
		prev_can_load = can_load;
	}

	if(hardcore_mode != prev_hardcore_mode)
	{
		bt[1].text = (hardcore_mode ? txSaveAndExit : txSave);
		prev_hardcore_mode = hardcore_mode;
	}

	for(int i = 0; i < 6; ++i)
	{
		bt[i].mouse_focus = focus;
		bt[i].Update(dt);
	}

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		gui->CloseDialog(this);
}

//=================================================================================================
void GameMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
			visible = true;
		pos = global_pos = (gui->wnd_size - size) / 2;
		for(int i = 0; i < 6; ++i)
			bt[i].global_pos = bt[i].pos + global_pos;
	}
	else if(e == GuiEvent_Close)
		visible = false;
	else if(e >= GuiEvent_Custom)
	{
		Game& game = Game::Get();
		switch((ButtonId)e)
		{
		case IdReturnToGame:
			CloseDialog();
			break;
		case IdSaveGame:
			game.gui->saveload->ShowSavePanel();
			break;
		case IdLoadGame:
			game.gui->saveload->ShowLoadPanel();
			break;
		case IdOptions:
			game.gui->ShowOptions();
			break;
		case IdExit:
			{
				DialogInfo info;
				info.event = [](int id)
				{
					if(id == BUTTON_YES)
						Game::Get().ExitToMenu();
				};
				info.name = "exit_to_menu";
				info.parent = nullptr;
				info.pause = true;
				info.text = txExitToMenuDialog;
				info.order = ORDER_TOP;
				info.type = DIALOG_YESNO;

				gui->ShowDialog(info);
			}
			break;
		case IdQuit:
			game.gui->ShowQuitDialog();
			break;
		}
	}
}
