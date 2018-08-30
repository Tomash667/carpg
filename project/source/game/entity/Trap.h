#pragma once

//-----------------------------------------------------------------------------
#include "BaseTrap.h"
#include "Object.h"

//-----------------------------------------------------------------------------
struct Trap
{
	int netid;
	BaseTrap* base;
	int state, dir;
	float time;
	vector<Unit*>* hitted;
	Int2 tile;
	Vec3 pos;
	Object obj, obj2;
	bool trigger; // u¿ywane u klienta w MP

	static const int MIN_SIZE = 31;
	static int netid_counter;

	Trap() : hitted(nullptr)
	{
	}
	~Trap()
	{
		delete hitted;
	}

	void Save(FileWriter& f, bool local);
	void Load(FileReader& f, bool local);
};
