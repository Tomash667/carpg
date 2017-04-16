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
			bool close_hover, have_changes;

		public:
			void Close() { parent->Close(this); }
			bool GetHaveChanges() const { return have_changes; }
			const string& GetId() const { return id; }
			TabControl* GetTabControl() const { return parent; }
			const string& GetText() const { return text; }
			bool IsSelected() const { return mode == Mode::Down; }
			void Select() { parent->Select(this); }
			void SetHaveChanges(bool _have_changes) { have_changes = _have_changes; }
		};

		typedef delegate<bool(int, int)> Handler;

		enum Action
		{
			A_BEFORE_CHANGE,
			A_CHANGED,
			A_BEFORE_CLOSE
		};

		TabControl(bool own_panels = true);
		~TabControl();

		void Dock(Control* c) override;
		void Draw(ControlDrawData* cdd) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		Tab* AddTab(cstring id, cstring text, Panel* panel, bool select = true);
		void Clear();
		void Close(Tab* tab);
		Tab* Find(cstring id);
		INT2 GetAreaPos() const;
		INT2 GetAreaSize() const;
		Tab* GetCurrentTab() const { return selected; }
		Handler GetHandler() { return handler; }
		void Select(Tab* tab, bool scroll_to = true);
		void SetHandler(Handler _handler) { handler = _handler; }
		void ScrollTo(Tab* tab);

	private:
		void Update(bool move, bool resize);
		void CalculateRect();
		void CalculateRect(Tab& tab, int offset);
		bool SelectInternal(Tab* tab);
		void CalculateTabOffsetMax();

		vector<Tab*> tabs;
		Tab* selected;
		Tab* hover;
		BOX2D line;
		Handler handler;
		int height, total_width, tab_offset, tab_offset_max, allowed_size;
		int arrow_hover; // -1-prev, 0-none, 1-next
		bool own_panels;
	};
}
