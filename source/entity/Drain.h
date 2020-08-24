#pragma once

//-----------------------------------------------------------------------------
struct Drain
{
	Entity<Unit> target;
	ParticleEmitter* pe;
	float t;

	bool Update(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f);
};
