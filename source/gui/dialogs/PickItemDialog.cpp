#include "Pch.h"
#include "Base.h"
#include "PickItemDialog.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
PickItemDialog* PickItemDialog::self;
CustomButton PickItemDialog::custom_x;

//=================================================================================================
void PickItemDialogParams::AddItem(cstring text, int group, int id, bool disabled)
{
	FlowItem2* item = FlowItem2::Pool.Get();
	item->type = FlowItem2::Item;
	item->group = group;
	item->id = id;
	item->text = text;
	item->state = (!disabled ? Button::NONE : Button::DISABLED);
	items.push_back(item);
}

//=================================================================================================
void PickItemDialogParams::AddSeparator(cstring text)
{
	FlowItem2* item = FlowItem2::Pool.Get();
	item->type = FlowItem2::Section;
	item->text = text;
	item->state = Button::NONE;
	items.push_back(item);
}

//=================================================================================================
PickItemDialog::PickItemDialog(const DialogInfo&  info) : Dialog(info)
{
	flow.allow_select = true;
	flow.on_select = VoidF(this, &PickItemDialog::OnSelect);

	btClose.custom = &custom_x;
	btClose.id = Cancel;
	btClose.size = INT2(32, 32);
	btClose.parent = this;
}

//=================================================================================================
PickItemDialog* PickItemDialog::Show(PickItemDialogParams& params)
{
	if(!self)
	{
		DialogInfo info;
		info.event = nullptr;
		info.name = "PickItemDialog";
		info.parent = nullptr;
		info.pause = false;
		info.order = ORDER_NORMAL;
		info.type = DIALOG_CUSTOM;

		self = new PickItemDialog(info);
	}

	self->Create(params);

	GUI.ShowDialog(self);

	return self;
}

//=================================================================================================
void PickItemDialog::Create(PickItemDialogParams& params)
{
	result = -1;
	parent = params.parent;
	order = parent ? ((Dialog*)parent)->order : ORDER_NORMAL;
	event = params.event;
	get_tooltip = params.get_tooltip;
	if(params.get_tooltip)
		tooltip.Init(params.get_tooltip);
	multiple = params.multiple;
	size.x = params.size_min.x;
	text = std::move(params.text);
	flow.pos = INT2(16, 64);
	flow.size = INT2(size.x - 32, 10000);
	flow.SetItems(params.items);
	int flow_sizey = flow.GetHeight();
	flow_sizey += 64;
	if(flow_sizey < params.size_min.y)
		flow_sizey = params.size_min.y;
	else if(flow_sizey > params.size_max.y)
		flow_sizey = params.size_max.y;
	size.y = flow_sizey;
	pos = global_pos = (GUI.wnd_size - size) / 2;
	flow.UpdateSize(pos + INT2(16,64), INT2(size.x - 32, size.y - 80), true);
	btClose.pos = INT2(size.x - 48, 16);
	btClose.global_pos = global_pos + btClose.pos;
	selected.clear();
}

//=================================================================================================
void PickItemDialog::Draw(ControlDrawData*)
{
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255, 255, 255, 128));
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255, 255, 255, 222), 16);

	btClose.Draw();

	RECT r = { global_pos.x + 16, global_pos.y + 16, global_pos.x + size.x - 56, global_pos.y + size.y };
	GUI.DrawText(GUI.default_font, text, DT_CENTER, BLACK, r);

	flow.Draw();
	if(get_tooltip)
		tooltip.Draw();
}

//=================================================================================================
void PickItemDialog::Update(float dt)
{
	btClose.mouse_focus = focus;
	btClose.Update(dt);

	flow.mouse_focus = focus;
	flow.Update(dt);

	if(get_tooltip)
	{
		int group = -1, id = -1;
		flow.GetSelected(group, id);
		tooltip.Update(dt, group, id);
	}

	if(result == -1)
	{
		if(Key.PressedRelease(VK_ESCAPE))
		{
			result = BUTTON_CANCEL;
			GUI.CloseDialog(this);
			if(event)
				event(result);
		}
	}
}

//=================================================================================================
void PickItemDialog::Event(GuiEvent e)
{
	if(e == GuiEvent_WindowResize)
	{
		// recenter
		pos = global_pos = (GUI.wnd_size - size) / 2;
		flow.UpdatePos(pos);
		btClose.global_pos = global_pos + btClose.pos;
	}
	else if(e == Cancel)
	{
		result = BUTTON_CANCEL;
		GUI.CloseDialog(this);
		if(event)
			event(result);
	}
}

//=================================================================================================
void PickItemDialog::OnSelect()
{
	if(multiple == 0)
	{
		// single selection, return result
		result = BUTTON_OK;
		selected.push_back(flow.selected);
		GUI.CloseDialog(this);
		if(event)
			event(result);
	}
	else
	{
		// multiple selection
		FlowItem2* item = flow.selected;
		if(item->state == Button::DOWN)
		{
			// remove selection
			item->state = Button::NONE;
			RemoveElement(selected, item);
		}
		else
		{
			// add selection
			item->state = Button::DOWN;
			selected.push_back(item);
			if(selected.size() == (uint)multiple)
			{
				// selected requested count of items, return result
				result = BUTTON_OK;
				GUI.CloseDialog(this);
				if(event)
					event(result);
			}
		}
	}
}
