#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <Grid.h>
#include <InputTextBox.h>
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
// Lobby shown after creating server, players can chat and select characters
class ServerPanel : public DialogBox
{
public:
	explicit ServerPanel(const DialogInfo& info);
	void Init();
	void LoadLanguage();
	void LoadData();
	void Draw() override;
	void Update(float dt) override;
	void UpdateLobby(float dt);
	void UpdateLobbyClient(float dt);
	bool DoLobbyUpdate(BitStreamReader& f);
	void UpdateLobbyServer(float dt);
	void OnChangePlayersCount();
	void UpdateServerInfo();
	void Event(GuiEvent e) override;
	void Show();
	void GetCell(int item, int column, Cell& cell);
	void ExitLobby(VoidF callback = nullptr);
	void AddMsg(cstring text) { itb.Add(text); }
	void OnKick(int id);
	void OnInput(const string& str);
	void StopStartup();
	void UseLoadedCharacter(bool have);
	void CheckAutopick();
	void PickClass(Class* clas, bool ready);
	void AddLobbyUpdate(const Int2& u);
	void ChangeReady();
	void CheckReady();
	bool Quickstart();
	cstring TryStart();
	void Start();

	Grid grid;
	InputTextBox itb;
	vector<Int2> lobby_updates;
	string serverName;
	float update_timer, startup_timer;
	uint max_players, autostart_count;
	int last_startup_sec, kick_id;
	Class* autopick_class;
	bool starting, autoready;
	cstring txReady, txNotReady, txStart, txStop, txPickChar, txKick, txNone, txSetLeader, txNick, txChar, txLoadedCharInfo, txNotLoadedCharInfo, txChangeChar,
		txCantKickMyself, txCantKickUnconnected, txReallyKick, txAlreadyLeader, txLeaderChanged, txNotJoinedYet, txNotAllReady, txStartingIn, txStartingStop,
		txDisconnecting, txYouAreLeader, txJoined, txPlayerLeft, txNeedSelectedPlayer, txServerText, txDisconnected, txClosing, txKicked, txUnknown,
		txUnconnected, txIpLostConnection, txPlayerLostConnection, txLeft, txStartingGame, txWaitingForServer, txRegisterFailed, txPlayerDisconnected;

private:
	TexturePtr tReady, tNotReady;
	Class* random_class;
};
