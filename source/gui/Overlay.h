#pragma once

#include "Control2.h"

namespace gui
{
	class MenuBar;

	class Overlay : public Control2
	{
	public:
		Overlay();

		MenuBar* menu;

	private:
		void Draw2() override;
		void Event2(GuiEvent2 e, void* data) override;
		void Update(float dt) override;
		bool NeedCursor() const override { return true; }

		vector<Control*> ctrls;
		Control* focused;
		Control* top_focus;
	};
}
