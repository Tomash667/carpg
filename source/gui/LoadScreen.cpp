#include "Pch.h"
#include "LoadScreen.h"

#include "GameGui.h"

#include <Engine.h>
#include <ResourceManager.h>

//=================================================================================================
void LoadScreen::LoadData()
{
	tLoadbarBg = resMgr->Load<Texture>("loadbar_bg.png");
	tLoadbar = resMgr->Load<Texture>("loadbar.png");
	tBackground = resMgr->Load<Texture>("load_bg.jpg");
}

//=================================================================================================
void LoadScreen::Draw()
{
	// background
	gui->DrawSpriteFull(tBackground);

	// loadbar background
	Int2 imgSize = tLoadbarBg->GetSize();
	Int2 pt((gui->wndSize.x - imgSize.x) / 2, gui->wndSize.y - imgSize.y - 16);
	gui->DrawSprite(tLoadbarBg, pt);

	// loadbar
	Rect r = { pt.x, pt.y, pt.x + 8 + int(progress * (503 - 8)), pt.y + imgSize.y };
	Rect rp = { 0, 0, 8 + int(progress * (503 - 8)), imgSize.y };
	gui->DrawSpriteRectPart(tLoadbar, r, rp);

	// text
	Rect r2 = { 32, 0, gui->wndSize.x - 32, gui->wndSize.y - imgSize.y - 32 };
	gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_BOTTOM, Color::White, r2);
}

//=================================================================================================
void LoadScreen::Setup(float minProgress, float maxProgress, int steps, cstring str)
{
	assert(maxProgress >= minProgress && InRange(minProgress, 0.f, 1.f) && InRange(maxProgress, 0.f, 1.f) && steps > 0);
	this->minProgress = minProgress;
	this->maxProgress = maxProgress;
	this->steps = steps;
	if(str)
		text = str;
	else
		text.clear();
	progress = minProgress;
	step = 0;
	engine->DoPseudotick();
}

//=================================================================================================
void LoadScreen::Tick(cstring str)
{
	assert(step != steps);
	++step;
	progress = minProgress + (maxProgress - minProgress) * step / steps;
	if(str)
		text = str;
	engine->DoPseudotick();
}
