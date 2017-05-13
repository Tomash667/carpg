#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Button.h"

//-----------------------------------------------------------------------------
struct Game;

//-----------------------------------------------------------------------------
class MainMenu : public Control
{
public:
	enum Id
	{
		IdNewGame = GuiEvent_Custom,
		IdLoadGame,
		IdMultiplayer,
		IdToolset,
		IdOptions,
		IdWebsite,
		IdInfo,
		IdQuit
	};

	MainMenu(Game* game, DialogEvent event, bool check_updates, uint skip_version);

	void Draw(ControlDrawData* cdd) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return true; }

	void LoadData();

	HANDLE check_version_thread;
	cstring txInfoText, txUrl, txVersion;

private:
	static const uint BUTTONS = 8u;

	void PlaceButtons();
	void OnNewVersion(int id);

	Game* game;
	Button bt[BUTTONS];
	TEX tBackground, tLogo;
	DialogEvent event;
	int check_version; // 0 - nie sprawdzono, 1 - trwa sprawdzanie, 2 - b³¹d, 3 - brak nowej wersji, 4 - jest nowa wersja
	uint skip_version;
	string version_text;
	bool prev_devmode, check_updates;
};
