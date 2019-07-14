#include "EnginePch.h"
#include "EngineCore.h"
#include "Layout.h"
#include "Gui.h"
#include "ResourceManager.h"

#include "Label.h"

using namespace gui;

Layout::~Layout()
{
	delete label;
}

void Layout::LoadDefault()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	Font* def_font = GUI.default_font;
	Font* font_big = GUI.fBig;
	TEX t;

	panel.background = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));

	window.background = AreaLayout(Color(0xAB, 0xAB, 0xAB), Color::Black);
	window.header = AreaLayout(Color(128, 128, 128), Color::Black);
	window.font = font_big;
	window.font_color = Color::Black;
	window.header_height = font_big->height + 4;
	window.padding = Int2(2, 2);

	menubar.background = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));
	menubar.button = AreaLayout(Color(245, 246, 247));
	menubar.button_hover = AreaLayout(Color(213, 231, 248), Color(122, 177, 232));
	menubar.button_down = AreaLayout(Color(184, 216, 249), Color(98, 163, 229));
	menubar.font = def_font;
	menubar.padding = Int2(4, 4);
	menubar.item_padding = Int2(10, 2);
	menubar.font_color = Color::Black;
	menubar.font_color_hover = Color::Black;
	menubar.font_color_down = Color::Black;

	menustrip.background = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));
	menustrip.button_hover = AreaLayout(Color(51, 153, 255));
	menustrip.font = def_font;
	menustrip.padding = Int2(2, 2);
	menustrip.item_padding = Int2(2, 2);
	menustrip.font_color = Color::Black;
	menustrip.font_color_hover = Color::Black;
	menustrip.font_color_disabled = Color(128, 128, 128);

	tabctrl.background = AreaLayout(Color(245, 246, 247));
	tabctrl.line = AreaLayout(Color(245, 246, 247), Color(0xA0, 0xA0, 0xA0));
	tabctrl.button = AreaLayout(Color(238, 238, 238), Color(172, 172, 172));
	tabctrl.button_hover = AreaLayout(Color(238, 238, 238), Color(142, 176, 200));
	tabctrl.button_down = AreaLayout(Color(233, 243, 252), Color(126, 180, 234));
	tabctrl.font = def_font;
	tabctrl.font_color = Color::Black;
	tabctrl.font_color_hover = Color::Black;
	tabctrl.font_color_down = Color::Black;
	tabctrl.padding = Int2(8, 4);
	tabctrl.padding_active = Int2(8, 8);
	t = tex_mgr.GetLoadedRaw("close_small.png");
	tabctrl.close = AreaLayout(t);
	tabctrl.close.size = Int2(12, 12);
	tabctrl.close_hover = AreaLayout(t, Color(51, 153, 255));
	tabctrl.close_hover.size = Int2(12, 12);
	t = tex_mgr.GetLoadedRaw("tabctrl_arrow.png");
	tabctrl.button_prev = AreaLayout(t, Rect(0, 0, 12, 16));
	tabctrl.button_prev_hover = AreaLayout(t, Rect(0, 0, 12, 16), Color(51, 153, 255));
	tabctrl.button_next = AreaLayout(t, Rect(16, 0, 28, 16));
	tabctrl.button_next_hover = AreaLayout(t, Rect(16, 0, 28, 16), Color(51, 153, 255));

	t = tex_mgr.GetLoadedRaw("box.png");
	tree_view.background = AreaLayout(t, 8, 32);
	t = tex_mgr.GetLoadedRaw("treeview.png");
	tree_view.button = AreaLayout(t, Rect(0, 0, 16, 16));
	tree_view.button_hover = AreaLayout(t, Rect(16, 0, 32, 16));
	tree_view.button_down = AreaLayout(t, Rect(32, 0, 48, 16));
	tree_view.button_down_hover = AreaLayout(t, Rect(48, 0, 64, 16));
	tree_view.font = def_font;
	tree_view.font_color = Color::Black;
	tree_view.selected = AreaLayout(Color(51, 153, 255));
	tree_view.level_offset = 16;
	t = tex_mgr.GetLoadedRaw("box2.png");
	tree_view.text_box_background = t;
	t = tex_mgr.GetLoadedRaw("drag_n_drop.png");
	tree_view.drag_n_drop = t;

	split_panel.background = AreaLayout(Color(0xAB, 0xAB, 0xAB), Color(0xA0, 0xA0, 0xA0));
	split_panel.padding = Int2(0, 0);
	t = tex_mgr.GetLoadedRaw("split_panel.png");
	split_panel.horizontal = AreaLayout(t, Rect(3, 2, 4, 5));
	split_panel.vertical = AreaLayout(t, Rect(11, 3, 14, 4));

	label = new LabelLayout;
	label->font = def_font;
	label->color = Color::Black;
	label->padding = Int2(0, 0);
	label->align = DTF_LEFT;

	t = tex_mgr.GetLoadedRaw("box.png");
	check_box_group.background = AreaLayout(t, 8, 32);
	t = tex_mgr.GetLoadedRaw("checkbox.png");
	check_box_group.box = AreaLayout(t, Rect(0, 0, 16, 16));
	check_box_group.checked = AreaLayout(t, Rect(16, 0, 32, 16));
	check_box_group.font = def_font;
	check_box_group.font_color = Color::Black;
}

Box2d AreaLayout::CalculateRegion(const Int2& pos, const Int2& region)
{
	Box2d box;
	box.v1.x = (float)pos.x + (region.x - size.x) / 2;
	box.v1.y = (float)pos.y + (region.y - size.y) / 2;
	box.v2.x = box.v1.x + size.x;
	box.v2.y = box.v1.y + size.y;
	return box;
}

void AreaLayout::SetFromArea(const Rect* area)
{
	Int2 tex_size = gui::GetSize(tex);
	if(area)
	{
		size = area->Size();
		region = Box2d(*area) / Vec2((float)tex_size.x, (float)tex_size.y);
	}
	else
		size = tex_size;
}
