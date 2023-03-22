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
	void Draw(const Matrix& matViewProj);
	void UpdateGui(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Clear(bool resetMpBox, bool onEnter);
	void Setup(PlayerController* pc);
	void OnResize();
	void OnFocus(bool focus, const Int2& activationPoint);
	void ShowMenu() { gui->ShowDialog((DialogBox*)gameMenu); }
	void ShowOptions() { gui->ShowDialog((DialogBox*)options); }
	void ShowMultiplayer();
	void ShowQuitDialog();
	void ShowCreateCharacterPanel(bool enterName, bool redo = false);
	void CloseAllPanels(bool closeMpBox = false);
	void AddMsg(cstring msg);
	void ChangeControls();

	static FontPtr font, fontSmall, fontBig;
	Notifications* notifications;
	// panels
	LoadScreen* loadScreen;
	LevelGui* levelGui;
	Inventory* inventory;
	StatsPanel* stats;
	TeamPanel* team;
	Journal* journal;
	Minimap* minimap;
	AbilityPanel* ability;
	BookPanel* book;
	CraftPanel* craft;
	GameMessages* messages;
	MpBox* mpBox;
	WorldMapGui* worldMap;
	MainMenu* mainMenu;
	// dialogs
	Console* console;
	GameMenu* gameMenu;
	Options* options;
	SaveLoad* saveload;
	CreateCharacterPanel* createCharacter;
	MultiplayerPanel* multiplayer;
	CreateServerPanel* createServer;
	PickServerPanel* pickServer;
	ServerPanel* server;
	InfoBox* infoBox;
	Controls* controls;
	// settings
	bool cursorAllowMove;

private:
	cstring txReallyQuit, txReallyQuitHardcore;
};
