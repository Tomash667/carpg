#pragma once

#include "MenuStrip.h"

namespace gui
{
	class MenuBar : public Control
	{
		struct Item
		{
			enum Mode
			{
				Up,
				Hover,
				Down
			};

			string text;
			BOX2D rect;
			int index;
			Mode mode;
			vector<SimpleMenuCtor> items;
			MenuStrip* menu;

			Item() : menu(nullptr) {}
			~Item() { delete menu; }
		};

	public:
		MenuBar();
		~MenuBar();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Update(float dt) override;
		inline bool NeedCursor() const override { return true; }
		void Event(GuiEvent e) override;

		void AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & items);
		void ChangeMenu(int offset);
		
		inline void SetHandler(MenuStrip::Handler _handler) { handler = _handler; }

	private:
		void Update(bool move, bool resize);
		void OnCloseMenuStrip();
		void EnsureMenu(Item* item);

		vector<Item*> items;
		Item* selected;
		MenuStrip::Handler handler;
		BOX2D rect;
	};
}
