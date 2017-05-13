#include "Pch.h"
#include "Base.h"
#include "DrawBox.h"

using namespace gui;

DrawBox::DrawBox() : Control(true)
{

}

void DrawBox::Draw(ControlDrawData*)
{
	RECT r = Rect::CreateRECT(global_pos, size);

	GUI.DrawArea(RED, r);
}
