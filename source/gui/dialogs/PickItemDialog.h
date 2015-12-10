#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "TooltipController.h"
#include "FlowContainer2.h"

//-----------------------------------------------------------------------------
struct PickItemDialogParams
{
	PickItemDialogParams() : event(nullptr), get_tooltip(nullptr), parent(nullptr), multiple(0), size_min(300, 200), size_max(300, 512)
	{

	}

	vector<FlowItem2*> items;
	int multiple;
	DialogEvent event;
	TooltipGetText get_tooltip;
	Control* parent;
	string text;
	INT2 size_min, size_max; // size.x is always taken from size_min for now

	void AddItem(cstring text, int group, int id, bool disabled = false);
	void AddSeparator(cstring text);
};

//-----------------------------------------------------------------------------
class PickItemDialog : public Dialog
{
public:
	inline void GetSelected(int& group, int& id) const
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
	inline vector<FlowItem2*>& GetSelected()
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

	explicit PickItemDialog(const DialogInfo&  info);
	void Draw(ControlDrawData* cdd = nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	void Create(PickItemDialogParams& params);
	void OnSelect();

	FlowContainer2 flow;
	TooltipGetText get_tooltip;
	int multiple;
	Button btClose;
	TooltipController tooltip;
	vector<FlowItem2*> selected;
};
