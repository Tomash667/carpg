#pragma once

//-----------------------------------------------------------------------------
#include "BaseTrap.h"
#include "Object.h"

//-----------------------------------------------------------------------------
struct Trap : public EntityType<Trap>
{
	SceneNode* node;
	BaseTrap* base;
	int state;
	GameDirection dir;
	float time, rot;
	vector<Unit*>* hitted;
	Int2 tile;
	Vec3 pos;
	bool mpTrigger;

	static const int MIN_SIZE = 31;

	Trap() : node(nullptr), hitted(nullptr) {}
	~Trap() { delete hitted; }
	void Cleanup() { node = nullptr; }
	bool Update(float dt, LevelArea& area);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
