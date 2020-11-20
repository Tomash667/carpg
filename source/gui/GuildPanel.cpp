#include "Pch.h"
#include "GuildPanel.h"

GuildPanel::GuildPanel()
{
	visible = false;
}

void GuildPanel::Draw(ControlDrawData*)
{
	gui->DrawArea(Color::Green, Rect(200, 200, 500, 500));
}
