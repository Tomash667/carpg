#include "Pch.h"
#include "Core.h"
#include "ActionPanel.h"
#include "Language.h"

//=================================================================================================
ActionPanel::ActionPanel()
{
	txActions = Str("actions");
}

//=================================================================================================
void ActionPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	// capition
	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	GUI.DrawText(GUI.fBig, txActions, DT_TOP | DT_CENTER, BLACK, rect);

	// grid
	int count_w = (size.x - 48) / 63;
	int count_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;
	for(int y = 0; y < count_h; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			GUI.DrawSprite(tItemBar, Int2(shift_x + x * 63, shift_y + y * 63));
			uint index = x + y * count_w;
			if(index < actions.size())
			{

			}
		}
	}

	// tooltips
	//tooltip.Draw();
}

//=================================================================================================
void ActionPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);
}

//=================================================================================================
void ActionPanel::Update(float dt)
{
	GamePanel::Update(dt);


}
