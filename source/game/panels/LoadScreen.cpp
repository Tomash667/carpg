#include "Pch.h"
#include "Base.h"
#include "LoadScreen.h"
#include "Game.h"

//=================================================================================================
void LoadScreen::Draw(ControlDrawData*)
{
	Game& game = Game::Get();

	// background
	GUI.DrawSpriteFull(tBackground, WHITE);

	// loadbar background
	D3DSURFACE_DESC desc;
	tLoadbarBg->GetLevelDesc(0, &desc);
	INT2 pt((GUI.wnd_size.x - desc.Width) / 2, GUI.wnd_size.y - desc.Height - 16);
	GUI.DrawSprite(tLoadbarBg, pt);

	// loadbar
	RECT r = {pt.x, pt.y, pt.x+8+int(progress*(503-8)), pt.y+desc.Height};
	RECT rp = {0, 0, 8+int(progress*(503-8)), desc.Height};
	GUI.DrawSpriteRectPart(tLoadbar, r, rp);

	// text
	RECT r2 = {32, 0, GUI.wnd_size.x-32, GUI.wnd_size.y-desc.Height-32};
	GUI.DrawText(GUI.default_font, text, DT_CENTER|DT_BOTTOM, WHITE, r2);
}

//=================================================================================================
void LoadScreen::LoadData()
{
	ResourceManager& resMgr = ResourceManager::Get();
	resMgr.GetLoadedTexture("loadbar_bg.png", tLoadbarBg);
	resMgr.GetLoadedTexture("loadbar.png", tLoadbar);
	resMgr.GetLoadedTexture("load_bg.jpg", tBackground);
}
