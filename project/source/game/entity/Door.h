// drzwi
#pragma once

//-----------------------------------------------------------------------------
#include "MeshInstance.h"

//-----------------------------------------------------------------------------
// id zamka
#define LOCK_NONE 0
#define LOCK_UNLOCKABLE 1
#define LOCK_MINE 2
#define LOCK_ORCS 3
#define LOCK_TUTORIAL 100

//-----------------------------------------------------------------------------
struct Door
{
	enum State
	{
		Closed,
		Opening,
		Opening2,
		Open,
		Closing,
		Closing2,
		Max
	};

	static const int MIN_SIZE = 31;

	Vec3 pos;
	float rot;
	Int2 pt;
	int locked;
	int netid;
	bool door2;

	// lokalne zmienne
	State state;
	MeshInstance* mesh_inst;
	btCollisionObject* phy;

	Door() : door2(false), mesh_inst(nullptr)
	{
	}
	~Door()
	{
		delete mesh_inst;
	}
	bool IsBlocking() const
	{
		return state == Closed || state == Opening || state == Closing2;
	}
	bool IsBlockingView() const
	{
		return state == Closed;
	}
	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
};
