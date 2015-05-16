#include "Pch.h"
#include "Base.h"
#include "StaticText.h"

//=================================================================================================
void StaticText::Draw(ControlDrawData* cdd)
{
	if(size.x > 1 && size.y > 1)
	{
		RECT r = {
			global_pos.x,
			global_pos.y,
			global_pos.x+size.x,
			global_pos.y+size.y
		};
		if(cdd)
			GUI.DrawText(font, text.c_str(), 0, color, r, cdd->clipping, cdd->hitboxes, cdd->hitbox_counter);
		else
			GUI.DrawText(font, text.c_str(), 0, color, r);
	}
}
