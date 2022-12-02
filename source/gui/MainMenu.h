#pragma once

//-----------------------------------------------------------------------------
#include <Control.h>
#include <Button.h>
#include <TooltipController.h>

//-----------------------------------------------------------------------------
// Main menu panel
class MainMenu : public Control
{
public:
	enum Id
	{
		IdContinue = GuiEvent_Custom,
		IdNewGame,
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
	void Draw() override;
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

	static const uint BUTTONS = 8u;

	void PlaceButtons();
	void OnNewVersion(int id);
	void CheckVersion();
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);

	TooltipController tooltip;
	Button bt[BUTTONS];
	TexturePtr tBackground, tLogo, tFModLogo;
	CheckVersionStatus checkStatus;
	int version_new;
	string version, versionText, versionChangelog;
	cstring txInfoText, txCheckingVersion, txNewVersion, txNewVersionDialog, txChanges, txDownload, txUpdate, txSkip, txNewerVersion, txNoNewVersion,
		txCheckVersionError;
	thread checkVersionThread;
	bool checkUpdates, versionUpdate;
};
