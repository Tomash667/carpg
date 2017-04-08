#include "Pch.h"
#include "Base.h"
#include "Layout.h"
#include "Gui2.h"
#include "ResourceManager.h"

#include "Label.h"

using namespace gui;

void Layout::LoadDefault()
{
	ResourceManager& res_mgr = ResourceManager::Get();
	Font* def_font = GUI.default_font;
	TEX t;

	panel.background = AreaLayout(COLOR_RGB(245, 246, 247), COLOR_RGB(0xA0, 0xA0, 0xA0));

	window.background = AreaLayout(COLOR_RGB(0xAB, 0xAB, 0xAB), COLOR_RGB(0xA0, 0xA0, 0xA0));
	window.header = AreaLayout(COLOR_RGB(128, 128, 128));
	window.font = def_font;
	window.font_color = BLACK;
	window.header_height = def_font->height + 4;
	window.padding = INT2(2, 2);

	menubar.background = AreaLayout(COLOR_RGB(245, 246, 247), COLOR_RGB(0xA0, 0xA0, 0xA0));
	menubar.button = AreaLayout(COLOR_RGB(245, 246, 247));
	menubar.button_hover = AreaLayout(COLOR_RGB(213, 231, 248), COLOR_RGB(122, 177, 232));
	menubar.button_down = AreaLayout(COLOR_RGB(184, 216, 249), COLOR_RGB(98, 163, 229));
	menubar.font = def_font;
	menubar.padding = INT2(4, 4);
	menubar.item_padding = INT2(10, 2);
	menubar.font_color = BLACK;
	menubar.font_color_hover = BLACK;
	menubar.font_color_down = BLACK;

	menustrip.background = AreaLayout(COLOR_RGB(245, 246, 247), COLOR_RGB(0xA0, 0xA0, 0xA0));
	menustrip.button_hover = AreaLayout(COLOR_RGB(51, 153, 255));
	menustrip.font = def_font;
	menustrip.padding = INT2(2, 2);
	menustrip.item_padding = INT2(2, 2);
	menustrip.font_color = BLACK;
	menustrip.font_color_hover = BLACK;
	menustrip.font_color_disabled = COLOR_RGB(128, 128, 128);

	tabctrl.background = AreaLayout(COLOR_RGB(245, 246, 247));
	tabctrl.line = AreaLayout(COLOR_RGB(245, 246, 247), COLOR_RGB(0xA0, 0xA0, 0xA0));
	tabctrl.button = AreaLayout(COLOR_RGB(238, 238, 238), COLOR_RGB(172, 172, 172));
	tabctrl.button_hover = AreaLayout(COLOR_RGB(238, 238, 238), COLOR_RGB(142, 176, 200));
	tabctrl.button_down = AreaLayout(COLOR_RGB(233, 243, 252), COLOR_RGB(126, 180, 234));
	tabctrl.font = def_font;
	tabctrl.font_color = BLACK;
	tabctrl.font_color_hover = BLACK;
	tabctrl.font_color_down = BLACK;
	tabctrl.padding = INT2(8, 4);
	tabctrl.padding_active = INT2(8, 8);
	res_mgr.GetLoadedTexture("close_small.png", t);
	tabctrl.close = AreaLayout(t);
	tabctrl.close.size = INT2(12, 12);
	tabctrl.close_hover = AreaLayout(t, COLOR_RGB(51, 153, 255));
	tabctrl.close_hover.size = INT2(12, 12);
	res_mgr.GetLoadedTexture("tabctrl_arrow.png", t);
	tabctrl.button_prev = AreaLayout(t, IBOX2D(0, 0, 12, 16));
	tabctrl.button_prev_hover = AreaLayout(t, IBOX2D(0, 0, 12, 16), COLOR_RGB(51, 153, 255));
	tabctrl.button_next = AreaLayout(t, IBOX2D(16, 0, 28, 16));
	tabctrl.button_next_hover = AreaLayout(t, IBOX2D(16, 0, 28, 16), COLOR_RGB(51, 153, 255));

	res_mgr.GetLoadedTexture("box.png", t);
	tree_view.background = AreaLayout(t, 8, 32);
	res_mgr.GetLoadedTexture("treeview.png", t);
	tree_view.button = AreaLayout(t, IBOX2D(0, 0, 16, 16));
	tree_view.button_hover = AreaLayout(t, IBOX2D(16, 0, 32, 16));
	tree_view.button_down = AreaLayout(t, IBOX2D(32, 0, 48, 16));
	tree_view.button_down_hover = AreaLayout(t, IBOX2D(48, 0, 64, 16));
	tree_view.font = def_font;
	tree_view.font_color = BLACK;
	tree_view.selected = AreaLayout(COLOR_RGB(51, 153, 255));
	tree_view.level_offset = 24;
	res_mgr.GetLoadedTexture("box2.png", t);
	tree_view.text_box_background = t;
	res_mgr.GetLoadedTexture("drag_n_drop.png", t);
	tree_view.drag_n_drop = t;

	split_panel.background = AreaLayout(COLOR_RGB(0xAB, 0xAB, 0xAB), COLOR_RGB(0xA0, 0xA0, 0xA0));
	split_panel.padding = INT2(0, 0);
	res_mgr.GetLoadedTexture("split_panel.png", t);
	split_panel.horizontal = AreaLayout(t, IBOX2D(3, 2, 4, 5));
	split_panel.vertical = AreaLayout(t, IBOX2D(11, 3, 14, 4));

	label = new LabelLayout;
	label->font = def_font;
	label->color = BLACK;
	label->padding = INT2(0, 0);
	label->align = DT_LEFT;

	res_mgr.GetLoadedTexture("box.png", t);
	check_box_group.background = AreaLayout(t, 8, 32);
	res_mgr.GetLoadedTexture("checkbox.png", t);
	check_box_group.box = AreaLayout(t, IBOX2D(0, 0, 16, 16));
	check_box_group.checked = AreaLayout(t, IBOX2D(16, 0, 32, 16));
	check_box_group.font = def_font;
	check_box_group.font_color = BLACK;
}

BOX2D AreaLayout::CalculateRegion(const INT2& pos, const INT2& region)
{
	BOX2D box;
	box.v1.x = (float)pos.x + (region.x - size.x) / 2;
	box.v1.y = (float)pos.y + (region.y - size.y) / 2;
	box.v2.x = box.v1.x + size.x;
	box.v2.y = box.v1.y + size.y;
	return box;
}

void AreaLayout::SetFromArea(const IBOX2D* area)
{
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);

	if(area)
	{
		size = area->Size();
		region = BOX2D(*area) / VEC2((float)desc.Width, (float)desc.Height);
	}
	else
		size = INT2(desc.Width, desc.Height);
}
