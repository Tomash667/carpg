#pragma once

#include "Control.h"

namespace gui
{
	class DrawBox : public Control
	{
	public:
		DrawBox();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Update(float dt) override;

		TEX GetTexture() const { return tex; }

		bool IsTexture() const { return tex != nullptr; }

		void SetTexture(TEX tex);

	private:
		TEX tex;
		INT2 tex_size, click_point;
		float scale, default_scale;
		VEC2 move;
		bool clicked;
	};
}
