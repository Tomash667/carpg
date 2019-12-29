#pragma once

//-----------------------------------------------------------------------------
#include "Unit.h"
#include "CreatedCharacter.h"

//-----------------------------------------------------------------------------
// Server side player info
struct PlayerInfo
{
	// max 8 atm
	enum UPDATE_FLAGS
	{
		UF_POISON_DAMAGE = 1 << 0,
		UF_GOLD = 1 << 1,
		UF_ALCOHOL = 1 << 2,
		UF_LEARNING_POINTS = 1 << 3,
		UF_LEVEL = 1 << 4,
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
		LEFT_NO,
		LEFT_TIMEOUT,
		LEFT_DISCONNECTED,
		LEFT_QUIT,
		LEFT_KICK,
		LEFT_SERVER_FULL
	};

	string name;
	PlayerController* pc;
	Unit* u;
	Class* clas;
	int id, ack, update_flags;
	SystemAddress adr;
	float timer;
	bool ready, devmode, warping, loaded, fast_travel;
	HumanData hd;
	CreatedCharacter cc;
	vector<string> notes;
	LeftReason left;
	vector<NetChangePlayer> changes;

	PlayerInfo();
	void UpdateGold() { update_flags |= PlayerInfo::UF_GOLD; }
	void Save(FileWriter& f);
	void Load(FileReader& f);
	int GetIndex() const;
};
