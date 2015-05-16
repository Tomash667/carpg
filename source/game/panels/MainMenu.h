#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Button.h"

//-----------------------------------------------------------------------------
class MainMenu : public Control
{
public:
	enum Id
	{
		IdNewGame = GuiEvent_Custom,
		IdLoadGame,
		IdMultiplayer,
		IdOptions,
		IdWebsite,
		IdInfo,
		IdQuit
	};

	MainMenu();
	void Draw(ControlDrawData* cdd);
	void Update(float dt);
	void Event(GuiEvent e);
	bool NeedCursor() const { return true; }

	void PlaceButtons();
	void OnNewVersion(int id);

	Button bt[7];
	DialogEvent event;
	cstring txInfoText, txUrl, txVersion;
	string version_text;
	int check_version; // 0 - nie sprawdzono, 1 - trwa sprawdzanie, 2 - b³¹d, 3 - brak nowej wersji, 4 - jest nowa wersja
	HANDLE check_version_thread;
	uint skip_version;
	bool check_updates;

	static TEX tBackground, tLogo;
};
