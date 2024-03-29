#include "Pch.h"
#include "MainMenu.h"

#include "Game.h"
#include "GameGui.h"
#include "Language.h"
#include "LobbyApi.h"
#include "SaveLoadPanel.h"
#include "Utility.h"
#include "Version.h"

#include <DialogBox.h>
#include <ResourceManager.h>
#include <thread>

//=================================================================================================
MainMenu::MainMenu() : checkStatus(CheckVersionStatus::None), checkUpdates(game->checkUpdates)
{
	focusable = true;
	visible = false;
}

//=================================================================================================
void MainMenu::LoadLanguage()
{
	Language::Section s = Language::GetSection("MainMenu");
	txInfoText = s.Get("infoText");
	txCheckingVersion = s.Get("checkingVersion");
	txNewVersion = s.Get("newVersion");
	txNewVersionDialog = s.Get("newVersionDialog");
	txChanges = s.Get("changes");
	txDownload = s.Get("download");
	txUpdate = s.Get("update");
	txSkip = s.Get("skip");
	txNewerVersion = s.Get("newerVersion");
	txNoNewVersion = s.Get("noNewVersion");
	txCheckVersionError = s.Get("checkVersionError");

	version = Format(s.Get("version"), VERSION_STR, Split(utility::GetCompileTime().c_str())[0].c_str());

	// create buttons
	const cstring names[BUTTONS] = {
		"continue",
		"newGame",
		"loadGame",
		"multiplayer",
		"options",
		"website",
		"info",
		"quit"
	};

	Int2 maxsize(0, 0);

	for(int i = 0; i < BUTTONS; ++i)
	{
		Button& b = bt[i];
		b.id = IdContinue + i;
		b.parent = this;
		b.text = s.Get(names[i]);
		b.size = GameGui::font->CalculateSize(b.text) + Int2(24, 24);

		maxsize = Int2::Max(maxsize, b.size);
	}

	for(int i = 0; i < BUTTONS; ++i)
		bt[i].size = maxsize;

	PlaceButtons();

	// tooltip
	tooltip.Init(TooltipController::Callback(this, &MainMenu::GetTooltip));
}

//=================================================================================================
void MainMenu::LoadData()
{
	tBackground = resMgr->Load<Texture>("menu_bg.jpg");
	tLogo = resMgr->Load<Texture>("logo.png");
	tFModLogo = resMgr->Load<Texture>("fmod_logo.png");
}

