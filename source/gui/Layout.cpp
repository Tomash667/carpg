#include "Pch.h"
#include "Base.h"
#include "Layout.h"
#include "Gui2.h"
#include "ResourceManager.h"

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
	menubar.font_color_hover = BLACK;
	menubar.font_color_down = BLACK;

	menustrip.background = AreaLayout(WHITE);
	menustrip.button = AreaLayout(WHITE);
	menustrip.button_hover = AreaLayout(COLOR_RGB(200, 200, 200));
	menustrip.font = def_font;
	menustrip.padding = INT2(2, 2);
	menustrip.item_padding = INT2(2, 2);
	menustrip.font_color = BLACK;
	menustrip.font_color_hover = BLACK;

	tabctrl.background = AreaLayout(COLOR_RGB(200, 200, 200));
	tabctrl.button = AreaLayout(COLOR_RGB(150, 150, 150), BLACK);
	tabctrl.button_hover = AreaLayout(COLOR_RGB(200, 200, 200), BLACK);
	tabctrl.button_down = AreaLayout(COLOR_RGB(255, 255, 255), BLACK);
	tabctrl.close = AreaLayout(BLACK);
	tabctrl.close_hover = AreaLayout(COLOR_RGB(150, 150, 150));
	tabctrl.font = def_font;
	tabctrl.font_color = BLACK;
	tabctrl.font_color_hover = BLACK;
	tabctrl.font_color_down = BLACK;
	tabctrl.padding = INT2(2, 2);
	tabctrl.close_size = INT2(12, 12);
}

void Layout::LoadData()
{
	ResourceManager& res_mgr = ResourceManager::Get();

	TEX tClose;
	res_mgr.GetLoadedTexture("close_small.png", tClose);
	tabctrl.close = AreaLayout(tClose);
	tabctrl.close_hover = AreaLayout(tClose, COLOR_RGB(150, 150, 150));
}
