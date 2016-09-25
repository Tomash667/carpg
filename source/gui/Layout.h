#pragma once

struct Font;

namespace gui
{  
	struct AreaLayout
	{
		DWORD color;
		TEX tex;
		BOX2D region;
		int pad; // border padding in pixels on texture

		inline AreaLayout() {}
		inline AreaLayout(DWORD color) : color(color), tex(nullptr) {}
	};

	class Layout
	{
	public:
		struct
		{
			AreaLayout background;
			AreaLayout header;
			Font* font;
			DWORD font_color;
			INT2 padding;
			int header_height;
		} window;

		struct
		{
			AreaLayout background;
			AreaLayout button;
			AreaLayout button_hover;
			AreaLayout button_down;
			Font* font;
			INT2 padding;
			INT2 item_padding;
			int height;
			DWORD font_color;
			DWORD font_hover_color;
			DWORD font_down_color;
		} menubar;

		struct
		{
			AreaLayout background;
			AreaLayout button;
			AreaLayout button_hover;
			Font* font;
			INT2 padding;
			INT2 item_padding;
			DWORD font_color;
			DWORD font_hover_color;
		} menustrip;

		void Default();
	};
}
