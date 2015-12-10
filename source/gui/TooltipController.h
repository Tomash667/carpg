// show box tooltip for elements under cursor
#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class TooltipController;

//-----------------------------------------------------------------------------
typedef fastdelegate::FastDelegate3<TooltipController*, int, int> TooltipGetText;

//-----------------------------------------------------------------------------
class TooltipController : public Control
{
public:
	void Init(TooltipGetText get_text);
	void Clear();
	void Update(float dt, int group, int id);
	void Draw(ControlDrawData* cdd = nullptr);

	string big_text, text, small_text;
	TEX img;
	bool anything;

private:
	enum class State
	{
		NOT_VISIBLE,
		COUNTING,
		VISIBLE
	};

	void FormatBox();

	State state;
	int group, id;
	TooltipGetText get_text;
	float timer, alpha;
	RECT r_big_text, r_text, r_small_text;
};
