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

	string id, sound_cast_id, sound_hit_id, tex_id, tex_particle_id, tex_explode_id, mesh_id;
	SOUND sound_cast, sound_hit;
	TextureResourcePtr tex, tex_particle, tex_explode;
	VEC2 cooldown;
	Type type;
	int flags, dmg, dmg_bonus;
	float range, size, size_particle, speed, explode_range;
	VEC2 sound_cast_dist, sound_hit_dist;
	btCollisionShape* shape;
	Animesh* mesh;

	Spell() : sound_cast(nullptr), sound_hit(nullptr), tex(nullptr), tex_particle(nullptr), tex_explode(nullptr), shape(nullptr), mesh(nullptr), type(Point),
		cooldown(0, 0), flags(0), dmg(0), dmg_bonus(0), range(10.f), size(0.f), size_particle(0.f), speed(10.f), explode_range(0.f),
		sound_cast_dist(2, 8), sound_hit_dist(2, 8) {}
	~Spell()
	{
		delete shape;
	}
};
extern vector<Spell*> spells;
extern vector<std::pair<string, Spell*>> spell_alias;

//-----------------------------------------------------------------------------
inline Spell* FindSpell(cstring id)
{
	assert(id);

	for(Spell* s : spells)
	{
		if(s->id == id)
			return s;
	}

	for(auto& alias : spell_alias)
	{
		if(alias.first == id)
			return alias.second;
	}

	return nullptr;
}

void LoadSpells(uint& crc);
void CleanupSpells();
