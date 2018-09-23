#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Level.h"
#include "Language.h"
#include "ErrorHandler.h"
#include "Game.h"

Net N;
const float CHANGE_LEVEL_TIMER = 5.f;

Net::Net() : peer(nullptr), current_packet(nullptr), mp_use_interp(true), mp_interp(0.05f)
{
}

void Net::LoadLanguage()
{
	txCreateServerFailed = Str("createServerFailed");
	txInitConnectionFailed = Str("initConnectionFailed");
}

void Net::Cleanup()
{
	DeleteElements(players);
	if(peer)
		RakPeerInterface::DestroyInstance(peer);
}

//=================================================================================================
void Net::WriteNetVars(BitStreamWriter& f)
{
	f << mp_use_interp;
	f << mp_interp;
}

//=================================================================================================
void Net::ReadNetVars(BitStreamReader& f)
{
	f >> mp_use_interp;
	f >> mp_interp;
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
			if(info->left == PlayerInfo::LEFT_NO)
				return info;
			break;
		}
	}
	return nullptr;
}

//=================================================================================================
void Net::InitServer()
{
	Info("Creating server (port %d)...", port);

	if(!peer)
		peer = RakPeerInterface::GetInstance();

	SocketDescriptor sd(port, 0);
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(max_players + 4, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create server: RakNet error %d.", r);
		throw Format(txCreateServerFailed, r);
	}

	if(!password.empty())
	{
		Info("Set server password.");
		peer->SetIncomingPassword(password.c_str(), password.length());
	}

	peer->SetMaximumIncomingConnections((word)max_players + 4);
	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));

	Info("Server created. Waiting for connection.");

	SetMode(Mode::Server);
	starting = false;
	Info("sv_online = true");
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
	if(!password.empty())
		flags |= SERVER_PASSWORD;
	if(game.mp_load)
		flags |= SERVER_SAVED;
	f.WriteCasted<byte>(flags);
	f << server_name;

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

void Net::SendServer(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr, StreamLogType type)
{
	assert(IsServer());
	peer->Send(&f.GetBitStream(), priority, reliability, 0, adr, false);
	StreamWrite(f.GetBitStream(), type, adr);
}

uint Net::SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type)
{
	assert(IsServer());
	if(active_players <= 1)
		return 0;
	uint ack = peer->Send(&f.GetBitStream(), priority, reliability, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	StreamWrite(f.GetBitStream(), type, UNASSIGNED_SYSTEM_ADDRESS);
	return ack;
}

//=================================================================================================
void Net::InitClient()
{
	Info("Initlializing client...");

	if(!peer)
		peer = RakPeerInterface::GetInstance();

	SocketDescriptor sd;
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(1, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create client: RakNet error %d.", r);
		throw Format(txInitConnectionFailed, r);
	}
	peer->SetMaximumIncomingConnections(0);

	SetMode(Mode::Client);
	Info("sv_online = true");

	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));
}

void Net::SendClient(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type)
{
	assert(IsClient());
	peer->Send(&f.GetBitStream(), priority, reliability, 0, server, false);
	StreamWrite(f.GetBitStream(), type, server);
}

//=================================================================================================
BitStream& Net::StreamStart(Packet* packet, StreamLogType type)
{
	assert(packet);
	assert(!current_packet);
	if(current_packet)
		StreamError("Unclosed stream.");

	current_packet = packet;
	current_stream.~BitStream();
	new((void*)&current_stream)BitStream(packet->data, packet->length, false);
	ErrorHandler::Get().StreamStart(current_packet, (int)type);

	return current_stream;
}

//=================================================================================================
void Net::StreamEnd()
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(true);
	current_packet = nullptr;
}

//=================================================================================================
void Net::StreamError()
{
	if(!current_packet)
		return;

	ErrorHandler::Get().StreamEnd(false);
	current_packet = nullptr;
}

//=================================================================================================
void Net::StreamWrite(const void* data, uint size, StreamLogType type, const SystemAddress& adr)
{
	ErrorHandler::Get().StreamWrite(data, size, type, adr);
}
