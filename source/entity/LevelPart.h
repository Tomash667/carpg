#pragma once

//-----------------------------------------------------------------------------
#include "Collision.h"
#include "Drain.h"

//-----------------------------------------------------------------------------
// Part of active level
struct LevelPart : ObjectPoolProxy<LevelPart>
{
	LocationPart* locPart;
	vector<Bullet*> bullets;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*> tpes;
	vector<Explo*> explos;
	vector<Electro*> electros;
	vector<Drain> drains;
	vector<CollisionObject> colliders;
	float lights_dt;

	void Clear();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
