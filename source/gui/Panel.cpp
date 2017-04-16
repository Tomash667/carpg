#include "Pch.h"
#include "Base.h"
#include "Panel.h"

using namespace gui;

void Panel::Draw(ControlDrawData*)
{
	if(use_custom_color)
	{
		if(custom_color != 0)
			GUI.DrawArea(custom_color, global_pos, size);
	}
	else
		GUI.DrawArea(BOX2D::Create(global_pos, size), layout->panel.background);

	Container::Draw();
}
