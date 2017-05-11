#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
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
class ServerPanel : public Dialog
{
public:
	explicit ServerPanel(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Show();
	void GetCell(int item, int column, Cell& cell);
	void ExitLobby(VoidF f = nullptr);
	void AddMsg(cstring text);
	void OnKick(int id);
	void OnInput(const string& str);
	void StopStartup();
	void UseLoadedCharacter(bool have);
	void CheckAutopick();
	void PickClass(Class clas, bool ready);

	Grid grid;
	cstring txReady, txNotReady, txStart, txStop, txStarting, txPickChar, txKick, txNone, txSetLeader, txNick, txChar, txLoadedCharInfo, txNotLoadedCharInfo, txChangeChar, txCantKickMyself,
		txCantKickUnconnected, txReallyKick, txAlreadyLeader, txLeaderChanged, txNotJoinedYet, txNotAllReady, txStartingIn, txStartingStop, txDisconnecting, txYouAreLeader, txJoined, txPlayerLeft,
		txNeedSelectedPlayer, txServerText;
	InputTextBox itb;
};
