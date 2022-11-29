#pragma once

//-----------------------------------------------------------------------------
#include <Container.h>

//-----------------------------------------------------------------------------
// Container for all game gui panels & dialogs
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
	void Draw() override;
	void Draw(const Matrix& mat_view_proj, bool draw_gui, bool draw_dialogs);
	void UpdateGui(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Clear(bool reset_mpbox, bool on_enter);
	void Setup(PlayerController* pc);
	void OnResize();
	void OnFocus(bool focus, const Int2& activationPoint);
	void ShowMenu() { gui->ShowDialog((DialogBox*)game_menu); }
	void ShowOptions() { gui->ShowDialog((DialogBox*)options); }
	void ShowMultiplayer();
	void ShowQuitDialog();
	void ShowCreateCharacterPanel(bool enter_name, bool redo = false);
	void CloseAllPanels(bool close_mp_box = false);
	void AddMsg(cstring msg);
	void ChangeControls();

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
	AbilityPanel* ability;
	BookPanel* book;
	CraftPanel* craft;
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
	cstring txReallyQuit, txReallyQuitHardcore;
};
