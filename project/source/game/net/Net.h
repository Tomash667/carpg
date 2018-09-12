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
	void Cleanup();

	//****************************************************************************
	// Common
	//****************************************************************************
public:
	RakPeerInterface* peer;
	
	//****************************************************************************
	// Server
	//****************************************************************************
public:
	void UpdateServerInfo();

private:
	BitStream server_info;

	//****************************************************************************
	// Client
	//****************************************************************************

private:
	static Mode mode;
};
extern Net N;
