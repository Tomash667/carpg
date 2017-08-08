// pocisk (strza�a, czar)
#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Unit;
struct Spell;
struct ParticleEmitter;
struct TrailParticleEmitter;

//-----------------------------------------------------------------------------
struct Bullet
{
	Vec3 pos, rot, start_pos;
	Mesh* mesh;
	float speed, timer, attack, tex_size, yspeed, poison_attack;
	int level, backstab;
	Unit* owner;
	Spell* spell;
	TexturePtr tex;
	TrailParticleEmitter* trail, *trail2;
	ParticleEmitter* pe;
	bool remove;

	static const int MIN_SIZE = 41;

	void Save(FileWriter& f);
	void Load(FileReader& f);
};
