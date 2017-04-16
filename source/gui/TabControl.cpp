#include "Pch.h"
#include "Base.h"
#include "TabControl.h"
#include "Panel.h"
#include "KeyStates.h"

using namespace gui;

static ObjectPool<TabControl::Tab> tab_pool;

TabControl::TabControl(bool own_panels) : selected(nullptr), hover(nullptr), own_panels(own_panels), tab_offset(0), tab_offset_max(0), arrow_hover(0)
{

}

TabControl::~TabControl()
{
	Clear();
}

void TabControl::Dock(Control* c)
{
	assert(c);
	INT2 area_pos = GetAreaPos() + global_pos;
	INT2 area_size = GetAreaSize();
	if(c->global_pos != area_pos)
	{
		c->global_pos = area_pos;
		if(c->IsInitialized())
			c->Event(GuiEvent_Moved);
	}
	if(c->size != area_size)
	{
		c->size = area_size;
		if(c->IsInitialized())
			c->Event(GuiEvent_Resize);
	}
}

void TabControl::Draw(ControlDrawData*)
{
	BOX2D body_rect = BOX2D::Create(global_pos, size);
	GUI.DrawArea(body_rect, layout->tabctrl.background);

	GUI.DrawArea(line, layout->tabctrl.line);

	BOX2D rectf;
	if(tab_offset > 0)
	{
		rectf.v1.x = (float)global_pos.x;
		rectf.v1.y = (float)global_pos.y + (height - layout->tabctrl.button_prev.size.y) / 2;
		rectf.v2 = rectf.v1 + layout->tabctrl.button_prev.size.ToVEC2();
		GUI.DrawArea(rectf, arrow_hover == -1 ? layout->tabctrl.button_prev_hover : layout->tabctrl.button_prev);
	}

	if(tab_offset_max != tabs.size())
	{
		rectf.v1.x = (float)global_pos.x + size.x - layout->tabctrl.button_next.size.x;
		rectf.v1.y = (float)global_pos.y + (height - layout->tabctrl.button_next.size.y) / 2;
		rectf.v2 = rectf.v1 + layout->tabctrl.button_next.size.ToVEC2();
		GUI.DrawArea(rectf, arrow_hover == 1 ? layout->tabctrl.button_next_hover : layout->tabctrl.button_next);
	}

	RECT rect;
	for(int i = tab_offset; i < tab_offset_max; ++i)
	{
		Tab* tab = tabs[i];
		AreaLayout* button;
		AreaLayout* close;
		DWORD color;
		switch(tab->mode)
		{
		default:
		case Tab::Up:
			button = &layout->tabctrl.button;
			color = layout->tabctrl.font_color;
			break;
		case Tab::Hover:
			button = &layout->tabctrl.button_hover;
			color = layout->tabctrl.font_color_hover;
			break;
		case Tab::Down:
			button = &layout->tabctrl.button_down;
			color = layout->tabctrl.font_color_down;
			break;
		}
		if(tab->close_hover)
			close = &layout->tabctrl.close_hover;
		else
			close = &layout->tabctrl.close;

		GUI.DrawArea(tab->rect, *button);
		rect = tab->rect.ToRect(layout->tabctrl.padding);
		GUI.DrawText(layout->tabctrl.font, tab->text, DT_LEFT | DT_VCENTER, color, rect);
		GUI.DrawArea(tab->close_rect, *close);

		if(tab->have_changes)
			GUI.DrawArea(RED, INT2(tab->rect.LeftTop()), INT2(2, tab->rect.SizeY()));
	}

	if(selected)
		selected->panel->Draw();
}

void TabControl::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		Update(true, true);
		for(Tab* tab : tabs)
			tab->panel->Initialize();
		break;
	case GuiEvent_Moved:
		Update(true, false);
		break;
	case GuiEvent_Resize:
		Update(false, true);
		break;
	}
}

