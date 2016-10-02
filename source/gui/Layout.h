#pragma once

struct Font;

namespace gui
{  
	struct AreaLayout
	{
		enum Mode
		{
			Color,
			BorderColor,
			Texture,
			TextureAndColor
		};

		Mode mode;
		DWORD color;
		union
		{
			struct
			{
				DWORD border_color;
				int width;
			};
			struct
			{
				TEX tex;
				BOX2D region;
				int pad; // border padding in pixels on texture
				DWORD background_color;
			};
		};

		inline AreaLayout() {}
		inline AreaLayout(DWORD color) : mode(Color), color(color) {}
		inline AreaLayout(DWORD color, DWORD border_color, int width=1) : mode(BorderColor), color(color), border_color(border_color), width(width) {}
		inline AreaLayout(TEX tex) : mode(Texture), tex(tex), color(WHITE), region(0, 0, 1, 1), pad(0) {}
		inline AreaLayout(TEX tex, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE), background_color(background_color),
			region(0, 0, 1, 1), pad(0) {}
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
			DWORD font_color_hover;
			DWORD font_color_down;
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
			DWORD font_color_hover;
		} menustrip;

		struct
		{
			AreaLayout background;
			AreaLayout button;
			AreaLayout button_hover;
			AreaLayout button_down;
			AreaLayout close;
			AreaLayout close_hover;
			Font* font;
			INT2 padding;
			INT2 close_size;
			DWORD font_color;
			DWORD font_color_hover;
			DWORD font_color_down;
		} tabctrl;

		void Default();
		void LoadData();
	};
}
