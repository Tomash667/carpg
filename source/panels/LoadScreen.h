#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class LoadScreen : public Control
{
public:
	LoadScreen() : progress(0) {}
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Setup(float min_progress, float max_progress, int steps, cstring str);
	void Tick(cstring str);

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
	TexturePtr tBackground, tLoadbar, tLoadbarBg;
	float progress, min_progress, max_progress;
	string text;
	int step, steps;
};
