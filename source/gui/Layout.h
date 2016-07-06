#pragma once

namespace gui
{
	// Gui element use texture or color (if use_tex, alpha is taken from color)
	struct AreaLayout
	{
		DWORD color;
		int left, right, top, bottom, pad;
		bool use_tex;
	};

	class Layout
	{
	public:
		struct
		{
			AreaLayout bar, selection;
			Font* font;
			int width;
		} menubar;

		struct
		{
			AreaLayout area;
			Font* font;
		};

		TEX tex;

		void Default();
	};
}
