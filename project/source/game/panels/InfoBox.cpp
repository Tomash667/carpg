#include "Pch.h"
#include "Core.h"
#include "InfoBox.h"
#include "Game.h"
#include "LoadScreen.h"

//=================================================================================================
InfoBox::InfoBox(const DialogInfo& info) : GameDialogBox(info)
{
	visible = false;
}

//=================================================================================================
void InfoBox::Draw(ControlDrawData*)
{
	// t�o
	if(!game->load_screen->visible)
		GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255, 255, 255, 128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255, 255, 255, 222), 16);

	// tekst
	Rect r = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawText(GUI.default_font, text, DT_CENTER | DT_VCENTER, BLACK, r);
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
		global_pos = (GUI.wnd_size - size) / 2;
	}
	else if(e == GuiEvent_Close)
		visible = false;
}

//=================================================================================================
void InfoBox::Show(cstring _text)
{
	size = GUI.default_font->CalculateSize(_text) + Int2(24, 24);
	text = _text;

	if(!visible)
		GUI.ShowDialog(this);
}
