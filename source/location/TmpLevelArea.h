#pragma once

//-----------------------------------------------------------------------------
#include "Collision.h"
#include "Drain.h"

//-----------------------------------------------------------------------------
// Temporary level area (used only for active level areas to hold temporary entities)
struct TmpLevelArea : ObjectPoolProxy<TmpLevelArea>
{
	LevelArea* area;
	Scene* scene;
	vector<Bullet*> bullets;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*> tpes;
	vector<Explo*> explos;
	vector<Electro*> electros;
	vector<Drain> drains;
	vector<CollisionObject> colliders;
	float lights_dt;

	TmpLevelArea();
	~TmpLevelArea();
	void Clear();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
