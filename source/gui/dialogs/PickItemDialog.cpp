#include "Pch.h"
#include "Base.h"
#include "PickItemDialog.h"

PickItemDialog* PickItemDialog::self;
static ObjectPool<PickItem> pick_items;

void PickItemDialogParams::AddItem(cstring text, int group, int id, bool disabled)
{
	PickItem* item = pick_items.Get();
	item->text = text;
	item->group = group;
	item->id = id;
	item->state = (disabled ? Button::DISABLED : Button::NONE);
	item->separator = false;
	items.push_back(item);
}

void PickItemDialogParams::AddSeparator(cstring text)
{
	PickItem* item = pick_items.Get();
	item->text = text;
	item->separator = true;
	items.push_back(item);
}

PickItemDialog::PickItemDialog(const DialogInfo&  info) : Dialog(info)
{

}

PickItemDialog::~PickItemDialog()
{
	Cleanup();
}

PickItemDialog* PickItemDialog::Show(const PickItemDialogParams& params)
{
	if(!self)
	{
		DialogInfo info;
		info.event = NULL;
		info.name = "PickItemDialog";
		info.parent = NULL;
		info.pause = false;
		info.order = ORDER_NORMAL;
		info.type = DIALOG_CUSTOM;

		self = new PickItemDialog(info);
	}
	else
		self->Cleanup();

	self->Create(params);

	GUI.ShowDialog(self);

	return self;
}

void PickItemDialog::Create(const PickItemDialogParams& params)
{
	parent = params.parent;
	order = parent ? ((Dialog*)parent)->order : ORDER_NORMAL;
	event = params.event;
	get_tooltip = params.get_tooltip;
	items = std::move(params.items);
	selected.clear();
	multiple = params.multiple;
}

void PickItemDialog::Draw(ControlDrawData*)
{

}

void PickItemDialog::Update(float dt)
{

}

void PickItemDialog::Event(GuiEvent e)
{

}

void PickItemDialog::Cleanup()
{
	pick_items.Free(items);
}
