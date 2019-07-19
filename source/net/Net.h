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
public:
	enum class Mode
	{
		Singleplayer,
		Server,
		Client
	};

	static Mode GetMode() { return mode; }

	static bool IsLocal() { return mode != Mode::Client; }
	static bool IsOnline() { return mode != Mode::Singleplayer; }
	static bool IsServer() { return mode == Mode::Server; }
	static bool IsClient() { return mode == Mode::Client; }
	static bool IsSingleplayer() { return mode == Mode::Singleplayer; }

	static void SetMode(Mode _mode) { mode = _mode; }

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
	PlayerInfo& GetMe() { return players[0]; }
	PlayerInfo* FindPlayer(Cstring nick);
	PlayerInfo* FindPlayer(const SystemAddress& adr);
	PlayerInfo* TryGetPlayer(int id);
	void ClosePeer(bool wait = false, bool check_was_client = false);

	LobbyApi* api;
	RakPeerInterface* peer;
	rvector<PlayerInfo> players; // contains players that left too
	vector<string*> net_strs;
	float update_timer, mp_interp;
	int port;
	bool mp_load, mp_load_worldmap, mp_use_interp, prepare_world, mp_quickload;

	//****************************************************************************
	// Server
	//****************************************************************************
public:
	void InitServer();
	void OnChangeLevel(int level);
	void SendServer(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr);
	uint SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability);
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
	void ClearChanges();

	rvector<PlayerInfo> old_players;
	uint active_players, max_players;
	string server_name, password;
	int last_id;
	MasterServerState master_server_state;
	SystemAddress master_server_adr;
	bool players_left, server_lan;

	//****************************************************************************
	// Client
	//****************************************************************************
public:
	void InitClient();
	void SendClient(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability);
	void InterpolateUnits(float dt);
	void FilterClientChanges();
	bool FilterOut(NetChange& c);
	bool ReadWorldData(BitStreamReader& f);
	bool ReadPlayerStartData(BitStreamReader& f);
	bool ReadLevelData(BitStreamReader& f);

	SystemAddress server, ping_adr;
	bool was_client, join_lan;

private:
	static Mode mode;
	cstring txCreateServerFailed, txInitConnectionFailed;
};
extern Net N;
