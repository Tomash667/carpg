#include "Pch.h"
#include "EngineCore.h"
#include "ComboBox.h"

//=================================================================================================
ComboBox::ComboBox() : menu_changed(false), selected(-1)
{
	menu.event_handler = delegate<void(int)>(this, &ComboBox::OnSelect);
}

//=================================================================================================
void ComboBox::Draw(ControlDrawData*)
{
	TextBox::Draw();
	if(menu.visible)
		menu.Draw();
}

//=================================================================================================
void ComboBox::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_GainFocus:
		menu_changed = true;
		break;
	case GuiEvent_LostFocus:
		menu.visible = false;
		break;
	}
	TextBox::Event(e);
}

//=================================================================================================
void ComboBox::Update(float dt)
{
	TextBox::Update(dt);
	if(menu_changed)
	{
		if(menu.items.empty())
			menu.visible = false;
		else
		{
			menu.visible = true;
			menu.Init();
			menu.global_pos = menu.pos = global_pos - Int2(0, menu.size.y);
		}
		menu_changed = false;
	}
	if(menu.visible)
	{
		menu.mouse_focus = focus;
		menu.focus = true;
		menu.Update(dt);
		if(!menu.focus)
			LostFocus();
	}
}

//=================================================================================================
void ComboBox::OnTextChanged()
{
	parent->Event((GuiEvent)Event_TextChanged);
}

//=================================================================================================
void ComboBox::Reset()
{
	TextBox::Reset();
	menu.visible = false;
	ClearItems();
	menu_changed = false;
}

//=================================================================================================
void ComboBox::ClearItems()
{
	if(!menu.items.empty())
	{
		menu_changed = true;
		if(destructor)
		{
			for(GuiElement* e : menu.items)
				destructor(e);
		}
		menu.items.clear();
	}
}

//=================================================================================================
void ComboBox::AddItem(GuiElement* e)
{
	menu_changed = true;
	menu.AddItem(e);
}

//=================================================================================================
void ComboBox::OnSelect(int index)
{
	selected = index;
	LostFocus();
	parent->Event((GuiEvent)Event_Selected);
}

//=================================================================================================
GuiElement* ComboBox::GetSelectedItem()
{
	return menu.items[selected];
}
