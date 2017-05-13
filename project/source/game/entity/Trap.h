#pragma once

//-----------------------------------------------------------------------------
#include "BaseTrap.h"
#include "Object.h"

//-----------------------------------------------------------------------------
struct Unit;

//-----------------------------------------------------------------------------
struct Trap
{
	BaseTrap* base;
	int state, dir, netid;
	float time;
	vector<Unit*>* hitted;
	INT2 tile;
	VEC3 pos;
	Object obj, obj2;
	bool trigger; // u¿ywane u klienta w MP

	static const int MIN_SIZE = 31;

	Trap() : hitted(nullptr)
	{

	}
	~Trap()
	{
		delete hitted;
	}

	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
};
