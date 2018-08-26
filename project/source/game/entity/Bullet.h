#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Bullet
{
	Unit* owner;
	Spell* spell;
	Vec3 pos, rot, start_pos;
	Mesh* mesh;
	float speed, timer, attack, tex_size, yspeed, poison_attack;
	int level, backstab;
	TexturePtr tex;
	TrailParticleEmitter* trail, *trail2;
	ParticleEmitter* pe;
	bool remove;

	static const int MIN_SIZE = 41;

	void Save(FileWriter& f);
	void Load(FileReader& f);
};
