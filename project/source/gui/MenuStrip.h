#pragma once

#include "Control.h"

class GuiElement;

namespace gui
{
	class MenuBar;

	struct SimpleMenuCtor
	{
		cstring text;
		int id;
	};

	class MenuStrip : public Control, public OnCharHandler
	{
	public:
		struct Item
		{
			friend MenuStrip;

			int GetAction() const { return action; }
			int GetIndex() const { return index; }

			bool IsEnabled() const { return enabled; }

			void SetEnabled(bool _enabled) { enabled = _enabled; }

		private:
			string text;
			int index, action;
			bool hover, enabled;
		};

		typedef delegate<void(int)> Handler;
		typedef delegate<void()> OnCloseHandler;

		MenuStrip(vector<SimpleMenuCtor>& items, int min_width = 0);
		MenuStrip(vector<GuiElement*>& items, int min_width = 0);
		~MenuStrip();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void OnChar(char c) override;
		void Update(float dt) override;

		// internal! call only from Overlay -> Move to better place
		void ShowAt(const INT2& pos);
		void ShowMenu() { ShowMenu(GUI.cursor_pos); }
		void ShowMenu(const INT2& pos);
		void OnClose()
		{
			if(on_close_handler)
				on_close_handler();
		}
		Item* FindItem(int action);
		void SetHandler(Handler _handler) { handler = _handler; }
		void SetOnCloseHandler(OnCloseHandler _on_close_handler) { on_close_handler = _on_close_handler; }
		void SetOwner(MenuBar* _parent_menu_bar, int _index)
		{
			parent_menu_bar = _parent_menu_bar;
			menu_bar_index = _index;
		}
		void SetSelectedIndex(int index);
		bool IsOpen();

	private:
		void CalculateWidth(int min_width);
		void ChangeIndex(int dir);
		void UpdateMouse();
		void UpdateKeyboard();

		Handler handler;
		OnCloseHandler on_close_handler;
		vector<Item> items;
		Item* selected;
		MenuBar* parent_menu_bar;
		int menu_bar_index;
	};
}
