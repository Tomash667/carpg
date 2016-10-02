#pragma once

#include "Control.h"

namespace gui
{
	class Window;

	class TabControl : public Control
	{
	public:
		struct Tab
		{
			friend class TabControl;
		private:
			enum Mode
			{
				Up,
				Hover,
				Down
			};

			TabControl* parent;
			string text, id;
			Window* window;
			Mode mode;
			INT2 size;
			BOX2D rect, close_rect;
			bool close_hover;

		public:
			inline void Close() { parent->Close(this); }
			inline const string& GetId() const { return id; }
			inline TabControl* GetTabControl() const { return parent; }
			inline const string& GetText() const { return text; }
			inline bool IsSelected() const { return mode == Mode::Down; }
			inline void Select() { parent->Select(this); }
		};

		TabControl(bool own_wnds = true);
		~TabControl();

		void Draw(ControlDrawData* cdd) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		Tab* AddTab(cstring id, cstring text, Window* window, bool select = true);
		void Clear();
		void Close(Tab* tab);
		Tab* Find(cstring id);
		void Select(Tab* tab);

	private:
		void Update(bool move, bool resize);
		void CalculateRect();
		void CalculateRect(Tab& tab);

		vector<Tab*> tabs;
		Tab* selected;
		Tab* hover;
		int offset;
		bool own_wnds;
	};
}
