#pragma once

//-----------------------------------------------------------------------------
#include "Collision.h"
#include "Drain.h"

//-----------------------------------------------------------------------------
// Part of active level
struct LevelPart
{
	LocationPart* const locPart;
	Scene* scene;
	vector<Bullet*> bullets;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*> tpes;
	vector<Explo*> explos;
	vector<Electro*> electros;
	vector<Drain> drains;
	vector<CollisionObject> colliders;
	float lightsDt, drawRange;

	LevelPart(LocationPart* locPart);
	~LevelPart();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
