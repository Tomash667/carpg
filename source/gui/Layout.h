#pragma once

#include "AreaLayout.h"

namespace gui
{  
	struct LabelLayout;

	class Layout
	{
	public:
		struct Panel
		{
			AreaLayout background;
		} panel;

		struct Window
		{
			AreaLayout background;
			AreaLayout header;
			Font* font;
			DWORD font_color;
			INT2 padding;
			int header_height;
		} window;

		struct MenuBar
		{
			AreaLayout background;
			AreaLayout button;
			AreaLayout button_hover;
			AreaLayout button_down;
			Font* font;
			INT2 padding;
			INT2 item_padding;
			DWORD font_color;
			DWORD font_color_hover;
			DWORD font_color_down;
		} menubar;

		struct MenuStrip
		{
			AreaLayout background;
			AreaLayout button_hover;
			Font* font;
			INT2 padding;
			INT2 item_padding;
			DWORD font_color;
			DWORD font_color_hover;
			DWORD font_color_disabled;
		} menustrip;

		struct TabControl
		{
			AreaLayout background;
			AreaLayout line;
			AreaLayout button;
			AreaLayout button_hover;
			AreaLayout button_down;
			AreaLayout close;
			AreaLayout close_hover;
			AreaLayout button_prev;
			AreaLayout button_prev_hover;
			AreaLayout button_next;
			AreaLayout button_next_hover;
			Font* font;
			INT2 padding;
			INT2 padding_active;
			DWORD font_color;
			DWORD font_color_hover;
			DWORD font_color_down;
		} tabctrl;
		
		struct TreeView
		{
			AreaLayout background;
			AreaLayout selected;
			AreaLayout button;
			AreaLayout button_hover;
			AreaLayout button_down;
			AreaLayout button_down_hover;
			Font* font;
			DWORD font_color;
			int level_offset;
			TEX text_box_background;
			TEX drag_n_drop;
		} tree_view;

		struct SplitPanel
		{
			AreaLayout background;
			AreaLayout horizontal;
			AreaLayout vertical;
			INT2 padding;
		} split_panel;

		LabelLayout* label;

		struct CheckBoxGroup
		{
			AreaLayout background;
			AreaLayout box;
			AreaLayout checked;
			Font* font;
			DWORD font_color;
		} check_box_group;

		~Layout();
		void LoadDefault();
	};
}
