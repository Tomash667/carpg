#include "Pch.h"
#include "Base.h"
#include "Dialog2.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
Game* Dialog::game;
TEX Dialog::tBackground;

//=================================================================================================
Dialog::Dialog(const DialogInfo& info) : name(info.name), text(info.text), type(info.type), event(info.event), order(info.order), pause(info.pause), need_delete(false)
{
	parent = info.parent;
	focusable = true;
}

//=================================================================================================
void Dialog::Draw(ControlDrawData*)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));
	pos = (GUI.wnd_size - size)/2;
	GUI.DrawItem(tDialog, pos, size, COLOR_RGBA(255,255,255,222), 16);

	for(uint i=0; i<bts.size(); ++i)
	{
		bts[i].global_pos = bts[i].pos + pos;
		bts[i].Draw();
	}

	RECT r = { pos.x+12, pos.y+12, pos.x+size.x-12, pos.y+size.y-12 };
	GUI.DrawText(GUI.default_font, text, DT_CENTER, BLACK, r);
}

//=================================================================================================
void Dialog::Update(float dt)
{
	result = -1;

	for(vector<Button>::iterator it = bts.begin(), end = bts.end(); it != end; ++it)
	{
		it->mouse_focus = focus;
		it->Update(dt);
	}

	if(Key.Focus() && focus && result == -1)
	{
		if(bts[0].state != Button::DISABLED)
		{
			if(Key.PressedRelease(VK_ESCAPE))
				result = (type == DIALOG_OK ? BUTTON_OK : BUTTON_NO);
			else if(Key.PressedRelease(VK_RETURN) || Key.PressedRelease(VK_SPACE))
				result = (type == DIALOG_OK ? BUTTON_OK : BUTTON_YES);
		}
	}

	if(result != -1)
	{
		RemoveElement(GUI.created_dialogs, this);
		if(event)
			event(result);
		if(GUI.CloseDialog(this))
			delete this;
	}
}

//=================================================================================================
void Dialog::Event(GuiEvent e)
{
	if(e >= GuiEvent_Custom)
		result = e - GuiEvent_Custom;
}

//=================================================================================================
DialogWithCheckbox::DialogWithCheckbox(const DialogInfo& info) : Dialog(info)
{

}

//=================================================================================================
void DialogWithCheckbox::Draw(ControlDrawData*)
{
	Dialog::Draw();

	checkbox.global_pos = checkbox.pos + pos;
	checkbox.Draw();
}

//=================================================================================================
void DialogWithCheckbox::Update(float dt)
{
	if(result == -1)
	{
		checkbox.mouse_focus = focus;
		checkbox.Update(dt);
	}

	Dialog::Update(dt);
}

//=================================================================================================
void DialogWithCheckbox::Event(GuiEvent e)
{
	if(e >= GuiEvent_Custom)
	{
		if(e == GuiEvent_Custom + BUTTON_CHECKED)
			event(checkbox.checked ? BUTTON_CHECKED : BUTTON_UNCHECKED);
		else
			result = e-GuiEvent_Custom;
	}
}
