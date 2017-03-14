#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "GuiElement.h"

//-----------------------------------------------------------------------------
class MenuList : public Control
{
public:
	MenuList();
	~MenuList();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Init();
	void AddItem(GuiElement* e);
	void AddItem(cstring text) { AddItem(new DefaultGuiElement(text)); }
	void AddItems(vector<GuiElement*>& items, bool items_owner = true);
	void PrepareItem(cstring text);

	DialogEvent event_handler;
	vector<GuiElement*> items;

private:
	int w, selected;
	bool items_owner;
};
