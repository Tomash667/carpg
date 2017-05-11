// Overlay is top element control that manages focus in new gui
// In future there will be multiple overlays (one 2d for window, and multiple 3d for ingame computers)
#pragma once

#include "Container.h"

namespace gui
{
	class GuiDialog;
	class MenuBar;
	class MenuStrip;

	class Overlay : public Container
	{
	public:
		Overlay();
		~Overlay();

		bool NeedCursor() const override { return true; }
		void Draw(ControlDrawData* cdd = nullptr) override;
		void Update(float dt) override;

		void CloseDialog(GuiDialog* dialog);
		void ShowDialog(GuiDialog* dialog);
		void ShowMenu(MenuStrip* menu, const INT2& pos);
		void CloseMenu(MenuStrip* menu);
		void CheckFocus(Control* ctrl, bool pressed = false);
		void SetFocus(Control* ctrl);
		bool IsOpen(MenuStrip* menu);

	private:
		void CloseMenus();

		Control* focused;
		Control* mouse_focused;
		Control* clicked;
		MenuStrip* to_add;
		vector<MenuStrip*> menus, menus_to_close;
		vector<GuiDialog*> dialogs, dialogs_to_close;
		bool mouse_click;
	};
}
