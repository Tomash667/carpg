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
			TextureAndColor,
			Item
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

		AreaLayout() : mode(None) {}
		AreaLayout(DWORD color) : mode(Color), color(color) {}
		AreaLayout(DWORD color, DWORD border_color, int width = 1) : mode(BorderColor), color(color), border_color(border_color), width(width) {}
		AreaLayout(TEX tex) : mode(Texture), tex(tex), color(WHITE), region(0, 0, 1, 1), pad(0)
		{
			SetFromArea(nullptr);
		}
		AreaLayout(TEX tex, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE), background_color(background_color),
			region(0, 0, 1, 1), pad(0) {}
		AreaLayout(TEX tex, const BOX2D& region) : mode(Texture), tex(tex), color(WHITE), background_color(WHITE), region(region), pad(0) {}
		AreaLayout(TEX tex, const BOX2D& region, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE),
			background_color(background_color), region(region), pad(0) {}
		AreaLayout(TEX tex, const IBOX2D& area) : mode(Texture), tex(tex), color(WHITE), background_color(WHITE), pad(0)
		{
			SetFromArea(&area);
		}
		AreaLayout(TEX tex, const IBOX2D& area, DWORD background_color) : mode(TextureAndColor), tex(tex), color(WHITE),
			background_color(background_color), pad(0)
		{
			SetFromArea(&area);
		}
		AreaLayout(TEX tex, int corner, int size) : mode(Item), tex(tex), size(corner, size) {}

		BOX2D CalculateRegion(const INT2& pos, const INT2& region);

	private:
		void SetFromArea(const IBOX2D* area);
	};
}
