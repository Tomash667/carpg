#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "GuiElement.h"

//-----------------------------------------------------------------------------
class MenuList : public Control
{
public:
	MenuList(bool is_new = false);
	~MenuList();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Init();
	void AddItem(GuiElement* e);
	void AddItem(cstring text) { AddItem(new DefaultGuiElement(text)); }
	void AddItems(vector<GuiElement*>& items, bool items_owner = true);
	int GetIndex() { return selected; }
	GuiElement* GetItem() { return selected == -1 ? nullptr : items[selected]; }
	void PrepareItem(cstring text);
	void Select(delegate<bool(GuiElement*)> pred);
	void Select(int index) { selected = index; }

	DialogEvent event_handler;
	vector<GuiElement*> items;

private:
	int w, selected;
	bool items_owner;
};
