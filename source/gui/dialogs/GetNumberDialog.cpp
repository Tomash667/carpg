#include "Pch.h"
#include "Base.h"
#include "GetNumberDialog.h"
#include "Language.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
GetNumberDialog* GetNumberDialog::self;

//=================================================================================================
GetNumberDialog::GetNumberDialog(const DialogInfo& info) : Dialog(info), scrollbar(true)
{
}

//=================================================================================================
void GetNumberDialog::Draw(ControlDrawData* cdd/* =nullptr */)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	for(int i=0; i<2; ++i)
		bts[i].Draw();

	RECT r = {global_pos.x+16,global_pos.y+16,global_pos.x+size.x,global_pos.y+size.y};
	GUI.DrawText(GUI.default_font, text, DT_CENTER, BLACK, r);

	textBox.Draw();
	scrollbar.Draw();

	RECT r2 = {global_pos.x+16,global_pos.y+124,global_pos.x+size.x-16,global_pos.y+size.y};
	GUI.DrawText(GUI.default_font, Format("%d", min_value), DT_LEFT, BLACK, r2);
	GUI.DrawText(GUI.default_font, Format("%d", max_value), DT_RIGHT, BLACK, r2);
}

//=================================================================================================
void GetNumberDialog::Update(float dt)
{
	textBox.mouse_focus = focus;

	if(Key.Focus() && focus)
	{
		for(int i=0; i<2; ++i)
		{
			bts[i].mouse_focus = focus;
			bts[i].Update(dt);
		}

		bool moving = scrollbar.clicked;
		float prev_offset = scrollbar.offset;
		scrollbar.ApplyMouseWheel();
		scrollbar.Update(dt);
		bool changed = false;
		if(scrollbar.change != 0 || GUI.mouse_wheel != 0.f)
		{
			int num = atoi(textBox.GetText().c_str());
			if(GUI.mouse_wheel != 0.f)
			{
				int change = 1;
				if(Key.Down(VK_SHIFT))
					change = max(1, (max_value-min_value)/20);
				if(GUI.mouse_wheel < 0.f)
					change = -change;
				num += change;
				if(num < min_value)
					num = min_value;
				else if(num > max_value)
					num = max_value;
			}
			else
				num += scrollbar.change;
			textBox.SetText(Format("%d", num));
			scrollbar.offset = float(num-min_value)/max_value*(scrollbar.total-scrollbar.part);
			changed = true;
		}
		else if(!equal(scrollbar.offset, prev_offset))
		{
			textBox.SetText(Format("%d", (int)lerp(float(min_value), float(max_value), scrollbar.offset / (scrollbar.total - scrollbar.part))));
			changed = true;
		}
		if(moving)
		{
			if(!scrollbar.clicked)
			{
				textBox.focus = true;
				textBox.Event(GuiEvent_GainFocus);
			}
			else
				changed = true;
		}
		else if(scrollbar.clicked)
		{
			textBox.focus = false;
			textBox.Event(GuiEvent_LostFocus);
		}
		textBox.Update(dt);
		if(!changed)
		{
			int num = atoi(textBox.GetText().c_str());
			if(!scrollbar.clicked)
				scrollbar.offset = float(num-min_value)/max_value*(scrollbar.total-scrollbar.part);
		}
		if(textBox.GetText().empty())
			bts[1].state = Button::DISABLED;
		else if(bts[1].state == Button::DISABLED)
			bts[1].state = Button::NONE;

		if(result == -1)
		{
			if(Key.Pressed(VK_ESCAPE))
				result = BUTTON_CANCEL;
			else if(Key.Pressed(VK_RETURN))
				result = BUTTON_OK;
		}

		if(result != -1)
		{
			if(result == BUTTON_OK)
				*value = atoi(textBox.GetText().c_str());
			GUI.CloseDialog(this);
			if(event)
				event(result);
		}
	}
}

//=================================================================================================
void GetNumberDialog::Event(GuiEvent e)
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
		scrollbar.LostFocus();
	}
	else if(e == GuiEvent_WindowResize)
	{
		self->pos = self->global_pos = (GUI.wnd_size-self->size)/2;
		self->bts[0].global_pos = self->bts[0].pos + self->global_pos;
		self->bts[1].global_pos = self->bts[1].pos + self->global_pos;
		self->textBox.global_pos = self->textBox.pos + self->global_pos;
		self->scrollbar.global_pos = self->scrollbar.pos + self->global_pos;
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
GetNumberDialog* GetNumberDialog::Show(Control* parent, DialogEvent event, cstring text, int min_value, int max_value, int* value)
{
	if(!self)
	{
		DialogInfo info;
		info.event = nullptr;
		info.name = "GetNumberDialog";
		info.parent = nullptr;
		info.pause = false;
		info.order = ORDER_NORMAL;
		info.type = DIALOG_CUSTOM;

		self = new GetNumberDialog(info);
		self->size = INT2(300, 200);
		self->bts.resize(2);

		Button& bt1 = self->bts[0],
			  & bt2 = self->bts[1];

		bt1.text = GUI.txCancel;
		bt1.id = Result_Cancel;
		bt1.size = INT2(100,40);
		bt1.pos = INT2(300-100-16,200-40-16);
		bt1.parent = self;

		bt2.text = GUI.txOk;
		bt2.id = Result_Ok;
		bt2.size = INT2(100,40);
		bt2.pos = INT2(16,200-40-16);
		bt2.parent = self;

		self->textBox.size = INT2(200,35);
		self->textBox.pos = INT2(50,60);
		self->textBox.SetNumeric(true);
		self->textBox.limit = 10;

		Scrollbar& scroll = self->scrollbar;
		scroll.pos = INT2(32,100);
		scroll.size = INT2(300-64,16);
		scroll.total = 100;
		scroll.part = 5;
	}

	self->result = -1;
	self->parent = parent;
	self->order = ((Dialog*)parent)->order;
	self->event = event;
	self->text = text;
	self->min_value = min_value;
	self->max_value = max_value;
	self->value = value;
	self->pos = self->global_pos = (GUI.wnd_size-self->size)/2;
	self->bts[0].global_pos = self->bts[0].pos + self->global_pos;
	self->bts[1].global_pos = self->bts[1].pos + self->global_pos;
	self->textBox.global_pos = self->textBox.pos + self->global_pos;
	self->textBox.num_min = min_value;
	self->textBox.num_max = max_value;
	self->scrollbar.global_pos = self->scrollbar.pos + self->global_pos;
	self->scrollbar.offset = 0;
	self->scrollbar.manual_change = true;
	self->textBox.SetText(nullptr);

	GUI.ShowDialog(self);

	return self;
}
