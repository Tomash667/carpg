#pragma once

//-----------------------------------------------------------------------------
#include "BaseTrap.h"
#include "Object.h"

//-----------------------------------------------------------------------------
struct Trap : public EntityType<Trap>
{
	BaseTrap* base;
	MeshInstance* meshInst;
	int state, attack;
	GameDirection dir;
	float time;
	Entity<Unit> owner;
	vector<Unit*>* hitted;
	Int2 tile;
	Vec3 pos;
	float rot;
	bool mpTrigger;

	static const int MIN_SIZE = 31;

	Trap() : hitted(nullptr) {}
	~Trap();
	bool Update(float dt, LocationPart& locPart);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	float GetAttack() const { return float(attack != 0 ? attack : base->attack); }
};
