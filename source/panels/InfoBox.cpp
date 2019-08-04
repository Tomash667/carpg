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
	DrawPanel(!game->gui->load_screen->visible);

	// text
	Rect r = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawText(GlobalGui::font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r);
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
	size = GlobalGui::font->CalculateSize(_text) + Int2(24, 24);
	text = _text;

	if(!visible)
		gui->ShowDialog(this);
	else
		global_pos = (gui->wnd_size - size) / 2;
}
