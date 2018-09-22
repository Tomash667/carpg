#include "Pch.h"
#include "GameCore.h"
#include "LoadScreen.h"
#include "ResourceManager.h"
#include "Engine.h"

//=================================================================================================
void LoadScreen::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tLoadbarBg = tex_mgr.GetLoadedRaw("loadbar_bg.png");
	tLoadbar = tex_mgr.GetLoadedRaw("loadbar.png");
	tBackground = tex_mgr.GetLoadedRaw("load_bg.jpg");
}

//=================================================================================================
void LoadScreen::Draw(ControlDrawData*)
{
	// background
	GUI.DrawSpriteFull(tBackground, Color::White);

	// loadbar background
	Int2 img_size = gui::GetSize(tLoadbarBg);
	Int2 pt((GUI.wnd_size.x - img_size.x) / 2, GUI.wnd_size.y - img_size.y - 16);
	GUI.DrawSprite(tLoadbarBg, pt);

	// loadbar
	Rect r = { pt.x, pt.y, pt.x + 8 + int(progress*(503 - 8)), LONG(pt.y + img_size.y) };
	Rect rp = { 0, 0, 8 + int(progress*(503 - 8)), LONG(img_size.y) };
	GUI.DrawSpriteRectPart(tLoadbar, r, rp);

	// text
	Rect r2 = { 32, 0, GUI.wnd_size.x - 32, LONG(GUI.wnd_size.y - img_size.y - 32) };
	GUI.DrawText(GUI.default_font, text, DTF_CENTER | DTF_BOTTOM, Color::White, r2);
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
	Engine::Get().DoPseudotick();
}

//=================================================================================================
void LoadScreen::Tick(cstring str)
{
	assert(step != steps);
	++step;
	progress = min_progress + (max_progress - min_progress) * step / steps;
	if(str)
		text = str;
	Engine::Get().DoPseudotick();
}
