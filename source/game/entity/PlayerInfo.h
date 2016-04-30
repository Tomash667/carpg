#pragma once

//-----------------------------------------------------------------------------
#include "Unit.h"
#include "CreatedCharacter.h"

//-----------------------------------------------------------------------------
struct PlayerController;

//-----------------------------------------------------------------------------
struct PlayerInfo
{
	enum UPDATE_FLAGS
	{
		UF_POISON_DAMAGE = 1 << 0,
		UF_NET_CHANGES = 1 << 1,
		UF_GOLD = 1 << 2,
		UF_ALCOHOL = 1 << 3,
		UF_BUFFS = 1 << 4
	};

	enum STATE
	{
		REMOVING,
		WAITING_FOR_HELLO,
		IN_LOBBY,
		WAITING_FOR_DATA,
		WAITING_FOR_RESPONSE,
		IN_GAME,
		WAITING_FOR_DATA2
	} state;

	enum LeftReason
	{
		LEFT_QUIT,
		LEFT_KICK,
		LEFT_LOADING
	};

	string name;
	Class clas;
	int id, ack, update_flags, buffs;
	SystemAddress adr;
	float timer, update_timer, yspeed;
	bool ready, devmode, warping, left, loaded;
	HumanData hd;
	CreatedCharacter cc;
	PlayerController* pc;
	Unit* u;
	vector<string> notes;
	LeftReason left_reason;

	inline void NeedUpdate()
	{
		update_flags |= PlayerInfo::UF_NET_CHANGES;
	}
	inline void UpdateGold()
	{
		update_flags |= PlayerInfo::UF_GOLD;
	}
	inline void NeedUpdateAndGold()
	{
		update_flags |= (PlayerInfo::UF_NET_CHANGES | PlayerInfo::UF_GOLD);
	}
	void Save(HANDLE file);
	void Load(HANDLE file);
};
