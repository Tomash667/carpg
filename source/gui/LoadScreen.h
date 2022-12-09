#pragma once

//-----------------------------------------------------------------------------
#include <Control.h>

//-----------------------------------------------------------------------------
// Load screen with progress bar & text
class LoadScreen : public Control
{
public:
	LoadScreen() : progress(0) {}
	void LoadData();
	void Draw() override;
	void Setup(float minProgress, float maxProgress, int steps, cstring str);
	void Tick(cstring str);

	void SetProgress(float progress)
	{
		assert(progress >= 0.f && progress <= 1.f);
		this->progress = progress;
	}

	void SetProgress(float progress, cstring str)
	{
		assert(progress >= 0.f && progress <= 1.f);
		this->progress = progress;
		if(str != nullptr)
			text = str;
		else
			text.clear();
	}

	void SetProgressOptional(float progress, cstring str)
	{
		assert(progress >= 0.f && progress <= 1.f);
		this->progress = progress;
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
	float progress, minProgress, maxProgress;
	string text;
	int step, steps;
};