void TabControl::Update(float dt)
{
	arrow_hover = 0;

	if(hover)
	{
		hover->mode = Tab::Up;
		hover->close_hover = false;
		hover = nullptr;
	}
	if(selected)
		selected->close_hover = false;

	if(mouse_focus && IsInside(GUI.cursor_pos))
	{
		BOX2D rectf;
		if(tab_offset > 0)
		{
			rectf.v1.x = (float)global_pos.x;
			rectf.v1.y = (float)global_pos.y + (height - layout->tabctrl.button_prev.size.y) / 2;
			rectf.v2 = rectf.v1 + layout->tabctrl.button_prev.size.ToVEC2();
			if(rectf.IsInside(GUI.cursor_pos))
			{
				arrow_hover = -1;
				if(Key.Pressed(VK_LBUTTON))
				{
					--tab_offset;
					CalculateTabOffsetMax();
				}
			}
		}

		if(tab_offset_max != tabs.size())
		{
			rectf.v1.x = (float)global_pos.x + size.x - layout->tabctrl.button_next.size.x;
			rectf.v1.y = (float)global_pos.y + (height - layout->tabctrl.button_next.size.y) / 2;
			rectf.v2 = rectf.v1 + layout->tabctrl.button_next.size.ToVEC2();
			if(rectf.IsInside(GUI.cursor_pos))
			{
				arrow_hover = 1;
				if(Key.Pressed(VK_LBUTTON))
				{
					++tab_offset;
					CalculateTabOffsetMax();
				}
			}
		}

		for(int i = tab_offset; i < tab_offset_max; ++i)
		{
			Tab* tab = tabs[i];
			if(tab->rect.IsInside(GUI.cursor_pos))
			{
				if(tab != selected)
				{
					hover = tab;
					hover->mode = Tab::Hover;
				}
				if(tab->close_rect.IsInside(GUI.cursor_pos))
				{
					tab->close_hover = true;
					if(Key.Pressed(VK_LBUTTON))
						tab->Close();
				}
				else if(Key.Pressed(VK_LBUTTON) && tab != selected)
				{
					if(SelectInternal(tab))
						hover = nullptr;
				}
				else if(Key.Pressed(VK_MBUTTON))
					tab->Close();
				break;
			}
		}
	}

	if(selected)
		UpdateControl(selected->panel, dt);
}

TabControl::Tab* TabControl::AddTab(cstring id, cstring text, Panel* panel, bool select)
{
	assert(id && text && panel);

	Tab* tab = new Tab;
	tab->parent = this;
	tab->id = id;
	tab->text = text;
	tab->panel = panel;
	tab->mode = Tab::Up;
	tab->close_hover = false;
	tab->size = layout->tabctrl.font->CalculateSize(text) + layout->tabctrl.padding * 2 + INT2(layout->tabctrl.close.size.x + layout->tabctrl.padding.x, 0);
	tab->have_changes = false;
	tabs.push_back(tab);
	CalculateTabOffsetMax();

	if(IsInitialized())
		panel->Initialize();
	if(panel->IsDocked())
		Dock(panel);

	bool selected = false;
	if(select || tabs.size() == 1u)
	{
		if(SelectInternal(tab))
		{
			ScrollTo(tab);
			selected = true;
		}
	}
	if(selected)
		ScrollTo(tab);

	return tab;
}

void TabControl::Clear()
{
	if(own_panels)
	{
		for(Tab* tab : tabs)
			delete tab->panel;
	}
	tab_pool.Free(tabs);
	selected = nullptr;
	hover = nullptr;
	total_width = 0;
	arrow_hover = 0;
}

TabControl::Tab* TabControl::Find(cstring id)
{
	for(Tab* tab : tabs)
	{
		if(tab->id == id)
			return tab;
	}
	return nullptr;
}

INT2 TabControl::GetAreaPos() const
{
	int height = layout->tabctrl.font->height + layout->tabctrl.padding_active.y;
	return INT2(0, height);
}

INT2 TabControl::GetAreaSize() const
{
	int height = layout->tabctrl.font->height + layout->tabctrl.padding_active.y;
	return INT2(size.x, size.y - height);
}

void TabControl::Close(Tab* tab)
{
	assert(tab);
	if(tab == hover)
		hover = nullptr;
	if(!handler(A_BEFORE_CLOSE, (int)tab))
		return;
	int index = GetIndex(tabs, tab);
	if(tab == selected)
	{
		// select next tab or previous if not exists
		Tab* new_selected;
		if(index == tabs.size() - 1)
		{
			if(index == 0)
				new_selected = nullptr;
			else
				new_selected = tabs[index - 1];
		}
		else
			new_selected = tabs[index + 1];
		SelectInternal(new_selected);
		if(new_selected == hover)
			hover = nullptr;
	}
	tab_pool.Free(tab);
	tabs.erase(tabs.begin() + index);
	if(tab_offset > (int)tabs.size())
		--tab_offset;
	CalculateTabOffsetMax();
}

