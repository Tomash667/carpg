#pragma once

struct Font;

namespace gui
{
	struct AreaLayout
	{
		enum Mode
		{
			None,
			ColorOnly,
			BorderColor,
			Texture,
			TextureAndColor,
			Item
		};

		Mode mode;
		Color color;
		union
		{
			struct
			{
				Color border_color;
				int width;
			};
			struct
			{
				TEX tex;
				Box2d region;
				int pad; // border padding in pixels on texture
				Color background_color;
			};
		};
		Int2 size;

		AreaLayout() : mode(None) {}
		AreaLayout(Color color) : mode(ColorOnly), color(color) {}
		AreaLayout(Color color, Color border_color, int width = 1) : mode(BorderColor), color(color), border_color(border_color), width(width) {}
		AreaLayout(TEX tex) : mode(Texture), tex(tex), color(Color::White), region(0, 0, 1, 1), pad(0)
		{
			SetFromArea(nullptr);
		}
		AreaLayout(TEX tex, Color background_color) : mode(TextureAndColor), tex(tex), color(Color::White), background_color(background_color),
			region(0, 0, 1, 1), pad(0) {}
		AreaLayout(TEX tex, const Box2d& region) : mode(Texture), tex(tex), color(Color::White), background_color(Color::White), region(region), pad(0) {}
		AreaLayout(TEX tex, const Box2d& region, Color background_color) : mode(TextureAndColor), tex(tex), color(Color::White),
			background_color(background_color), region(region), pad(0) {}
		AreaLayout(TEX tex, const Rect& area) : mode(Texture), tex(tex), color(Color::White), background_color(Color::White), pad(0)
		{
			SetFromArea(&area);
		}
		AreaLayout(TEX tex, const Rect& area, Color background_color) : mode(TextureAndColor), tex(tex), color(Color::White),
			background_color(background_color), pad(0)
		{
			SetFromArea(&area);
		}
		AreaLayout(TEX tex, int corner, int size) : mode(Item), tex(tex), size(corner, size) {}
		AreaLayout(const AreaLayout& l) : mode(l.mode), color(l.color), size(l.size)
		{
			memcpy(&tex, &l.tex, sizeof(TEX) + sizeof(Box2d) + sizeof(int) + sizeof(Color));
		}

		AreaLayout& operator = (const AreaLayout& l)
		{
			mode = l.mode;
			color = l.color;
			size = l.size;
			memcpy(&tex, &l.tex, sizeof(TEX) + sizeof(Box2d) + sizeof(int) + sizeof(Color));
			return *this;
		}

		Box2d CalculateRegion(const Int2& pos, const Int2& region);

	private:
		void SetFromArea(const Rect* area);
	};
}
