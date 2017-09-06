#pragma once

//-----------------------------------------------------------------------------
#include "GamePacket.h"
#include "NetChange.h"
#include "NetChangePlayer.h"

//-----------------------------------------------------------------------------
enum AttackId
{
	AID_Attack,
	AID_PowerAttack, // or just start holding attack key
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
	InvalidItemsCrc,
	BrokenPacket,
	OtherError,
	InvalidNick,
	InvalidSpellsCrc,
	InvalidUnitsCrc,
	InvalidDialogsCrc,
	InvalidTypeCrc
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

	static vector<NetChange> changes;
	static vector<NetChangePlayer> player_changes;

private:
	static Mode mode;
};
