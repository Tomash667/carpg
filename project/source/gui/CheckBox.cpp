#include "Pch.h"
#include "Base.h"
#include "CheckBox.h"
#include "Button.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
TEX CheckBox::tTick;

//=================================================================================================
CheckBox::CheckBox(StringOrCstring text, bool checked) : text(text.c_str()), checked(checked), state(NONE), bt_size(32,32), radiobox(false)
{
}

//=================================================================================================
void CheckBox::Draw(ControlDrawData* cdd/* =nullptr */)
{
	GUI.DrawItem(Button::tex[state], global_pos, bt_size, WHITE, 12);

	if(checked)
		GUI.DrawSprite(tTick, global_pos);

	RECT r = {global_pos.x+bt_size.x+4, global_pos.y, global_pos.x+size.x, global_pos.y+size.y};
	GUI.DrawText(GUI.default_font, text, DT_VCENTER, BLACK, r);
}

//=================================================================================================
void CheckBox::Update(float dt)
{
	if(state == DISABLED)
		return;

	if(Key.Focus() && mouse_focus && PointInRect(GUI.cursor_pos, global_pos, bt_size))
	{
		GUI.cursor_mode = CURSOR_HAND;
		if(state == PRESSED)
		{
			if(Key.Up(VK_LBUTTON))
			{
				state = FLASH;
				if(radiobox)
				{
					if(!checked)
					{
						checked = true;
						parent->Event((GuiEvent)id);
					}
				}
				else
				{
					checked = !checked;
					parent->Event((GuiEvent)id);
				}
			}
		}
		else if(Key.Pressed(VK_LBUTTON))
			state = PRESSED;
		else
			state = FLASH;
	}
	else
		state = NONE;
}
