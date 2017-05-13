#pragma once

#include "Control.h"

namespace gui
{
	class DrawBox : public Control
	{
	public:
		DrawBox();

		void Draw(ControlDrawData* cdd = nullptr) override;

	};
}
