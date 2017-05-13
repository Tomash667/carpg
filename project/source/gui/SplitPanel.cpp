#include "Pch.h"
#include "Base.h"
#include "SplitPanel.h"
#include "Panel.h"

using namespace gui;

SplitPanel::SplitPanel() : min_size1(0), min_size2(0), panel1(nullptr), panel2(nullptr), allow_move(true), horizontal(true), splitter_size(3)
{
}

SplitPanel::~SplitPanel()
{
	delete panel1;
	delete panel2;
}

void SplitPanel::Draw(ControlDrawData*)
{
	if(use_custom_color)
		GUI.DrawArea(custom_color, global_pos, size);
	else
		GUI.DrawArea(BOX2D::Create(global_pos, size), layout->panel.background);

	GUI.DrawArea(BOX2D(split_global), horizontal ? layout->split_panel.horizontal : layout->split_panel.vertical);

	panel1->Draw();
	panel2->Draw();
}

void SplitPanel::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		assert(parent);
		if(!panel1)
			panel1 = new Panel;
		panel1->parent = this;
		if(!panel2)
			panel2 = new Panel;
		panel2->parent = this;
		// tmp
		panel1->custom_color = RED;
		panel1->use_custom_color = true;
		panel2->custom_color = BLUE;
		panel2->use_custom_color = true;
		custom_color = GREEN;
		use_custom_color = true;
		Update(e, true, true);
		panel1->Initialize();
		panel2->Initialize();
		break;
	case GuiEvent_Moved:
		Update(e, false, true);
		break;
	case GuiEvent_Resize:
		Update(e, true, false);
		break;
	case GuiEvent_Show:
	case GuiEvent_Hide:
		panel1->Event(e);
		panel2->Event(e);
		break;
	case GuiEvent_LostMouseFocus:
		break;
	}
}

void SplitPanel::Update(float dt)
{
}

void SplitPanel::Update(GuiEvent e, bool resize, bool move)
{
	if(resize)
	{
		const INT2& padding = layout->split_panel.padding;
		INT2 size_left = size;
		if(horizontal)
		{
			size_left.x -= splitter_size;
			panel1->size = INT2(size_left.x / 2 - padding.x * 2, size_left.y - padding.y * 2);
			panel1->pos = padding;
			split = IBOX2D::Create(INT2(panel1->size.x + padding.x * 2, 0), INT2(splitter_size, size.y));
			panel2->size = INT2(size_left.x - panel1->size.x - padding.x * 2, size_left.y - padding.y * 2);
			panel2->pos = INT2(split.p1.x + padding.x, padding.y);
		}
		else
		{
			size_left.y -= splitter_size;
			panel1->size = INT2(size_left.x - padding.x * 2, size_left.y / 2 - padding.y * 2);
			panel1->pos = padding;
			split = IBOX2D::Create(INT2(0, panel1->size.y + padding.y), INT2(size.x, splitter_size));
			panel2->size = INT2(size_left.x - padding.x * 2, size_left.y - panel1->size.y - padding.y * 2);
			panel2->pos = INT2(padding.x, split.p1.y + padding.y);
		}
	}

	if(move)
	{
		global_pos = pos + parent->global_pos;
		panel1->global_pos = panel1->pos + global_pos;
		panel2->global_pos = panel2->pos + global_pos;
		split_global += global_pos;
	}

	if(e != GuiEvent_Initialize)
	{
		panel1->Event(e);
		panel2->Event(e);
	}
}

void SplitPanel::SetPanel1(Panel* panel)
{
	assert(panel);
	assert(!panel1);
	panel1 = panel;
}

void SplitPanel::SetPanel2(Panel* panel)
{
	assert(panel);
	assert(!panel2);
	panel2 = panel;
}

void SplitPanel::SetSplitterSize(uint _splitter_size)
{
	assert(!initialized);
	splitter_size = _splitter_size;
}
