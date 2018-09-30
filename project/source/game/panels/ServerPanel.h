#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "Grid.h"
#include "InputTextBox.h"
#include "Class.h"

//-----------------------------------------------------------------------------
enum LobbyUpdate
{
	Lobby_UpdatePlayer,
	Lobby_AddPlayer,
	Lobby_RemovePlayer,
	Lobby_KickPlayer,
	Lobby_ChangeCount,
	Lobby_ChangeLeader
};

//-----------------------------------------------------------------------------
class ServerPanel : public GameDialogBox
{
public:
	explicit ServerPanel(const DialogInfo& info);
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void UpdateLobby(float dt);
	void UpdateLobbyClient(float dt);
	bool DoLobbyUpdate(BitStreamReader& f);
	void UpdateLobbyServer(float dt);
	void UpdateServerInfo();
	void Event(GuiEvent e) override;
	void Show();
	void GetCell(int item, int column, Cell& cell);
	void ExitLobby(VoidF callback = nullptr);
	void AddMsg(cstring text);
	void OnKick(int id);
	void OnInput(const string& str);
	void StopStartup();
	void UseLoadedCharacter(bool have);
	void CheckAutopick();
	void PickClass(Class clas, bool ready);
	void AddLobbyUpdate(const Int2& u);
	void ChangeReady();
	void CheckReady();
	bool Quickstart();
	cstring TryStart();
	void Start();

	Grid grid;
	InputTextBox itb;
	vector<Int2> lobby_updates;
	string server_name;
	float update_timer, startup_timer;
	uint max_players, autostart_count;
	int last_startup_sec;
	Class autopick_class;
	bool starting, autoready, password;
	cstring txReady, txNotReady, txStart, txStop, txPickChar, txKick, txNone, txSetLeader, txNick, txChar, txLoadedCharInfo, txNotLoadedCharInfo, txChangeChar,
		txCantKickMyself, txCantKickUnconnected, txReallyKick, txAlreadyLeader, txLeaderChanged, txNotJoinedYet, txNotAllReady, txStartingIn, txStartingStop,
		txDisconnecting, txYouAreLeader, txJoined, txPlayerLeft, txNeedSelectedPlayer, txServerText, txDisconnected, txClosing, txKicked, txUnknown,
		txUnconnected, txIpLostConnection, txPlayerLostConnection, txLeft, txStartingGame, txWaitingForServer;
	TEX tGotowy, tNieGotowy;
};
