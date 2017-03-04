#pragma once

#include "Control.h"

namespace gui
{
	class Panel;

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
			Panel* panel;
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

		TabControl(bool own_panels = true);
		~TabControl();

		void Draw(ControlDrawData* cdd) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		Tab* AddTab(cstring id, cstring text, Panel* panel, bool select = true);
		void Clear();
		void Close(Tab* tab);
		Tab* Find(cstring id);
		void Select(Tab* tab, bool scroll_to = true);
		void ScrollTo(Tab* tab);

	private:
		void Update(bool move, bool resize);
		void CalculateRect();
		void CalculateRect(Tab& tab, int offset);
		void SelectInternal(Tab* tab);
		void CalculateTabOffsetMax();

		vector<Tab*> tabs;
		Tab* selected;
		Tab* hover;
		BOX2D line;
		int height, total_width, tab_offset, tab_offset_max, allowed_size;
		int arrow_hover; // -1-prev, 0-none, 1-next
		bool own_panels;
	};
}
