#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class LoadScreen : public Control
{
public:
	LoadScreen() : progress(0) {}

	void Draw(ControlDrawData* cdd = nullptr) override;

	void LoadData();

	void SetProgress(float _progress)
	{
		assert(_progress >= 0.f && _progress <= 1.f);
		progress = _progress;
	}

	void SetProgress(float _progress, const AnyString& str)
	{
		assert(_progress >= 0.f && _progress <= 1.f);
		progress = _progress;
		if(str.s != nullptr)
			text = str.s;
		else
			text.clear();
	}

	void SetProgressOptional(float _progress, cstring str)
	{
		assert(_progress >= 0.f && _progress <= 1.f);
		progress = _progress;
		if(str)
			text = str;
	}

	void Reset()
	{
		progress = 0.f;
		text.clear();
	}

private:
	TEX tBackground, tLoadbar, tLoadbarBg;
	float progress;
	string text;
};
