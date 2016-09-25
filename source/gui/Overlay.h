// Overlay is top element control that manages mouse focus in new gui
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

		inline bool NeedCursor() const override { return true; }
		void Update(float dt) override;

		void ShowMenu(MenuStrip* menu, const INT2& pos);
		void CloseMenu(MenuStrip* menu);

		void SetFocus(Control* ctrl);

	private:
		bool mouse_click;
		Control* focused;
		Control* clicked;
		MenuStrip* to_add;
		vector<MenuStrip*> menus;
		vector<MenuStrip*> to_close;
	};
}
