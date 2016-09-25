/*
Basic window, have two modes:
1. normal - have header with text, can be moved
2. fullscreen - no header, takes full screen space, can't be moved
*/

#pragma once

#include "Container.h"

namespace gui
{
	class MenuBar;
	class ToolStrip;

	class Window : public Container
	{
	public:
		Window(bool fullscreen = false);
		~Window();

		// override
		void Draw(ControlDrawData* cdd = nullptr) override;
		void Update(float dt) override;
		void Event(GuiEvent e) override;
		inline bool NeedCursor() const override { return true; }

		// before initialize
		void SetMenu(MenuBar* menu);
		inline void SetText(AnyString s) { text = s.s; }
		void SetToolStrip(ToolStrip* toolstrip);

		// getters
		inline bool GetFullscreen() const { return fullscreen; }
		inline MenuBar* GetMenu() const { return menu; }
		inline const string& GetText() const { return text; }
		inline ToolStrip* GetToolStrip() const { return toolstrip; }

	private:
		MenuBar* menu;
		ToolStrip* toolstrip;
		string text;
		BOX2D body_rect, header_rect;
		bool fullscreen;
	};
}
