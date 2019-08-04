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
	void Draw(const Matrix& mat_view_proj, bool draw_gui, bool draw_dialogs);
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
	// GUI
	bool HaveDialog() { return gui->HaveDialog(); }
	bool HaveDialog(cstring name) { return gui->HaveDialog(name); }
	bool HaveDialog(DialogBox* dialog) { return gui->HaveDialog(dialog); }
	bool HaveTopDialog(cstring name) { return gui->HaveTopDialog(name); }
	bool HavePauseDialog() { return gui->HavePauseDialog(); }
	DialogBox* GetDialog(cstring name) { return gui->GetDialog(name); }
	void ShowDialog(DialogBox* dialog) { gui->ShowDialog(dialog); }
	DialogBox* ShowDialog(const DialogInfo& info) { return gui->ShowDialog(info); }
	void CloseDialog(DialogBox* dialog) { gui->CloseDialog(dialog); }
	void CloseDialogs() { gui->CloseDialogs(); }
	void SimpleDialog(cstring text, Control* parent, cstring name = "simple") { gui->SimpleDialog(text, parent, name); }

	static FontPtr font, font_small, font_big;
	Notifications* notifications;
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
