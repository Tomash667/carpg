#pragma once

#include "Panel.h"

namespace gui
{
	class Button;
	class ListBox;
	//class MenuList;
	class TextBox;

	class PickFileDialog : public Panel
	{
	public:

	private:
		ListBox* list_box;
		TextBox* tb_path, *tb_filename;
		//MenuList* menu_list;
		Button* b_select, *b_cancel;
	};
}
