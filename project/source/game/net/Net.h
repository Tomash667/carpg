#pragma once

//-----------------------------------------------------------------------------
#include "GamePacket.h"
#include "NetChange.h"
#include "NetChangePlayer.h"

//-----------------------------------------------------------------------------
enum AttackId
{
	AID_Attack,
	AID_PrepareAttack,
	AID_Shoot,
	AID_StartShoot,
	AID_Block,
	AID_Bash,
	AID_RunningAttack,
	AID_StopBlock
};

//-----------------------------------------------------------------------------
enum class ChangedStatType
{
	ATTRIBUTE,
	SKILL,
	BASE_ATTRIBUTE,
	BASE_SKILL
};

//-----------------------------------------------------------------------------
enum class JoinResult
{
	Ok = -1,
	FullServer = 0,
	InvalidVersion,
	TakenNick,
	BrokenPacket,
	OtherError,
	InvalidNick,
	InvalidCrc
};

//-----------------------------------------------------------------------------
enum class NetState
{
	Client_PingIp,
	Client_WaitingForPassword,
	Client_Connecting,
	Client_BeforeTransfer,
	Client_ReceivedWorldData,
	Client_ReceivedPlayerStartData,
	Client_ChangingLevel,
	Client_ReceivedLevelData,
	Client_ReceivedPlayerData,
	Client_Start,
	Client_StartOnWorldmap,

	Server_Starting,
	Server_Initialized,
	Server_WaitForPlayersToLoadWorld,
	Server_EnterLocation,
	Server_Send
};

//-----------------------------------------------------------------------------
enum StreamLogType
{
	Stream_None,
	Stream_PickServer,
	Stream_PingIp,
	Stream_Connect,
	Stream_Quitting,
	Stream_QuittingServer,
	Stream_Transfer,
	Stream_TransferServer,
	Stream_ServerSend,
	Stream_UpdateLobbyServer,
	Stream_UpdateLobbyClient,
	Stream_UpdateGameServer,
	Stream_UpdateGameClient,
	Stream_Chat
};

//-----------------------------------------------------------------------------
class Net
{
public:
	enum class Mode
	{
		Singleplayer,
		Server,
		Client
	};

	static Mode GetMode()
	{
		return mode;
	}

	static bool IsLocal()
	{
		return mode != Mode::Client;
	}
	static bool IsOnline()
	{
		return mode != Mode::Singleplayer;
	}
	static bool IsServer()
	{
		return mode == Mode::Server;
	}
	static bool IsClient()
	{
		return mode == Mode::Client;
	}
	static bool IsSingleplayer()
	{
		return mode == Mode::Singleplayer;
	}

	static void SetMode(Mode _mode)
	{
		mode = _mode;
	}

	static NetChange& PushChange(NetChange::TYPE type)
	{
		auto& c = Add1(changes);
		c.type = type;
		return c;
	}

	static vector<NetChange> changes;

	Net();
	void LoadLanguage();
	void Cleanup();

	//****************************************************************************
	// Common
	//****************************************************************************
public:
	PlayerInfo& GetMe() { return *players[0]; }
	PlayerInfo* FindPlayer(Cstring nick);
	PlayerInfo* FindPlayer(const SystemAddress& adr);
	PlayerInfo* TryGetPlayer(int id);
	void Send(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr, StreamLogType type);

	RakPeerInterface* peer;
	vector<PlayerInfo*> players; // contains players that left too
	int port;

	//****************************************************************************
	// Server
	//****************************************************************************
public:
	void InitServer();
	void UpdateServerInfo();
	void OnChangeLevel(int level);
	uint SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type);

	uint active_players, max_players;
	string server_name, password;
	bool starting;

	//****************************************************************************
	// Client
	//****************************************************************************
	void InitClient();

	//****************************************************************************
	BitStream& StreamStart(Packet* packet, StreamLogType type);
	void StreamEnd();
	void StreamError();
	template<typename... Args>
	inline void StreamError(cstring msg, const Args&... args)
	{
		Error(msg, args...);
		StreamError();
	}
	void StreamWrite(vector<byte>& data, StreamLogType type, const SystemAddress& adr)
	{
		StreamWrite(data.data(), data.size(), type, adr);
	}
	void StreamWrite(BitStream& data, StreamLogType type, const SystemAddress& adr)
	{
		StreamWrite(data.GetData(), data.GetNumberOfBytesUsed(), type, adr);
	}
	void StreamWrite(Packet* packet, StreamLogType type, const SystemAddress& adr)
	{
		StreamWrite(packet->data, packet->length, type, adr);
	}
	void StreamWrite(const void* data, uint size, StreamLogType type, const SystemAddress& adr);

private:
	static Mode mode;
	BitStream current_stream;
	Packet* current_packet;
	cstring txCreateServerFailed, txInitConnectionFailed;
};
extern Net N;
