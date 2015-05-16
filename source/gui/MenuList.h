#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class MenuList : public Control
{
public:
	MenuList();
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Event(GuiEvent e);

	void Init();
	void AddItem(cstring text);
	void AddItems(const vector<string*>& texts);
	void PrepareItem(cstring text);

	DialogEvent event;
	vector<string> items;
	int w, selected;
};
