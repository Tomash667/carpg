#pragma once

#include "Container.h"

namespace gui
{
	class Panel : public Container
	{
	public:
		Panel() : Container(true), use_custom_color(false) {}

		void Draw(ControlDrawData* cdd = nullptr) override;

		DWORD custom_color;
		bool use_custom_color;
	};
}
