#pragma once

//-----------------------------------------------------------------------------
#include "Container.h"

//-----------------------------------------------------------------------------
class GameGui : public Container
{
public:
	GameGui();
	~GameGui();
	void PreInit();
	void Init();
	void LoadLanguage();
	void LoadData();
	void PostInit();
	void UpdateGui(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Clear(bool reset_mpbox, bool on_enter);
	void Setup(PlayerController* pc);
	void OnResize();
	void OnFocus(bool focus, const Int2& activation_point);
	void ShowMenu() { gui->ShowDialog((DialogBox*)game_menu); }
	void ShowOptions() { gui->ShowDialog((DialogBox*)options); }
	void ShowMultiplayer();
	void ShowQuitDialog();
	void ShowCreateCharacterPanel(bool enter_name, bool redo = false);
	void CloseAllPanels(bool close_mp_box = false);
	void AddMsg(cstring msg);

	static FontPtr font, font_small, font_big;
	Notifications* notifications;
	// panels
	LoadScreen* load_screen;
	LevelGui* level_gui;
	Inventory* inventory;
	StatsPanel* stats;
	TeamPanel* team;
	Journal* journal;
	Minimap* minimap;
	ActionPanel* actions;
	BookPanel* book;
	GameMessages* messages;
	MpBox* mp_box;
	WorldMapGui* world_map;
	MainMenu* main_menu;
	// dialogs
	Console* console;
	GameMenu* game_menu;
	Options* options;
	SaveLoad* saveload;
	CreateCharacterPanel* create_character;
	MultiplayerPanel* multiplayer;
	CreateServerPanel* create_server;
	PickServerPanel* pick_server;
	ServerPanel* server;
	InfoBox* info_box;
	Controls* controls;
	// settings
	bool cursor_allow_move;

private:
	cstring txReallyQuit;
};
