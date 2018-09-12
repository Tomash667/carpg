#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Game.h"

Net N;

Net::Net() : peer(nullptr)
{
}

void Net::Cleanup()
{
	if(peer)
		RakPeerInterface::DestroyInstance(peer);
}

//=================================================================================================
void Net::UpdateServerInfo()
{
	Game& game = Game::Get();

	// 0 char C
	// 1 char A
	// 2-5 int - version
	// 6 byte - players count
	// 7 byte - max players
	// 8 byte - flags
	// 9+ byte - name
	server_info.Reset();
	BitStreamWriter f(server_info);
	f.WriteCasted<byte>('C');
	f.WriteCasted<byte>('A');
	f << VERSION;
	f.WriteCasted<byte>(game.players);
	f.WriteCasted<byte>(game.max_players);
	byte flags = 0;
	if(!game.server_pswd.empty())
		flags |= SERVER_PASSWORD;
	if(game.mp_load)
		flags |= SERVER_SAVED;
	f.WriteCasted<byte>(flags);
	f << game.server_name;

	peer->SetOfflinePingResponse((cstring)server_info.GetData(), server_info.GetNumberOfBytesUsed());
}
