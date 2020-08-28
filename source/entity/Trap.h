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
	bool mpTrigger;

	static const int MIN_SIZE = 31;

	Trap() : hitted(nullptr) {}
	~Trap() { delete hitted; }
	bool Update(float dt, LevelArea& area);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
