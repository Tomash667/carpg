#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
// Czar
struct Spell
{
	enum Type
	{
		Point,
		Ray,
		Target,
		Ball
	};

	enum Flags
	{
		Explode = 1<<0,
		Poison = 1<<1,
		Raise = 1<<2,
		Jump = 1<<3,
		Drain = 1<<4,
		Hold = 1<<5,
		Triple = 1<<6, // tylko dla Point i Ball
		Heal = 1<<7,
		NonCombat = 1<<8
	};

	string name, sound_cast_id, sound_hit_id, tex_id, tex_particle_id, tex_explode_id, mesh_id;
	SOUND sound_cast, sound_hit;
	Texture tex, tex_particle, tex_explode;
	VEC2 cooldown;
	Type type;
	int id, flags, dmg, dmg_bonus;
	float range, size, size_particle, speed, explode_range;
	VEC2 sound_cast_dist, sound_hit_dist;
	btCollisionShape* shape;
	Animesh* mesh;

	Spell() : sound_cast(nullptr), sound_hit(nullptr), tex(nullptr), tex_particle(nullptr), tex_explode(nullptr), shape(nullptr), mesh(nullptr), type(Point), cooldown(0, 0), id(-1), flags(0), dmg(0), dmg_bonus(0),
		range(10.f), size(0.f), size_particle(0.f), speed(10.f), explode_range(0.f), sound_cast_dist(2, 8), sound_hit_dist(2, 8) {}

	Spell(int id, cstring name, Type type, int flags, int dmg, int dmg_bonus, const VEC2& cooldown, float range, float speed, cstring _tex_id, cstring _tex_particle_id, cstring _tex_explode_id,
		float size, float size_particle, cstring _sound_cast_id, cstring _sound_hit_id, const VEC2& sound_cast_dist, const VEC2& sound_hit_dist, float explode_range, cstring _mesh_id) :
		id(id), name(name), cooldown(cooldown), type(type), flags(flags), dmg(dmg), range(range), sound_cast(nullptr), sound_hit(nullptr), tex(nullptr), tex_particle(nullptr), tex_explode(nullptr), speed(speed),
		sound_cast_dist(sound_cast_dist), sound_hit_dist(sound_hit_dist), size(size), size_particle(size_particle), shape(nullptr), explode_range(explode_range), mesh(nullptr), dmg_bonus(dmg_bonus)
	{
		if(_tex_id)
			tex_id = _tex_id;
		if(_tex_particle_id)
			tex_particle_id = _tex_particle_id;
		if(_tex_explode_id)
			tex_explode_id = _tex_explode_id;
		if(_sound_cast_id)
			sound_cast_id = _sound_cast_id;
		if(_sound_hit_id)
			sound_hit_id = _sound_hit_id;
		if(_mesh_id)
			mesh_id = _mesh_id;
	}
};
extern Spell g_spells[];
extern const uint n_spells;
extern vector<Spell*> spells;

//-----------------------------------------------------------------------------
Spell* FindSpell(cstring name);
void LoadSpells(uint& crc);
