#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Level.h"
#include "Game.h"

Net N;
const float CHANGE_LEVEL_TIMER = 5.f;

Net::Net() : peer(nullptr)
{
}

void Net::Cleanup()
{
	DeleteElements(players);
	if(peer)
		RakPeerInterface::DestroyInstance(peer);
}

PlayerInfo* Net::FindPlayer(Cstring nick)
{
	assert(nick);
	for(PlayerInfo* info : players)
	{
		if(info->left == PlayerInfo::LEFT_NO && info->name == nick)
			return info;
	}
	return nullptr;
}

PlayerInfo* Net::FindPlayer(const SystemAddress& adr)
{
	assert(adr != UNASSIGNED_SYSTEM_ADDRESS);
	for(PlayerInfo* info : players)
	{
		if(info->adr == adr)
			return info;
	}
	return nullptr;
}

PlayerInfo* Net::TryGetPlayer(int id)
{
	for(PlayerInfo* info : players)
	{
		if(info->id == id)
		{
			if(info->left != PlayerInfo::LEFT_NO)
				return info;
			break;
		}
	}
	return nullptr;
}

void Net::Send(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr, StreamLogType type)
{
	peer->Send(&f.GetBitStream(), priority, reliability, 0, adr, false);
	Game::Get().StreamWrite(f.GetBitStream(), type, adr);
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
	BitStreamWriter f;
	f.WriteCasted<byte>('C');
	f.WriteCasted<byte>('A');
	f << VERSION;
	f.WriteCasted<byte>(active_players);
	f.WriteCasted<byte>(max_players);
	byte flags = 0;
	if(!game.server_pswd.empty())
		flags |= SERVER_PASSWORD;
	if(game.mp_load)
		flags |= SERVER_SAVED;
	f.WriteCasted<byte>(flags);
	f << game.server_name;

	peer->SetOfflinePingResponse(f.GetData(), f.GetSize());
}

void Net::OnChangeLevel(int level)
{
	BitStreamWriter f;
	f << ID_CHANGE_LEVEL;
	f << (byte)L.location_index;
	f << (byte)level;
	f << false;

	uint ack = SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, Stream_TransferServer);
	Game& game = Game::Get();
	for(PlayerInfo* info : players)
	{
		if(info->id == game.my_id)
			info->state = PlayerInfo::IN_GAME;
		else
		{
			info->state = PlayerInfo::WAITING_FOR_RESPONSE;
			info->ack = ack;
			info->timer = CHANGE_LEVEL_TIMER;
		}
	}
}

uint Net::SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type)
{
	Game& game = Game::Get();
	if(active_players <= 1)
		return 0;
	uint ack = peer->Send(&f.GetBitStream(), priority, reliability, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	game.StreamWrite(f.GetBitStream(), type, UNASSIGNED_SYSTEM_ADDRESS);
	return ack;
}