void TabControl::Select(Tab* tab, bool scroll_to)
{
	if(tab == selected)
		return;
	if(SelectInternal(tab))
	{
		if(selected == hover)
			hover = nullptr;
		if(tab && scroll_to)
			ScrollTo(tab);
	}
}

void TabControl::Update(bool move, bool resize)
{
	if(move)
	{
		if(!IsDocked())
			global_pos = parent->global_pos + pos;
		CalculateRect();
	}

	if(resize)
	{
		height = layout->tabctrl.font->height + layout->tabctrl.padding_active.y;
		allowed_size = (int)(size.x - layout->tabctrl.button_prev.size.x - layout->tabctrl.button_next.region.SizeX());
	}

	int p = (layout->tabctrl.padding_active.y - layout->tabctrl.padding.y);
	line.v1.x = (float)global_pos.x;
	line.v2.x = (float)global_pos.x + size.x;
	line.v1.y = (float)(global_pos.y + height - p / 2);
	line.v2.y = line.v1.y + p / 2;
}

void TabControl::CalculateRect()
{
	total_width = (int)layout->tabctrl.button_prev.size.x;
	for(int i = tab_offset; i < tab_offset_max; ++i)
	{
		Tab* tab = tabs[i];
		CalculateRect(*tab, total_width);
		total_width += tab->size.x;
	}
}

void TabControl::CalculateRect(Tab& tab, int offset)
{
	INT2 pad = (tab.mode == Tab::Down ? layout->tabctrl.padding_active : layout->tabctrl.padding);
	int p = (layout->tabctrl.padding_active.y - layout->tabctrl.padding.y);

	tab.rect.v1 = VEC2((float)global_pos.x + offset, (float)global_pos.y);
	tab.rect.v2 = tab.rect.v1 + tab.size.ToVEC2();
	if(tab.mode == Tab::Down)
		tab.rect.v2.y += (layout->tabctrl.padding_active.y - layout->tabctrl.padding.y);
	else
	{
		tab.rect.v1.y += p / 2;
		tab.rect.v1.y += p;
	}

	tab.close_rect.v1.x = tab.rect.v2.x - pad.x - layout->tabctrl.close.size.x;
	tab.close_rect.v1.y = tab.rect.v1.y + (tab.rect.SizeY() - layout->tabctrl.close.size.y) / 2;
	tab.close_rect.v2 = tab.close_rect.v1 + layout->tabctrl.close.size.ToVEC2();
}

bool TabControl::SelectInternal(Tab* tab)
{
	if(tab == selected)
		return true;

	if(!handler(A_BEFORE_CHANGE, (int)selected))
		return false;

	if(selected)
	{
		selected->mode = Tab::Up;
		CalculateRect(*selected, (int)selected->rect.v1.x);
	}

	selected = tab;
	if(selected)
	{
		selected->mode = Tab::Down;
		if(!selected->panel->IsInitialized())
		{
			selected->panel->pos = INT2(1, height + 1);
			selected->panel->global_pos = global_pos + selected->panel->pos;
			selected->panel->size = size - INT2(2, height + 2);
			selected->panel->Initialize();
		}
		selected->panel->Show();
		CalculateRect(*selected, (int)selected->rect.v1.x);
	}

	handler(A_CHANGED, (int)selected);
	return true;
}

void TabControl::ScrollTo(Tab* tab)
{
	assert(tab);
	int index = GetIndex(tabs, tab);
	if(index >= tab_offset && index < tab_offset_max)
		return;
	tab_offset = index;
	CalculateTabOffsetMax();
}

void TabControl::CalculateTabOffsetMax()
{
	int width = 0;

	for(int i = tab_offset, count = tabs.size(); i < count; ++i)
	{
		width += tabs[i]->size.x;
		if(width > allowed_size)
		{
			tab_offset_max = i;
			CalculateRect();
			return;
		}
	}

	tab_offset_max = tabs.size();

	for(int i = tab_offset - 1; i >= 0; --i)
	{
		width += tabs[i]->size.x;
		if(width < allowed_size)
			--tab_offset;
	}

	CalculateRect();
}
