// pasek przewijania
#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Scrollbar : public Control
{
public:
	explicit Scrollbar(bool hscrollbar = false);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);

	void LostFocus();
	// porusza scrollbar myszk¹, zwraca czy cokolwiek siê zmieni³o
	bool ApplyMouseWheel();
	inline void SetValue(float p)
	{
		offset = float(total-part)*p;
	}
	inline float GetValue() const
	{
		return offset/float(total-part);
	}

	int total, part, change;
	float offset;
	bool clicked, hscrollbar, manual_change;
	INT2 click_pt;
	static TEX tex, tex2;
};
