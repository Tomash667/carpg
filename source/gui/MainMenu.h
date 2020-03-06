#pragma once

//-----------------------------------------------------------------------------
#include <Control.h>
#include <Button.h>

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
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return true; }
	void ShutdownThread();
	void UpdateCheckVersion();

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

	Button bt[BUTTONS];
	TexturePtr tBackground, tLogo, tFModLogo;
	CheckVersionStatus check_status;
	int version_new;
	string version_text, version_changelog;
	cstring txInfoText, txVersion, txCheckingVersion, txNewVersion, txNewVersionDialog, txChanges, txDownload, txSkip, txNewerVersion, txNoNewVersion,
		txCheckVersionError;
	thread check_version_thread;
	bool check_updates;
};
