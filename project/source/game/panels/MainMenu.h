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
	enum class CheckVersionStatus
	{
		None,
		Checking,
		Done,
		Finished,
		Error,
		Cancel
	};

	static const uint BUTTONS = 7u;

	void PlaceButtons();
	void OnNewVersion(int id);
	void CheckVersion();
	void UpdateCheckVersion();

	Game* game;
	Button bt[BUTTONS];
	TEX tBackground, tLogo;
	CheckVersionStatus check_status;
	int version_new;
	string version_text;
	cstring txInfoText, txVersion;
	thread check_version_thread;
	bool check_updates;
};
