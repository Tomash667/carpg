// Overlay is top element control that manages focus in new gui
// In future there will be multiple overlays (one 2d for window, and multiple 3d for ingame computers)
#pragma once

#include "Container.h"

namespace gui
{
	class MenuBar;
	class MenuStrip;

	class Overlay : public Container
	{
	public:
		Overlay();
		~Overlay();

		bool NeedCursor() const override { return true; }
		void Update(float dt) override;

		void ShowMenu(MenuStrip* menu, const INT2& pos);
		void CloseMenu(MenuStrip* menu);

		void SetFocus(Control* ctrl, bool pressed = false);

	private:
		Control* focused;
		Control* mouse_focused;
		Control* clicked;
		MenuStrip* to_add;
		vector<MenuStrip*> menus;
		vector<MenuStrip*> to_close;
		bool mouse_click;
	};
}
