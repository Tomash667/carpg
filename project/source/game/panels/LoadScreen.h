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

	void Setup(float min_progress, float max_progress, int steps, cstring str)
	{
		assert(min_progress >= max_progress && InRange(min_progress, 0.f, 1.f) && InRange(max_progress, 0.f, 1.f) && steps > 0);
		this->min_progress = min_progress;
		this->max_progress = max_progress;
		this->step = steps;
		if(str)
			text = str;
		else
			text.clear();
		progress = min_progress;
		step = 0;
	}

	void SetProgress(float _progress)
	{
		assert(_progress >= 0.f && _progress <= 1.f);
		progress = _progress;
	}

	void SetProgress(float _progress, cstring str)
	{
		assert(_progress >= 0.f && _progress <= 1.f);
		progress = _progress;
		if(str != nullptr)
			text = str;
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
	float progress, min_progress, max_progress;
	string text;
	int step, steps;
};
