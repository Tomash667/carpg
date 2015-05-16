#include "Pch.h"
#include "Base.h"
#include "GameMenu.h"
#include "Language.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
TEX GameMenu::tLogo;

//=================================================================================================
GameMenu::GameMenu(const DialogInfo& info) : Dialog(info), prev_can_save(true), prev_can_load(true), prev_hardcore_mode(false)
{
	txSave = Str("saveGame");
	txSaveAndExit = Str("saveAndExit");
	txExitToMenuDialog = Str("exitToMenuDialog");

	cstring names[] = {
		"returnToGame",
		"saveGame",
		"loadGame",
		"options",
		"exitToMenu",
		"quit"
	};

	INT2 maxsize(0,0);

	for(int i=0; i<6; ++i)
	{
		bt[i].id = IdReturnToGame+i;
		bt[i].parent = this;
		bt[i].text = Str(names[i]);
		bt[i].size = GUI.default_font->CalculateSize(bt[i].text) + INT2(24,24);

		maxsize = Max(maxsize, bt[i].size);
	}

	size = INT2(256+16, 128+16+(maxsize.y+8)*6);

	INT2 offset((size.x-maxsize.x)/2,128+8);

	// ustaw przyciski
	for(int i=0; i<6; ++i)
	{
		bt[i].pos = offset;
		bt[i].size = maxsize;
		offset.y += maxsize.y+8;
	}

	visible = false;
}

//=================================================================================================
void GameMenu::Draw(ControlDrawData* /*cdd*/)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	GUI.DrawSprite(tLogo, global_pos+INT2(8,8));

	for(int i=0; i<6; ++i)
		bt[i].Draw();
}

//=================================================================================================
void GameMenu::Update(float dt)
{
	for(int i=0; i<6; ++i)
	{
		bt[i].mouse_focus = focus;
		bt[i].Update(dt);
	}

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		GUI.CloseDialog(this);
}

//=================================================================================================
void GameMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
			visible = true;
		pos = global_pos = (GUI.wnd_size-size)/2;
		for(int i=0; i<6; ++i)
			bt[i].global_pos = bt[i].pos + global_pos;
	}
	else if(e == GuiEvent_Close)
		visible = false;
	else if(e >= GuiEvent_Custom)
	{
		if(event)
			event(e);
	}
}

//=================================================================================================
void GameMenu::Set(bool can_save, bool can_load, bool hardcore_mode)
{
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
}