//=================================================================================================
void MainMenu::Draw()
{
	gui->DrawSpriteFull(tBackground);
	gui->DrawSprite(tLogo, Int2(gui->wndSize.x - 512 - 16, 16));
	gui->DrawSpriteRect(tFModLogo, Rect(
		int(gui->wndSize.x - (512.f * 0.6f + 50.f) * gui->wndSize.x / 1920),
		int(gui->wndSize.y - (135.f * 0.6f + 50.f) * gui->wndSize.y / 1080),
		int(gui->wndSize.x - 50.f * gui->wndSize.x / 1920),
		int(gui->wndSize.y - 50.f * gui->wndSize.y / 1080)), Color::Alpha(250));

	Rect r = { 0, 0, gui->wndSize.x, gui->wndSize.y };
	r.Top() = r.Bottom() - 64;
	gui->DrawText(GameGui::font, "Devmode(2013,2023) Tomashu & Leinnan", DTF_CENTER | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

	r.Left() = gui->wndSize.x - 512 - 16;
	r.Right() = gui->wndSize.x - 16;
	r.Top() = 256;
	r.Bottom() = r.Top() + 64;
	gui->DrawText(GameGui::font, version, DTF_CENTER | DTF_OUTLINE, Color::White, r);

	r.Left() = 0;
	r.Right() = gui->wndSize.x;
	r.Bottom() = gui->wndSize.y - 16;
	r.Top() = r.Bottom() - 64;
	gui->DrawText(GameGui::font, versionText, DTF_CENTER | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

	for(int i = 0; i < BUTTONS; ++i)
	{
		if(bt[i].visible)
			bt[i].Draw();
	}

	tooltip.Draw();
}

//=================================================================================================
void MainMenu::Update(float dt)
{
	for(int i = 0; i < BUTTONS; ++i)
	{
		bt[i].mouseFocus = focus;
		bt[i].Update(dt);
	}

	int group = -1, id = -1;
	if(bt[0].state == Button::HOVER)
	{
		group = 0;
		id = 0;
	}
	tooltip.UpdateTooltip(dt, group, id);

	UpdateCheckVersion();
}

//=================================================================================================
void MainMenu::UpdateCheckVersion()
{
	if(checkStatus == CheckVersionStatus::None)
	{
		if(checkUpdates)
		{
			Info("Checking CaRpg version.");
			checkStatus = CheckVersionStatus::Checking;
			checkVersionThread = thread(&MainMenu::CheckVersion, this);
		}
		else
			checkStatus = CheckVersionStatus::Finished;
	}
	else if(checkStatus == CheckVersionStatus::Done)
	{
		if(versionNew > VERSION)
		{
			cstring str = VersionToString(versionNew);
			versionText = Format(txNewVersion, str);
			Info("New version %s is available.", str);

			// show dialog box with question about updating
			DialogInfo info;
			info.event = delegate<void(int)>(this, &MainMenu::OnNewVersion);
			info.name = "new_version";
			info.order = DialogOrder::Top;
			info.parent = nullptr;
			info.pause = false;
			info.text = Format(txNewVersionDialog, VERSION_STR, VersionToString(versionNew));
			if(!versionChangelog.empty())
				info.text += Format("\n\n%s\n%s", txChanges, versionChangelog.c_str());
			info.type = DIALOG_YESNO;
			cstring names[] = { versionUpdate ? txUpdate : txDownload, txSkip };
			info.customNames = names;

			gui->ShowDialog(info);
		}
		else if(versionNew < VERSION)
		{
			versionText = txNewerVersion;
			Info("You have newer version then available.");
		}
		else
		{
			versionText = txNoNewVersion;
			Info("No new version available.");
		}
		checkStatus = CheckVersionStatus::Finished;
		checkVersionThread.join();
	}
	else if(checkStatus == CheckVersionStatus::Error)
	{
		versionText = txCheckVersionError;
		Error("Failed to check version.");
		checkStatus = CheckVersionStatus::Finished;
		checkVersionThread.join();
	}
}

//=================================================================================================
void MainMenu::CheckVersion()
{
	auto cancel = [&]() { return checkStatus == CheckVersionStatus::Cancel; };
	versionNew = api->GetVersion(cancel, versionChangelog, versionUpdate);
	checkStatus = (versionNew < 0 ? CheckVersionStatus::Error : CheckVersionStatus::Done);
}

//=================================================================================================
void MainMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_Show)
	{
		if(checkStatus == CheckVersionStatus::Checking)
			versionText = txCheckingVersion;
		if(game->lastSave != -1 && !gameGui->saveload->GetSaveSlot(game->lastSave, false).valid)
			game->SetLastSave(-1);
		bt[0].state = (game->lastSave == -1 ? Button::DISABLED : Button::NONE);
		tooltip.Clear();
	}
	else if(e == GuiEvent_WindowResize)
		PlaceButtons();
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdContinue:
			Net::SetMode(Net::Mode::Singleplayer);
			game->LoadLastSave();
			break;
		case IdNewGame:
			Net::SetMode(Net::Mode::Singleplayer);
			gameGui->ShowCreateCharacterPanel(true);
			break;
		case IdLoadGame:
			Net::SetMode(Net::Mode::Singleplayer);
			gameGui->saveload->ShowLoadPanel();
			break;
		case IdMultiplayer:
			gameGui->ShowMultiplayer();
			break;
		case IdOptions:
			gameGui->ShowOptions();
			break;
		case IdInfo:
			gui->SimpleDialog(Format(txInfoText, VERSION_STR, utility::GetCompileTime().c_str()), nullptr);
			break;
		case IdWebsite:
			io::OpenUrl(Format("https://carpg.pl/redirect.php?language=%s", Language::prefix.c_str()));
			break;
		case IdQuit:
			game->Quit();
			break;
		}
	}
}

//=================================================================================================
void MainMenu::PlaceButtons()
{
	float kat = -PI / 2;
	for(int i = 0; i < BUTTONS; ++i)
	{
		if(!bt[i].visible)
			continue;
		bt[i].pos = bt[i].globalPos = Int2(16 + gui->wndSize.x - 200 + int(sin(kat) * (gui->wndSize.x - 200)), 100 + int(cos(kat) * gui->wndSize.y));
		kat += PI / 4 / BUTTONS;
	}
}

//=================================================================================================
void MainMenu::OnNewVersion(int id)
{
	if(id == BUTTON_YES)
	{
		if(versionUpdate)
		{
			// start updater
			GetModuleFileNameA(nullptr, BUF, 256);
			cstring exe = io::FilenameFromPath(BUF);
			STARTUPINFO si = { 0 };
			si.cb = sizeof(STARTUPINFO);
			PROCESS_INFORMATION pi = { 0 };
			CreateProcess(nullptr, (char*)Format("updater.exe %s", exe), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			game->Quit();
		}
		else
			io::OpenUrl(Format("https://carpg.pl/redirect.php?action=download&language=%s", Language::prefix.c_str()));
	}
}

//=================================================================================================
void MainMenu::ShutdownThread()
{
	if(checkStatus != CheckVersionStatus::Finished && checkStatus != CheckVersionStatus::None)
	{
		checkStatus = CheckVersionStatus::Cancel;
		checkVersionThread.join();
		checkStatus = CheckVersionStatus::Cancel;
	}
	else if(checkStatus == CheckVersionStatus::Done)
		checkVersionThread.join();
}

//=================================================================================================
void MainMenu::GetTooltip(TooltipController* tooltip, int group, int id, bool refresh)
{
	tooltip->anything = true;
	SaveSlot& slot = gameGui->saveload->GetSaveSlot(game->lastSave, false);
	tooltip->img = gameGui->saveload->GetSaveImage(game->lastSave, false);
	tooltip->imgSize = Int2(256, 192);
	tooltip->bigText = slot.text;
	tooltip->text = gameGui->saveload->GetSaveText(slot);
}
