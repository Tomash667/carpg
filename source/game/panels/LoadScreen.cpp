#include "Pch.h"
#include "Base.h"
#include "LoadScreen.h"
#include "Game.h"

TEX LoadScreen::tBackground;

//=================================================================================================
void LoadScreen::Draw(ControlDrawData*)
{
	Game& game = Game::Get();

	// t³o
	GUI.DrawSpriteFull(tBackground, WHITE);

	D3DSURFACE_DESC desc;
	V(game.tWczytywanie[0]->GetLevelDesc(0, &desc));

	INT2 pt((GUI.wnd_size.x - desc.Width) / 2, GUI.wnd_size.y - desc.Height - 16);

	// t³o paska
	GUI.DrawSprite(game.tWczytywanie[0], pt);

	// pasek
	RECT r = { pt.x, pt.y, pt.x + 8 + int(game.load_game_progress*(503 - 8)), pt.y + (int)desc.Height };
	RECT rp = { 0, 0, 8 + int(game.load_game_progress*(503 - 8)), (int)desc.Height };
	GUI.DrawSpriteRectPart(game.tWczytywanie[1], r, rp);

	// tekst
	RECT r2 = { 32, 0, GUI.wnd_size.x - 32, GUI.wnd_size.y - (int)desc.Height - 32 };
	GUI.DrawText(GUI.default_font, game.load_game_text, DT_CENTER | DT_BOTTOM, WHITE, r2);
}