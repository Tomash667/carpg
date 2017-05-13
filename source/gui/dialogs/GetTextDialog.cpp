#include "Pch.h"
#include "Base.h"
#include "GetTextDialog.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
GetTextDialog* GetTextDialog::self;

//=================================================================================================
GetTextDialog::GetTextDialog(const DialogInfo& info) : Dialog(info), singleline(true)
{

}

//=================================================================================================
void GetTextDialog::Draw(ControlDrawData* cdd/* =nullptr */)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	for(int i=0; i<2; ++i)
		bts[i].Draw();

	RECT r = {global_pos.x+16,global_pos.y+16,global_pos.x+size.x,global_pos.y+size.y};
	GUI.DrawText(GUI.default_font, text, DT_CENTER, BLACK, r);

	textBox.Draw();
}

//=================================================================================================
void GetTextDialog::Update(float dt)
{
	textBox.mouse_focus = focus;

	if(Key.Focus() && focus)
	{
		for(int i=0; i<2; ++i)
		{
			bts[i].mouse_focus = focus;
			bts[i].Update(dt);
			if(result != -1)
				goto got_result;
		}

		textBox.focus = true;
		textBox.Update(dt);
		
		if(textBox.GetText().empty())
			bts[1].state = Button::DISABLED;
		else if(bts[1].state == Button::DISABLED)
			bts[1].state = Button::NONE;

		if(result == -1)
		{
			if(Key.PressedRelease(VK_ESCAPE))
				result = BUTTON_CANCEL;
			else if(Key.Pressed(VK_RETURN))
			{
				if(!textBox.IsMultiline() || Key.Down(VK_SHIFT))
				{
					Key.SetState(VK_RETURN, IS_UP);
					result = BUTTON_OK;
				}
			}
		}

		if(result != -1)
		{
got_result:
			if(result == BUTTON_OK)
				*input = textBox.GetText();
			GUI.CloseDialog(this);
			if(event)
				event(result);
		}
	}
}

//=================================================================================================
void GetTextDialog::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		textBox.focus = true;
		textBox.Event(GuiEvent_GainFocus);
	}
	else if(e == GuiEvent_LostFocus || e == GuiEvent_Close)
	{
		textBox.focus = false;
		textBox.Event(GuiEvent_LostFocus);
	}
	else if(e == GuiEvent_WindowResize)
	{
		self->pos = self->global_pos = (GUI.wnd_size-self->size)/2;
		self->bts[0].global_pos = self->bts[0].pos + self->global_pos;
		self->bts[1].global_pos = self->bts[1].pos + self->global_pos;
		self->textBox.global_pos = self->textBox.pos + self->global_pos;
	}
	else if(e >= GuiEvent_Custom)
	{
		if(e == Result_Ok)
			result = BUTTON_OK;
		else if(e == Result_Cancel)
			result = BUTTON_CANCEL;
	}
}

//=================================================================================================
GetTextDialog* GetTextDialog::Show(const GetTextDialogParams& params)
{
	if(!self)
	{
		DialogInfo info;
		info.event = nullptr;
		info.name = "GetTextDialog";
		info.parent = nullptr;
		info.pause = false;
		info.order = ORDER_NORMAL;
		info.type = DIALOG_CUSTOM;

		self = new GetTextDialog(info);
		self->bts.resize(2);

		Button& bt1 = self->bts[0],
			& bt2 = self->bts[1];

		bt1.id = Result_Cancel;
		bt1.size = INT2(100,40);
		bt1.parent = self;

		bt2.id = Result_Ok;
		bt2.size = INT2(100,40);
		bt2.parent = self;

		self->textBox.pos = INT2(25,60);
	}

	self->Create(params);

	GUI.ShowDialog(self);

	return self;
}

//=================================================================================================
void GetTextDialog::Create(const GetTextDialogParams& params)
{
	Button& bt1 = bts[0],
		  & bt2 = bts[1];

	int lines = params.lines;
	if(!params.multiline || params.lines < 1)
		lines = 1;

	size = INT2(params.width, 180+lines*20);
	textBox.size = INT2(params.width-50, 15+lines*20);
	textBox.SetMultiline(params.multiline);
	textBox.limit = params.limit;
	textBox.SetText(params.input->c_str());

	// ustaw przyciski
	bt1.pos = INT2(size.x-100-16, size.y-40-16);
	bt2.pos = INT2(16, size.y-40-16);
	if(params.custom_names)
	{
		bt1.text = (params.custom_names[0] ? params.custom_names[0] : GUI.txCancel);
		bt2.text = (params.custom_names[1] ? params.custom_names[1] : GUI.txOk);
	}
	else
	{
		bt1.text = GUI.txCancel;
		bt2.text = GUI.txOk;
	}

	// ustaw parametry
	result = -1;
	parent = params.parent;
	order = parent ? ((Dialog*)parent)->order : ORDER_NORMAL;
	event = params.event;
	text = params.text;
	input = params.input;

	// ustaw pozycjê
	pos = global_pos = (GUI.wnd_size-size)/2;
	bt1.global_pos = bt1.pos + global_pos;
	bt2.global_pos = bt2.pos + global_pos;
	textBox.global_pos = textBox.pos + global_pos;
}
