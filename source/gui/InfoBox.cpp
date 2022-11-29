#include "Pch.h"
#include "InfoBox.h"

#include "Game.h"
#include "GameGui.h"
#include "LoadScreen.h"

//=================================================================================================
InfoBox::InfoBox(const DialogInfo& info) : DialogBox(info)
{
	visible = false;
}

//=================================================================================================
void InfoBox::Draw()
{
	DrawPanel(!game_gui->load_screen->visible);

	// text
	Rect r = { globalPos.x, globalPos.y, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r);
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
		globalPos = pos = (gui->wndSize - size) / 2;
	}
	else if(e == GuiEvent_Close)
		visible = false;
}

//=================================================================================================
void InfoBox::Show(cstring text)
{
	size = GameGui::font->CalculateSize(text) + Int2(24, 24);
	this->text = text;

	if(!visible)
		gui->ShowDialog(this);
	else
		globalPos = pos = (gui->wndSize - size) / 2;
}
