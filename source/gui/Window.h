/*
Basic window, have two modes:
1. normal - have header with text, can be moved
2. fullscreen - no header, takes full screen space, can't be moved
*/

#pragma once

#include "Panel.h"

namespace gui
{
	class MenuBar;
	class ToolStrip;

	class Window : public Panel
	{
	public:
		Window(bool fullscreen = false);
		~Window();

		// override
		void Draw(ControlDrawData* cdd = nullptr) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		// before initialize
		void SetMenu(MenuBar* menu);
		void SetText(const AnyString& s) { text = s.s; }
		void SetToolStrip(ToolStrip* toolstrip);

		// getters
		bool GetFullscreen() const { return fullscreen; }
		MenuBar* GetMenu() const { return menu; }
		const string& GetText() const { return text; }
		ToolStrip* GetToolStrip() const { return toolstrip; }

	private:
		void CalculateArea();

		MenuBar* menu;
		ToolStrip* toolstrip;
		string text;
		BOX2D body_rect, header_rect, area;
		bool fullscreen;
	};
}
