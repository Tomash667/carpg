/*
Basic window, have two modes:
1. normal - have header with text, can be moved
2. borderless - no header, takes full screen space, can't be moved
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
		Window(bool fullscreen = false, bool borderless = false);
		~Window();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		Int2 GetAreaSize() const { return Int2(area.Size()); }
		Control* GetEventProxy() const { return event_proxy; }
		MenuBar* GetMenu() const { return menu; }
		const string& GetText() const { return text; }
		ToolStrip* GetToolStrip() const { return toolstrip; }
		bool IsBorderless() const { return borderless; }
		bool IsFullscreen() const { return fullscreen; }
		void SetAreaSize(const Int2& size);
		void SetEventProxy(Control* _event_proxy) { event_proxy = _event_proxy; }
		void SetMenu(MenuBar* menu);
		void SetText(Cstring s) { text = s.s; }
		void SetToolStrip(ToolStrip* toolstrip);

	private:
		void CalculateArea();

		MenuBar* menu;
		ToolStrip* toolstrip;
		Control* event_proxy;
		string text;
		Box2d body_rect, header_rect, area;
		bool fullscreen, borderless;
	};
}
