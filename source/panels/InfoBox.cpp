#include "Pch.h"
#include "GameCore.h"
#include "InfoBox.h"
#include "Game.h"
#include "LoadScreen.h"
#include "GlobalGui.h"

//=================================================================================================
InfoBox::InfoBox(const DialogInfo& info) : GameDialogBox(info)
{
	visible = false;
}

//=================================================================================================
void InfoBox::Draw(ControlDrawData*)
{
	// t³o
	if(!game->gui->load_screen->visible)
		gui->DrawSpriteFull(tBackground, Color::Alpha(128));

	// panel
	gui->DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

	// tekst
	Rect r = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawText(gui->default_font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r);
}

//=================================================================================================
void InfoBox::Update(float dt)
{
	game->GenericInfoBoxUpdate(dt);
}

//=================================================================================================
void InfoBox::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
			visible = true;
		global_pos = (gui->wnd_size - size) / 2;
	}
	else if(e == GuiEvent_Close)
		visible = false;
}

//=================================================================================================
void InfoBox::Show(cstring _text)
{
	size = gui->default_font->CalculateSize(_text) + Int2(24, 24);
	text = _text;

	if(!visible)
		gui->ShowDialog(this);
	else
		global_pos = (gui->wnd_size - size) / 2;
}
