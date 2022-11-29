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
	Int2 img_size = tLoadbarBg->GetSize();
	Int2 pt((gui->wndSize.x - img_size.x) / 2, gui->wndSize.y - img_size.y - 16);
	gui->DrawSprite(tLoadbarBg, pt);

	// loadbar
	Rect r = { pt.x, pt.y, pt.x + 8 + int(progress*(503 - 8)), LONG(pt.y + img_size.y) };
	Rect rp = { 0, 0, 8 + int(progress*(503 - 8)), LONG(img_size.y) };
	gui->DrawSpriteRectPart(tLoadbar, r, rp);

	// text
	Rect r2 = { 32, 0, gui->wndSize.x - 32, LONG(gui->wndSize.y - img_size.y - 32) };
	gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_BOTTOM, Color::White, r2);
}

//=================================================================================================
void LoadScreen::Setup(float min_progress, float max_progress, int steps, cstring str)
{
	assert(max_progress >= min_progress && InRange(min_progress, 0.f, 1.f) && InRange(max_progress, 0.f, 1.f) && steps > 0);
	this->min_progress = min_progress;
	this->max_progress = max_progress;
	this->steps = steps;
	if(str)
		text = str;
	else
		text.clear();
	progress = min_progress;
	step = 0;
	engine->DoPseudotick();
}

//=================================================================================================
void LoadScreen::Tick(cstring str)
{
	assert(step != steps);
	++step;
	progress = min_progress + (max_progress - min_progress) * step / steps;
	if(str)
		text = str;
	engine->DoPseudotick();
}
