#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class LoadScreen : public Control
{
public:
	LoadScreen() : progress(0) {}
	void Draw(ControlDrawData* cdd=nullptr);
	void LoadData();

	inline void SetProgress(float _progress, AnyString str)
	{
		assert(_progress >= 0.f && _progress <= 1.f);
		progress = _progress;
		text = str.s;
	}

private:
	TEX tBackground, tLoadbar, tLoadbarBg;
	float progress;
	string text;
};
