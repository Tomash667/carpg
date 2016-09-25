#include "Pch.h"
#include "Base.h"
#include "Layout.h"
#include "Gui2.h"

using namespace gui;

void Layout::Default()
{
	Font* def_font = GUI.default_font;

	window.background = AreaLayout(COLOR_RGB(200, 200, 200));
	window.header = AreaLayout(COLOR_RGB(128, 128, 128));
	window.font = def_font;
	window.font_color = BLACK;
	window.header_height = def_font->height + 4;
	window.padding = INT2(2, 2);

	menubar.background = AreaLayout(COLOR_RGB(150, 150, 150));
	menubar.button = AreaLayout(COLOR_RGB(150, 150, 150));
	menubar.button_hover = AreaLayout(COLOR_RGB(200, 200, 200));
	menubar.button_down = AreaLayout(COLOR_RGB(255, 255, 255));
	menubar.font = def_font;
	menubar.height = def_font->height + 4;
	menubar.padding = INT2(2, 2);
	menubar.item_padding = INT2(10, 2);
	menubar.font_color = BLACK;
	menubar.font_hover_color = BLACK;
	menubar.font_down_color = BLACK;

	menustrip.background = AreaLayout(WHITE);
	menustrip.button = AreaLayout(WHITE);
	menustrip.button_hover = AreaLayout(COLOR_RGB(200, 200, 200));
	menustrip.font = def_font;
	menustrip.padding = INT2(2, 2);
	menustrip.item_padding = INT2(2, 2);
	menustrip.font_color = BLACK;
	menustrip.font_hover_color = BLACK;
}
