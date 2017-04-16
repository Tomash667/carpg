// pasek przewijania
#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Scrollbar : public Control
{
public:
	explicit Scrollbar(bool hscrollbar = false, bool is_new = false);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	void LostFocus();
	// porusza scrollbar myszk¹, zwraca czy cokolwiek siê zmieni³o
	bool ApplyMouseWheel();
	void SetValue(float p) { offset = float(total - part)*p; }
	float GetValue() const { return offset / float(total - part); }
	void UpdateTotal(int new_total);
	void UpdateOffset(float change);

	static TEX tex, tex2;
	int total, part, change;
	float offset;
	INT2 click_pt;
	bool clicked, hscrollbar, manual_change;
};
