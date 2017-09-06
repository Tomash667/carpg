#pragma once

#include "Control.h"
#include "Scrollbar.h"

namespace gui
{
	class CheckBoxGroup : public Control
	{
	public:
		CheckBoxGroup();
		~CheckBoxGroup();

		void Draw(ControlDrawData*) override;
		void Update(float dt) override;

		void Add(cstring name, int value);
		int GetValue();
		void SetValue(int value);

	private:
		struct Item
		{
			string name;
			int value;
			bool checked;
		};

		vector<Item*> items;
		Scrollbar scrollbar;
		int row_height;
	};
}
