#pragma once

//-----------------------------------------------------------------------------
#include "BaseTrap.h"
#include "Object.h"

//-----------------------------------------------------------------------------
struct Trap : public EntityType<Trap>
{
	BaseTrap* base;
	int state;
	GameDirection dir;
	float time;
	vector<Unit*>* hitted;
	Int2 tile;
	Vec3 pos;
	Object obj, obj2;
	bool trigger; // u¿ywane u klienta w MP

	static const int MIN_SIZE = 31;

	Trap() : hitted(nullptr) {}
	~Trap() { delete hitted; }
	void Save(FileWriter& f, bool local);
	void Load(FileReader& f, bool local);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
