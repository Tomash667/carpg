#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
struct PickItem
{
	string text;
	int group, id;
	Button::State state;
	bool separator;
};

//-----------------------------------------------------------------------------
struct PickItemDialogParams
{
	PickItemDialogParams() : event(NULL), get_tooltip(NULL), parent(NULL), multiple(0)
	{

	}

	vector<PickItem*> items;
	int multiple;
	DialogEvent event;
	TooltipGetText get_tooltip;
	Control* parent;
	string text;

	void AddItem(cstring text, int group, int id, bool disabled = false);
	void AddSeparator(cstring text);
};

//-----------------------------------------------------------------------------
class PickItemDialog : public Dialog
{
public:
	~PickItemDialog();

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
	inline vector<PickItem*>& GetSelected()
	{
		return selected;
	}

	static PickItemDialog* Show(const PickItemDialogParams& params);

	static PickItemDialog* self;

private:
	PickItemDialog(const DialogInfo&  info);
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Event(GuiEvent e);
	void Create(const PickItemDialogParams& params);
	void Cleanup();

	vector<PickItem*> items;
	vector<PickItem*> selected;
	TooltipGetText get_tooltip;
	int multiple;
};
