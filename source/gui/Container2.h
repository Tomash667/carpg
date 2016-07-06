#pragma once

#include "Control2.h"

namespace gui
{
	class Container2 : public Control2
	{
		friend class IGUI;
	public:
		void Event2(GuiEvent2 e, void* data);

	private:
		vector<Container*> ctrls;
		Control2* focused;
	};
}
