#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "Container.h"

//-----------------------------------------------------------------------------
class GlobalGui : public GameComponent, public Container
{
public:
	GlobalGui();
	~GlobalGui();
	void Prepare() override;
	void InitOnce() override;
	void LoadLanguage() override;
	void LoadData() override;
	void PostInit() override;
	void Cleanup() override;
	void Draw(ControlDrawData*) override;
	void UpdateGui(float dt);
	void LoadOldGui(FileReader& f);
	void Clear(bool reset_mpbox);
	void Setup(PlayerController* pc);
	void OnResize();
	void OnFocus(bool focus, const Int2& activation_point);
	void ShowMenu() { GUI.ShowDialog((DialogBox*)game_menu); }
	void ShowOptions() { GUI.ShowDialog((DialogBox*)options); }
	void ShowMultiplayer();
	void ShowSavePanel();
	void ShowLoadPanel();
	void ShowQuitDialog();
	void ShowCreateCharacterPanel(bool enter_name, bool redo = false);

	// panels
	LoadScreen* load_screen;
	GameGui* game_gui;
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
