#pragma once

struct Font;

namespace gui
{  
	struct AreaLayout
	{
		enum Mode
		{
			None,
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
		INT2 size;

		inline AreaLayout() : mode(None) {}
		inline AreaLayout(DWORD color) : mode(Color), color(color) {}
		inline AreaLayout(DWORD color, DWORD border_color, int width=1) : mode(BorderColor), color(color), border_color(border_color), width(width) {}
		inline AreaLayout(TEX tex) : mode(Texture), tex(tex), color(WHITE), region(0, 0, 1, 1), pad(0) {}
		inline AreaLayout(TEX tex, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE), background_color(background_color),
			region(0, 0, 1, 1), pad(0) {}
		inline AreaLayout(TEX tex, const BOX2D& region) : mode(Texture), tex(tex), color(WHITE), background_color(WHITE), region(region), pad(0) {}
		inline AreaLayout(TEX tex, const BOX2D& region, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE),
			background_color(background_color), region(region), pad(0) {}
		inline AreaLayout(TEX tex, const IBOX2D& area) : mode(Texture), tex(tex), color(WHITE), background_color(WHITE), pad(0)
		{
			SetFromArea(area);
		}
		inline AreaLayout(TEX tex, const IBOX2D& area, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE),
			background_color(background_color), pad(0)
		{
			SetFromArea(area);
		}

	private:
		inline void SetFromArea(const IBOX2D& area)
		{
			size = area.Size();
			D3DSURFACE_DESC desc;
			tex->GetLevelDesc(0, &desc);
			region = BOX2D(area) / VEC2((float)desc.Width, (float)desc.Height);
		}
	};

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
			AreaLayout button;
			AreaLayout button_hover;
			Font* font;
			INT2 padding;
			INT2 item_padding;
			DWORD font_color;
			DWORD font_color_hover;
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

		struct Label
		{
			Font* font;
			DWORD color;
			DWORD align;
			INT2 padding;
		} label;

		struct TreeView
		{
			AreaLayout background;
			AreaLayout selected;
			AreaLayout selected_focus;
			AreaLayout button;
			AreaLayout button_hover;
			AreaLayout button_down;
			AreaLayout button_down_hover;
			Font* font;
			DWORD font_color;
		} tree_view;

		struct SplitPanel
		{
			AreaLayout background;
			AreaLayout horizontal;
			AreaLayout vertical;
			INT2 padding;
		} split_panel;

		void LoadDefault();
	};
}
