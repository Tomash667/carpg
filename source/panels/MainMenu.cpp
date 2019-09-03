#include "Pch.h"
#include "GameCore.h"
#include "MainMenu.h"
#include "Language.h"
#include "Version.h"
#include "DialogBox.h"
#include "ResourceManager.h"
#include "Game.h"
#include "GameGui.h"
#include "SaveLoadPanel.h"
#include <thread>
#include "LobbyApi.h"
#include "Utility.h"

//=================================================================================================
MainMenu::MainMenu() : check_status(CheckVersionStatus::None), check_updates(game->check_updates)
{
	focusable = true;
	visible = false;
}

//=================================================================================================
void MainMenu::LoadLanguage()
{
	Language::Section& s = Language::GetSection("MainMenu");
	txInfoText = s.Get("infoText");
	txVersion = s.Get("version");
	txCheckingVersion = s.Get("checkingVersion");
	txNewVersion = s.Get("newVersion");
	txNewVersionDialog = s.Get("newVersionDialog");
	txChanges = s.Get("changes");
	txDownload = s.Get("download");
	txSkip = s.Get("skip");
	txNewerVersion = s.Get("newerVersion");
	txNoNewVersion = s.Get("noNewVersion");
	txCheckVersionError = s.Get("checkVersionError");

	const cstring names[BUTTONS] = {
		"newGame",
		"loadGame",
		"multiplayer",
		"options",
		"website",
		"info",
		"quit"
	};

	Int2 maxsize(0, 0);

	// stwórz przyciski
	for(int i = 0; i < BUTTONS; ++i)
	{
		Button& b = bt[i];
		b.id = IdNewGame + i;
		b.parent = this;
		b.text = s.Get(names[i]);
		b.size = GameGui::font->CalculateSize(b.text) + Int2(24, 24);

		maxsize = Int2::Max(maxsize, b.size);
	}

	// ustaw rozmiar
	for(int i = 0; i < BUTTONS; ++i)
		bt[i].size = maxsize;

	PlaceButtons();
}

//=================================================================================================
void MainMenu::LoadData()
{
	tBackground = res_mgr->Load<Texture>("menu_bg.jpg");
	tLogo = res_mgr->Load<Texture>("logo.png");
	tFModLogo = res_mgr->Load<Texture>("fmod_logo.png");
}

