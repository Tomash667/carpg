#pragma once

//-----------------------------------------------------------------------------
#include "GamePacket.h"
#include "NetChange.h"
#include "NetChangePlayer.h"

//-----------------------------------------------------------------------------
class PacketLogger;

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
class Net
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
	~Net();
	void Init();
	void LoadLanguage();
	void LoadData();
	bool ValidateNick(cstring nick);

	//****************************************************************************
	// Common
	//****************************************************************************
public:
	PlayerInfo& GetMe() { return players[0]; }
	PlayerInfo* FindPlayer(Cstring nick);
	PlayerInfo* FindPlayer(const SystemAddress& adr);
	PlayerInfo* TryGetPlayer(int id);
	void OpenPeer();
	void ClosePeer(bool wait = false, bool check_was_client = false);
	bool IsFastTravel() const { return fast_travel; }
	void StartFastTravel(int who);
	void CancelFastTravel(int mode, int id);
	void ClearFastTravel();
	void OnFastTravel(bool accept);
	void AddServerMsg(cstring msg);

	RakPeerInterface* peer;
	rvector<PlayerInfo> players; // contains players that left too
	vector<string*> net_strs;
	PacketLogger* packet_logger;
	float update_timer, mp_interp;
	int port;
	bool mp_load, mp_load_worldmap, mp_use_interp, prepare_world, mp_quickload;

	//****************************************************************************
	// Server
	//****************************************************************************
public:
	void InitServer();
	void OnNewGameServer();
	void UpdateServer(float dt);
	void InterpolatePlayers(float dt);
	void UpdateFastTravel(float dt);
	void ServerProcessUnits(vector<Unit*>& units);
	void UpdateWarpData(float dt);
	void ProcessLeftPlayers();
	void RemovePlayer(PlayerInfo& info);
	bool ProcessControlMessageServer(BitStreamReader& f, PlayerInfo& info);
	bool CheckMove(Unit& unit, const Vec3& pos);
	void WriteServerChanges(BitStreamWriter& f);
	void WriteServerChangesForPlayer(BitStreamWriter& f, PlayerInfo& info);
	void Server_Say(BitStreamReader& f, PlayerInfo& info);
	void Server_Whisper(BitStreamReader& f, PlayerInfo& info);
	void ClearChanges();
	void FilterServerChanges();
	bool FilterOut(NetChangePlayer& c);
	void WriteNetVars(BitStreamWriter& f);
	void WriteWorldData(BitStreamWriter& f);
	void WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info);
	void WriteLevelData(BitStreamWriter& f, bool loaded_resources);
	void WritePlayerData(BitStreamWriter& f, PlayerInfo& info);
	void SendServer(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr);
	uint SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	int GetServerFlags();
	int GetNewPlayerId();
	PlayerInfo* FindOldPlayer(cstring nick);
	void DeleteOldPlayers();
	void KickPlayer(PlayerInfo& info);
	void OnChangeLevel(int level);
	void OnLeaveLocation(int where)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::LEAVE_LOCATION;
		c.id = where;
	}

	rvector<PlayerInfo> old_players;
	uint active_players, max_players;
	string server_name, password;
	int last_id;
	MasterServerState master_server_state;
	SystemAddress master_server_adr;
	struct WarpData
	{
		Unit* u;
		int where; // <-1 - get outside the building,  >=0 - get inside the building
		float timer;
	};
	vector<WarpData> warps;
	bool players_left, server_lan;

	//****************************************************************************
	// Client
	//****************************************************************************
public:
	void InitClient();
	void OnNewGameClient();
	void UpdateClient(float dt);
	void InterpolateUnits(float dt);
	void WriteClientChanges(BitStreamWriter& f);
	bool ProcessControlMessageClient(BitStreamReader& f, bool& exit_from_server);
	bool ProcessControlMessageClientForMe(BitStreamReader& f);
	void Client_Say(BitStreamReader& f);
	void Client_Whisper(BitStreamReader& f);
	void Client_ServerSay(BitStreamReader& f);
	void FilterClientChanges();
	bool FilterOut(NetChange& c);
	void ReadNetVars(BitStreamReader& f);
	bool ReadWorldData(BitStreamReader& f);
	bool ReadPlayerStartData(BitStreamReader& f);
	bool ReadLevelData(BitStreamReader& f);
	bool ReadPlayerData(BitStreamReader& f);
	void SendClient(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability);

	SystemAddress server, ping_adr;
	float interpolate_timer;
	bool was_client, join_lan;

private:
	static Mode mode;
	TexturePtr tFastTravel, tFastTravelDeny;
	cstring txCreateServerFailed, txInitConnectionFailed, txFastTravelText, txFastTravelWaiting, txFastTravelCancel, txFastTravelDeny, txFastTravelNotSafe;
	Notification* fast_travel_notif;
	float fast_travel_timer;
	bool fast_travel;
};
