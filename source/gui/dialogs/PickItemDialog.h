#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "TooltipController.h"
#include "FlowContainer.h"

//-----------------------------------------------------------------------------
struct PickItemDialogParams
{
	PickItemDialogParams() : event(nullptr), get_tooltip(nullptr), parent(nullptr), multiple(0), size_min(300, 200), size_max(300, 512)
	{
	}

	vector<FlowItem*> items;
	int multiple;
	DialogEvent event;
	TooltipGetText get_tooltip;
	Control* parent;
	string text;
	INT2 size_min, size_max; // size.x is always taken from size_min for now

	void AddItem(cstring item_text, int group, int id, bool disabled = false);
	void AddSeparator(cstring item_text);
};

//-----------------------------------------------------------------------------
class PickItemDialog : public Dialog
{
public:
	void GetSelected(int& group, int& id) const
	{
		if(!selected.empty())
		{
			group = selected[0]->group;
			id = selected[0]->id;
		}
		else
		{
			group = -1;
			id = -1;
		}
	}
	vector<FlowItem*>& GetSelected()
	{
		return selected;
	}

	static PickItemDialog* Show(PickItemDialogParams& params);

	static PickItemDialog* self;
	static CustomButton custom_x;

private:
	enum Id
	{
		Cancel = GuiEvent_Custom
	};

	explicit PickItemDialog(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Create(PickItemDialogParams& params);
	void OnSelect();

	FlowContainer flow;
	TooltipGetText get_tooltip;
	int multiple;
	Button btClose;
	TooltipController tooltip;
	vector<FlowItem*> selected;
};
