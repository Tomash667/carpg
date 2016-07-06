#pragma once

#include "Control2.h"

namespace gui
{
	class MenuStrip;

	struct SimpleMenuCtor
	{
		cstring text;
		int action;
	};

	class MenuItem
	{
	public:
		string text;
		MenuStrip* menu;
		int action;
		bool hr;
	};

	class MenuStrip : public Control2
	{
	public:
		void AddAction(cstring text, int action);
		void AddLine();
		void AddMenu(cstring text, MenuStrip* menu);
		void AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & items);
		void AddItems(std::initializer_list<SimpleMenuCtor> const & items);

		void SetHandler(Action handler);

		Action GetHandler() const { return handler; }

	private:
		vector<MenuItem*> items;
		MenuStrip* parent_menu;
		//Menu* open_child_menu;
		Action handler;
	};
}
