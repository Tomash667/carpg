#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
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
	SKILL
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
	Client_WaitingForPasswordProxy,
	Client_Connecting,
	Client_ConnectingProxy,
	Client_Punchthrough,
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
enum class MasterServerState
{
	NotConnected,
	Connecting,
	Registering,
	Connected
};

//-----------------------------------------------------------------------------
class Net : public GameComponent
{
	enum StartFlags
	{
		SF_DEVMODE = 1 << 0,
		SF_INVISIBLE = 1 << 1,
		SF_NOCLIP = 1 << 2,
		SF_GODMODE = 1 << 3,
		SF_NOAI = 1 << 4
	};

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
	void InitOnce() override;
	void LoadLanguage() override;
	void Cleanup() override;
	void WriteNetVars(BitStreamWriter& f);
	void ReadNetVars(BitStreamReader& f);
	bool ValidateNick(cstring nick);

	//****************************************************************************
	// Common
	//****************************************************************************
public:
	PlayerInfo& GetMe() { return *players[0]; }
	PlayerInfo* FindPlayer(Cstring nick);
	PlayerInfo* FindPlayer(const SystemAddress& adr);
	PlayerInfo* TryGetPlayer(int id);
	void ClosePeer(bool wait = false);

	LobbyApi* api;
	RakPeerInterface* peer;
	vector<PlayerInfo*> players; // contains players that left too
	vector<string*> net_strs;
	float update_timer, mp_interp;
	int port;
	bool mp_load, mp_load_worldmap, mp_use_interp;

	//****************************************************************************
	// Server
	//****************************************************************************
public:
	void InitServer();
	void OnChangeLevel(int level);
	void SendServer(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr, StreamLogType type);
	uint SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type);
	int GetNewPlayerId();
	PlayerInfo* FindOldPlayer(cstring nick);
	void DeleteOldPlayers();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void InterpolatePlayers(float dt);
	void KickPlayer(PlayerInfo& info);
	void FilterServerChanges();
	bool FilterOut(NetChangePlayer& c);
	void WriteWorldData(BitStreamWriter& f);
	void WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info);
	void WriteLevelData(BitStream& stream, bool loaded_resources);
	int GetServerFlags();

	vector<PlayerInfo*> old_players;
	uint active_players, max_players;
	string server_name, password;
	int last_id;
	MasterServerState master_server_state;
	SystemAddress master_server_adr;
	bool players_left, server_hidden;

	//****************************************************************************
	// Client
	//****************************************************************************
public:
	void InitClient();
	void SendClient(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, StreamLogType type);
	void InterpolateUnits(float dt);
	void FilterClientChanges();
	bool FilterOut(NetChange& c);
	bool ReadWorldData(BitStreamReader& f);
	bool ReadPlayerStartData(BitStreamReader& f);
	bool ReadLevelData(BitStreamReader& f);

	SystemAddress server;
	string net_adr;
	bool was_client;

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
