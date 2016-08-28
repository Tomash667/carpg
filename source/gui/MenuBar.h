#pragma once

#include "Control2.h"

namespace gui
{
	class Menu;

	/*class Menu;
	class MenuItem;

	class MenuItemContainer
	{
	public:
		~MenuItemContainer()
		{
			DeleteElements(items);
		}

		MenuItem* AddItem(AnyString text, int action = -1);
		void AddItems(std::initializer_list<MenuItem*> const & items);

	protected:
		vector<MenuItem*> items;
	};*/

	

	class MenuBar : public Control2
	{
		struct Item
		{
			/*friend class MenuBar;
		public:
			inline void FillRect(RECT& rect, const INT2& offset) const
			{
				rect.left = global_pos.x + offset.x;
				rect.top = global_pos.y + offset.y;
				rect.right = rect.left + size.x;
				rect.bottom = rect.top + size.y;
			}

		private:*/
			Menu* menu;
			string text;
			ElementState state;
			INT2 global_pos, pos, size;
			int action;
		};

	public:
		MenuBar();

		

		// Add item to bar, action -1 is no action
		void AddAction(cstring text, int action);
		// Add menu item to bar
		void AddMenu(cstring text, Menu* menu);
		// Add simple constructed menu, use "---" for horizontal line
		//void AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & items);


		/*virtual void Draw(ControlDrawData* cdd = nullptr) override;
		virtual void Event(GuiEvent e) override;
		virtual void Update(float dt) override;*/

		inline DWORD GetBkgColor() const { return bkg_color; }
		inline DWORD GetClickColor() const { return click_color; }
		inline Font* GetFont() const { return font; }
		inline DWORD GetFontColor() const { return font_color; }
		inline int GetHeight() const { return height; }
		inline DWORD GetHoverColor() const { return hover_color; }

		delegate<void(int)> handler;

	private:
		void Recalculate(bool size_change, bool pos_change);

		vector<Item*> items;
		Item* selected;
		Font* font;
		DWORD bkg_color, font_color, hover_color, click_color;
		int height;
	};
}
