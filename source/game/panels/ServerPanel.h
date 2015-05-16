#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "Grid.h"
#include "InputTextBox.h"

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
class ServerPanel : public Dialog
{
public:
	ServerPanel(const DialogInfo& info);
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Event(GuiEvent e);

	void Show();
	void GetCell(int item, int column, Cell& cell);
	void ExitLobby(VoidF f=NULL);
	void AddMsg(cstring text);
	void OnKick(int id);
	void OnInput(const string& str);
	void StopStartup();
	void UseLoadedCharacter(bool have);
	void CheckAutopick();

	Grid grid;
	cstring txReady, txNotReady, txStart, txStop, txStarting, txPickChar, txKick, txNone, txSetLeader, txNick, txChar, txLoadedCharInfo, txNotLoadedCharInfo, txChangeChar, txCantKickMyself,
		txCantKickUnconnected, txReallyKick, txAlreadyLeader, txLeaderChanged, txNotJoinedYet, txNotAllReady, txStartingIn, txStartingStop, txDisconnecting, txYouAreLeader, txJoined, txPlayerLeft,
		txNeedSelectedPlayer, txServerText;
	bool have_char;
	InputTextBox itb;
};
