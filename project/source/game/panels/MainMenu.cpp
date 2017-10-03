#include "Pch.h"
#include "Core.h"
#include "MainMenu.h"
#include "Language.h"
#include "Version.h"
#include "DialogBox.h"
#include "Game.h"
#include "NetStats.h"
#define far
#include <wininet.h>
#include <process.h>

//-----------------------------------------------------------------------------
#pragma comment(lib, "wininet.lib")

//-----------------------------------------------------------------------------
enum CheckVersionResult
{
	CVR_None,
	CVR_InternetOpenFailed,
	CVR_InternetOpenUrlFailed,
	CVR_ReadFailed,
	CVR_InvalidSignature,
	CVR_Ok
};
CriticalSection csCheckVersion;
CheckVersionResult version_check_result;
uint version_check_error, version_new;

//=================================================================================================
CheckVersionResult CheckVersion(HINTERNET internet, cstring url, uint& error, uint& version)
{
	HINTERNET file = InternetOpenUrl(internet, url, nullptr, 0, 0, 0);
	if(!file)
	{
		// Nie mo¿na pobraæ pliku z serwera
		error = GetLastError();
		return CVR_InternetOpenUrlFailed;
	}

	int data[2];
	DWORD read;

	InternetReadFile(file, &data, sizeof(data), &read);
	if(read != 8)
	{
		// Nie mo¿na odczytaæ pliku z serwera
		error = GetLastError();
		InternetCloseHandle(file);
		return CVR_ReadFailed;
	}

	InternetCloseHandle(file);

	if(data[0] != 0x475052CA) // magic number [najpierw 0xCA a póŸniej w ascii RPG]
	{
		error = data[0];
		return CVR_InvalidSignature;
	}

	version = (data[1] & 0xFFFFFF);
	return CVR_Ok;
}

//=================================================================================================
uint __stdcall CheckVersion(void*)
{
	HINTERNET internet = InternetOpen("carpg", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);

	if(!internet)
	{
		// Nie mo¿na nawi¹zaæ po³¹czenia z serwerem
		csCheckVersion.Enter();
		version_check_error = GetLastError();
		version_check_result = CVR_InternetOpenFailed;
		csCheckVersion.Leave();
		return 1;
	}

	uint error = 0, version = 0;
	CheckVersionResult result = CheckVersion(internet, "http://carpg.pl/carpgdata/wersja", error, version);
	if(result != CVR_Ok)
		result = CheckVersion(internet, "http://dhost.info/radsun/carpgdata/wersja", error, version);

	InternetCloseHandle(internet);
	csCheckVersion.Enter();
	version_check_result = result;
	version_check_error = error;
	version_new = version;
	csCheckVersion.Leave();

	return result == CVR_Ok ? 0 : 1;
}

//=================================================================================================
MainMenu::MainMenu(Game* game, DialogEvent event, bool check_updates) : check_version(0), check_version_thread(nullptr), check_updates(check_updates),
game(game), event(event), send_stats(true)
{
	focusable = true;
	visible = false;

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
	GUI.DrawSpriteFull(tBackground, WHITE);
	GUI.DrawSprite(tLogo, Int2(GUI.wnd_size.x - 512 - 16, 16));

	Rect r = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
	r.Top() = r.Bottom() - 64;
	GUI.DrawText(GUI.default_font, "Devmode(2013,2017) Tomashu & Leinnan", DT_CENTER | DT_BOTTOM | DT_OUTLINE, WHITE, r);

	r.Left() = GUI.wnd_size.x - 512 - 16;
	r.Right() = GUI.wnd_size.x - 16;
	r.Top() = 256 + 24;
	r.Bottom() = r.Top() + 64;
	GUI.DrawText(GUI.default_font, Format(txVersion, VERSION_STR), DT_CENTER | DT_OUTLINE, WHITE, r);

	r.Left() = 0;
	r.Right() = GUI.wnd_size.x;
	r.Bottom() = GUI.wnd_size.y - 16;
	r.Top() = r.Bottom() - 64;
	GUI.DrawText(GUI.default_font, version_text, DT_CENTER | DT_BOTTOM | DT_OUTLINE, WHITE, r);

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

	if(send_stats)
	{
		send_stats = false;
		NetStats::TryUpdate();
	}

	if(check_version == 0)
	{
#ifdef _DEBUG
		check_updates = false;
#endif
		if(check_updates)
		{
			version_text = Str("checkingVersion");
			Info("Checking CaRpg version.");
			check_version = 1;
			csCheckVersion.Create();
			check_version_thread = (HANDLE)_beginthreadex(nullptr, 1024, CheckVersion, nullptr, 0, nullptr);
			if(!check_version_thread)
			{
				int error = errno;
				csCheckVersion.Free();
				check_version = 2;
				version_text = Str("checkingError");
				Error("Failed to create version checking thread (%d).", error);
			}
		}
		else
			check_version = 3;
	}
	else if(check_version == 1)
	{
		bool cleanup = false;
		csCheckVersion.Enter();
		if(version_check_result != CVR_None)
		{
			cleanup = true;
			if(version_check_result == CVR_Ok)
			{
				if(version_new > VERSION)
				{
					check_version = 4;
					cstring str = VersionToString(version_new);
					version_text = Format(Str("newVersion"), str);
					Info("New version %s is available.", str);

					// wyœwietl pytanie o pobranie nowej wersji
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
					check_version = 3;
					version_text = Str("newerVersion");
					Info("You have newer version then available.");
				}
				else
				{
					check_version = 3;
					version_text = Str("noNewVersion");
					Info("No new version available.");
				}
			}
			else
			{
				check_version = 2;
				version_text = Format(Str("checkVersionError"), version_check_result, version_check_error);
				Error("Failed to check version (%d, %d).", version_check_result, version_check_error);
			}
		}
		csCheckVersion.Leave();

		if(cleanup)
		{
			csCheckVersion.Free();
			CloseHandle(check_version_thread);
			check_version_thread = nullptr;
		}
	}
}

//=================================================================================================
void MainMenu::Event(GuiEvent e)
{
	if(e == GuiEvent_WindowResize)
		PlaceButtons();
	else if(e >= GuiEvent_Custom)
	{
		if(event)
			event(e);
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
		ShellExecute(nullptr, "open", Format("http://carpg.pl/redirect.php?action=download&language=%s", g_lang_prefix.c_str()), nullptr, nullptr, SW_SHOWNORMAL);
}
