#include "Pch.h"
#include "GameCore.h"
#include "MainMenu.h"
#include "Language.h"
#include "Version.h"
#include "DialogBox.h"
#include "ResourceManager.h"
#include "Game.h"
#include "GlobalGui.h"
#include "SaveLoadPanel.h"
#include <thread>
#include "LobbyApi.h"

extern string g_ctime;

//=================================================================================================
MainMenu::MainMenu(Game* game) : game(game), check_status(CheckVersionStatus::None), check_updates(game->check_updates)
{
	focusable = true;
	visible = false;
}

//=================================================================================================
void MainMenu::LoadLanguage()
{
	txInfoText = Str("infoText");
	txVersion = Str("version");

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
		b.text = Str(names[i]);
		b.size = GUI.default_font->CalculateSize(b.text) + Int2(24, 24);

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
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("menu_bg.jpg", MainMenu::tBackground);
	tex_mgr.AddLoadTask("logo.png", MainMenu::tLogo);
}

//=================================================================================================
void MainMenu::Draw(ControlDrawData* /*cdd*/)
{
	GUI.DrawSpriteFull(tBackground, Color::White);
	GUI.DrawSprite(tLogo, Int2(GUI.wnd_size.x - 512 - 16, 16));

	Rect r = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
	r.Top() = r.Bottom() - 64;
	GUI.DrawText(GUI.default_font, "Devmode(2013,2018) Tomashu & Leinnan", DTF_CENTER | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

	r.Left() = GUI.wnd_size.x - 512 - 16;
	r.Right() = GUI.wnd_size.x - 16;
	r.Top() = 256 + 24;
	r.Bottom() = r.Top() + 64;
	GUI.DrawText(GUI.default_font, Format(txVersion, VERSION_STR), DTF_CENTER | DTF_OUTLINE, Color::White, r);

	r.Left() = 0;
	r.Right() = GUI.wnd_size.x;
	r.Bottom() = GUI.wnd_size.y - 16;
	r.Top() = r.Bottom() - 64;
	GUI.DrawText(GUI.default_font, version_text, DTF_CENTER | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

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
#ifdef _DEBUG
		check_updates = false;
#endif
		if(check_updates)
		{
			version_text = Str("checkingVersion");
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
			version_text = Format(Str("newVersion"), str);
			Info("New version %s is available.", str);

			// show dialog box with question about updating
			DialogInfo info;
			info.event = delegate<void(int)>(this, &MainMenu::OnNewVersion);
			info.name = "new_version";
			info.order = ORDER_TOP;
			info.parent = nullptr;
			info.pause = false;
			info.text = Format(Str("newVersionDialog"), VERSION_STR, VersionToString(version_new));
			info.type = DIALOG_YESNO;
			cstring names[] = { Str("download"), Str("skip") };
			info.custom_names = names;

			GUI.ShowDialog(info);
		}
		else if(version_new < VERSION)
		{
			version_text = Str("newerVersion");
			Info("You have newer version then available.");
		}
		else
		{
			version_text = Str("noNewVersion");
			Info("No new version available.");
		}
		check_status = CheckVersionStatus::Finished;
		check_version_thread.join();
	}
	else if(check_status == CheckVersionStatus::Error)
	{
		version_text = Str("checkVersionError");
		Error("Failed to check version.");
		check_status = CheckVersionStatus::Finished;
		check_version_thread.join();
	}
}

//=================================================================================================
void MainMenu::CheckVersion()
{
	version_new = N.api->GetVersion([&]() {return check_status == CheckVersionStatus::Cancel; });
	if(version_new < 0)
		check_status = CheckVersionStatus::Error;
	else
		check_status = CheckVersionStatus::Done;
}

//=================================================================================================
void MainMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_WindowResize)
		PlaceButtons();
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdNewGame:
			Net::SetMode(Net::Mode::Singleplayer);
			game->gui->ShowCreateCharacterPanel(true);
			break;
		case IdLoadGame:
			Net::SetMode(Net::Mode::Singleplayer);
			game->gui->saveload->ShowLoadPanel();
			break;
		case IdMultiplayer:
			game->gui->ShowMultiplayer();
			break;
		case IdOptions:
			game->gui->ShowOptions();
			break;
		case IdInfo:
			GUI.SimpleDialog(Format(txInfoText, VERSION_STR, g_ctime.c_str()), nullptr);
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
		bt[i].pos = bt[i].global_pos = Int2(16 + GUI.wnd_size.x - 200 + int(sin(kat)*(GUI.wnd_size.x - 200)), 100 + int(cos(kat)*GUI.wnd_size.y));
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
	if(check_status != CheckVersionStatus::Finished)
	{
		check_status = CheckVersionStatus::Cancel;
		check_version_thread.join();
		check_status = CheckVersionStatus::Cancel;
	}
}
