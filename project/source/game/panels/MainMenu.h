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

	MainMenu(Game* game);
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return true; }
	void ShutdownThread();

private:
	static const uint BUTTONS = 7u;

	void PlaceButtons();
	void OnNewVersion(int id);

	Game* game;
	HANDLE check_version_thread;
	Button bt[BUTTONS];
	TEX tBackground, tLogo;
	int check_version; // 0 - nie sprawdzono, 1 - trwa sprawdzanie, 2 - b³¹d, 3 - brak nowej wersji, 4 - jest nowa wersja
	string version_text;
	cstring txInfoText, txVersion;
	bool check_updates;
};
