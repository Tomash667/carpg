#pragma once

//-----------------------------------------------------------------------------
#include "TextBox.h"
#include "MenuList.h"

//-----------------------------------------------------------------------------
class ComboBox : public TextBox
{
public:
	enum Id
	{
		Event_TextChanged = GuiEvent_Custom,
		Event_Selected
	};

	ComboBox();
	void Draw(ControlDrawData* = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;
	void OnTextChanged() override;
	void ClearItems();
	void AddItem(GuiElement* e);
	void Reset();
	void OnSelect(int index);
	GuiElement* GetSelectedItem();

	delegate<void(GuiElement*)> destructor;

private:
	MenuList menu;
	int selected;
	bool menu_changed;
};
