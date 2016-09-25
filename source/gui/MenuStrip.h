#pragma once

#include "Control.h"

namespace gui
{
	class MenuBar;

	struct SimpleMenuCtor
	{
		cstring text;
		int id;
	};

	class MenuStrip : public Control
	{
	public:
		struct Item
		{
			string text;
			int index, action;
			bool hover;
		};

		typedef delegate<void(int)> Handler;
		typedef delegate<void()> OnCloseHandler;

		MenuStrip(vector<SimpleMenuCtor>& items, int min_width = 0);
		~MenuStrip();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Update(float dt) override;

		void ShowAt(const INT2& pos);
		inline void OnClose()
		{
			if(on_close_handler)
				on_close_handler();
		}

		inline void SetHandler(Handler _handler) { handler = _handler; }
		inline void SetOnCloseHandler(OnCloseHandler _on_close_handler) { on_close_handler = _on_close_handler; }
		inline void SetOwner(MenuBar* _parent_menu_bar, int _index)
		{
			parent_menu_bar = _parent_menu_bar;
			index = _index;
		}
		void SetSelectedIndex(int index);

	private:
		void UpdateMouse();
		void UpdateKeyboard();

		Handler handler;
		OnCloseHandler on_close_handler;
		vector<Item> items;
		Item* selected;
		MenuBar* parent_menu_bar;
		int index;
	};
}