//=================================================================================================
void MainMenu::Draw(ControlDrawData*)
{
	gui->DrawSpriteFull(tBackground, Color::White);
	gui->DrawSprite(tLogo, Int2(gui->wnd_size.x - 512 - 16, 16));
	gui->DrawSpriteRect(tFModLogo, Rect(int(gui->wnd_size.x - 562.f * gui->wnd_size.x / 1920), int(gui->wnd_size.y - 185.f * gui->wnd_size.y / 1080),
		int(gui->wnd_size.x - 50.f * gui->wnd_size.x / 1920), int(gui->wnd_size.y - 50.f * gui->wnd_size.y / 1080)), Color::Alpha(250));

	Rect r = { 0, 0, gui->wnd_size.x, gui->wnd_size.y };
	r.Top() = r.Bottom() - 64;
	gui->DrawText(GameGui::font, "Devmode(2013,2019) Tomashu & Leinnan", DTF_CENTER | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

	r.Left() = gui->wnd_size.x - 512 - 16;
	r.Right() = gui->wnd_size.x - 16;
	r.Top() = 256 + 24;
	r.Bottom() = r.Top() + 64;
	gui->DrawText(GameGui::font, Format(txVersion, VERSION_STR), DTF_CENTER | DTF_OUTLINE, Color::White, r);

	r.Left() = 0;
	r.Right() = gui->wnd_size.x;
	r.Bottom() = gui->wnd_size.y - 16;
	r.Top() = r.Bottom() - 64;
	gui->DrawText(GameGui::font, version_text, DTF_CENTER | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

	for(int i = 0; i < BUTTONS; ++i)
	{
		if(bt[i].visible)
			bt[i].Draw();
	}
}

//=================================================================================================
void MainMenu::Update(float dt)
{
	for(int i = 0; i < BUTTONS; ++i)
	{
		bt[i].mouse_focus = focus;
		bt[i].Update(dt);
	}

	UpdateCheckVersion();
}

//=================================================================================================
void MainMenu::UpdateCheckVersion()
{
	if(check_status == CheckVersionStatus::None)
	{
		if(check_updates)
		{
			Info("Checking CaRpg version.");
			check_status = CheckVersionStatus::Checking;
			check_version_thread = thread(&MainMenu::CheckVersion, this);
		}
		else
			check_status = CheckVersionStatus::Finished;
	}
	else if(check_status == CheckVersionStatus::Done)
	{
		if(version_new > VERSION)
		{
			cstring str = VersionToString(version_new);
			version_text = Format(txNewVersion, str);
			Info("New version %s is available.", str);

			// show dialog box with question about updating
			DialogInfo info;
			info.event = delegate<void(int)>(this, &MainMenu::OnNewVersion);
			info.name = "new_version";
			info.order = ORDER_TOP;
			info.parent = nullptr;
			info.pause = false;
			info.text = Format(txNewVersionDialog, VERSION_STR, VersionToString(version_new));
			if(!version_changelog.empty())
				info.text += Format("\n\n%s\n%s", txChanges, version_changelog.c_str());
			info.type = DIALOG_YESNO;
			cstring names[] = { txDownload, txSkip };
			info.custom_names = names;

			gui->ShowDialog(info);
		}
		else if(version_new < VERSION)
		{
			version_text = txNewerVersion;
			Info("You have newer version then available.");
		}
		else
		{
			version_text = txNoNewVersion;
			Info("No new version available.");
		}
		check_status = CheckVersionStatus::Finished;
		check_version_thread.join();
	}
	else if(check_status == CheckVersionStatus::Error)
	{
		version_text = txCheckVersionError;
		Error("Failed to check version.");
		check_status = CheckVersionStatus::Finished;
		check_version_thread.join();
	}
}

//=================================================================================================
void MainMenu::CheckVersion()
{
	auto cancel = [&]() { return check_status == CheckVersionStatus::Cancel; };
	version_new = net->api->GetVersion(cancel);
	if(version_new < 0)
		check_status = CheckVersionStatus::Error;
	else
	{
		version_changelog.clear();
		if(version_new > VERSION)
			net->api->GetChangelog(version_changelog, cancel);
		check_status = CheckVersionStatus::Done;
	}
}

//=================================================================================================
void MainMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_Show)
	{
		if(check_status == CheckVersionStatus::Checking)
			version_text = txCheckingVersion;
	}
	else if(e == GuiEvent_WindowResize)
		PlaceButtons();
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdNewGame:
			Net::SetMode(Net::Mode::Singleplayer);
			game_gui->ShowCreateCharacterPanel(true);
			break;
		case IdLoadGame:
			Net::SetMode(Net::Mode::Singleplayer);
			game_gui->saveload->ShowLoadPanel();
			break;
		case IdMultiplayer:
			game_gui->ShowMultiplayer();
			break;
		case IdOptions:
			game_gui->ShowOptions();
			break;
		case IdInfo:
			gui->SimpleDialog(Format(txInfoText, VERSION_STR, utility::GetCompileTime().c_str()), nullptr);
			break;
		case IdWebsite:
			io::OpenUrl(Format("http://carpg.pl/redirect.php?language=%s", Language::prefix.c_str()));
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
		bt[i].pos = bt[i].global_pos = Int2(16 + gui->wnd_size.x - 200 + int(sin(kat)*(gui->wnd_size.x - 200)), 100 + int(cos(kat)*gui->wnd_size.y));
		kat += PI / 4 / BUTTONS;
	}
}

//=================================================================================================
void MainMenu::OnNewVersion(int id)
{
	if(id == BUTTON_YES)
		io::OpenUrl(Format("http://carpg.pl/redirect.php?action=download&language=%s", Language::prefix.c_str()));
}

//=================================================================================================
void MainMenu::ShutdownThread()
{
	if(check_status != CheckVersionStatus::Finished && check_status != CheckVersionStatus::None)
	{
		check_status = CheckVersionStatus::Cancel;
		check_version_thread.join();
		check_status = CheckVersionStatus::Cancel;
	}
	else if(check_status == CheckVersionStatus::Done)
		check_version_thread.join();
}
