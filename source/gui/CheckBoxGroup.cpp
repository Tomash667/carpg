#include "Pch.h"
#include "Base.h"
#include "CheckBoxGroup.h"
#include "KeyStates.h"

using namespace gui;

CheckBoxGroup::CheckBoxGroup() : scrollbar(false, true)
{
	row_height = max(layout->check_box_group.font->height, layout->check_box_group.box.size.y) + 2;
}

CheckBoxGroup::~CheckBoxGroup()
{
	DeleteElements(items);
}

void CheckBoxGroup::Draw(ControlDrawData*)
{
	BOX2D rect = BOX2D::Create(global_pos, size);
	GUI.DrawArea(rect, layout->check_box_group.background);

	int box_height = layout->check_box_group.box.size.y;
	int box_x = global_pos.x + 2;
	int box_y = global_pos.y + (row_height - box_height) / 2;
	int text_x = box_x + layout->check_box_group.box.size.x + 2;
	int text_y = global_pos.y + (row_height - layout->check_box_group.font->height) / 2;

	BOX2D r;
	RECT re;
	int offset = 0;
	for(auto item : items)
	{
		r.v1 = VEC2((float)box_x, (float)box_y + offset);
		r.v2 = r.v1 + layout->check_box_group.box.size.ToVEC2();
		GUI.DrawArea(r, item->checked ? layout->check_box_group.checked : layout->check_box_group.box);

		re.left = text_x;
		re.top = text_y + offset;
		re.right = global_pos.x + size.x - 2;
		re.bottom = text_y + offset + 50;
		GUI.DrawText(layout->check_box_group.font, item->name, DT_LEFT | DT_SINGLELINE, layout->check_box_group.font_color, re);

		offset += row_height;
	}
}

void CheckBoxGroup::Update(float dt)
{
	if(!mouse_focus)
		return;

	const INT2& box_size = layout->check_box_group.box.size;
	int box_x = global_pos.x + 2;
	int box_y = global_pos.y + (row_height - box_size.y) / 2;

	int offset = 0;
	for(auto item : items)
	{
		if(PointInRect(GUI.cursor_pos, box_x, box_y + offset, box_x + box_size.x, box_y + offset + box_size.y) && Key.Pressed(VK_LBUTTON))
		{
			item->checked = !item->checked;
			TakeFocus(true);
			break;
		}

		offset += row_height;
	}
}

void CheckBoxGroup::Add(cstring name, int value)
{
	assert(name);

	auto item = new Item;
	item->name = name;
	item->value = value;
	item->checked = false;

	items.push_back(item);
}

int CheckBoxGroup::GetValue()
{
	int value = 0;

	for(auto item : items)
	{
		if(item->checked)
			value |= item->value;
	}

	return value;
}

void CheckBoxGroup::SetValue(int value)
{
	for(auto item : items)
		item->checked = IS_SET(value, item->value);
}
