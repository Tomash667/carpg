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

	string name, sound_id, sound2_id, tex_id, tex2_id, tex3_id, mesh_id;
	SOUND sound, sound2;
	Texture tex, tex2, tex3;
	VEC2 cooldown;
	Type type;
	int id, flags, dmg, dmg_bonus;
	float range, size, size2, speed, explode_range;
	VEC2 sound_dist, sound_dist2;
	btCollisionShape* shape;
	Animesh* mesh;

	Spell(int id, cstring name, Type type, int flags, int dmg, int dmg_bonus, const VEC2& cooldown, float range, float speed, cstring _tex_id, cstring _tex2_id, cstring _tex3_id,
		float size, float size2, cstring _sound_id, cstring _sound2_id, const VEC2& sound_dist, const VEC2& sound_dist2, float explode_range, cstring _mesh_id) :
		id(id), name(name), cooldown(cooldown), type(type), flags(flags), dmg(dmg), range(range), sound(NULL), sound2(NULL), tex(NULL), tex2(NULL), tex3(NULL), speed(speed), sound_dist(sound_dist),
		sound_dist2(sound_dist2), size(size), size2(size2), shape(NULL), explode_range(explode_range), mesh_id(mesh_id), mesh(NULL), dmg_bonus(dmg_bonus)
	{
		if(_tex_id)
			tex_id = _tex_id;
		if(_tex2_id)
			tex2_id = _tex2_id;
		if(_tex3_id)
			tex3_id = _tex3_id;
		if(_sound_id)
			sound_id = _sound_id;
		if(_sound2_id)
			sound2_id = _sound2_id;
		if(_mesh_id)
			mesh_id = _mesh_id;
	}
};
extern Spell g_spells[];
extern const uint n_spells;

//-----------------------------------------------------------------------------
Spell* FindSpell(cstring name);
void LoadSpells(uint& crc);
