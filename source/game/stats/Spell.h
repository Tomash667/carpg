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

	cstring name, sound_id, sound2_id, tex_id, tex2_id, tex3_id, mesh_id;
	SOUND sound, sound2;
	Texture tex, tex2, tex3;
	VEC2 cooldown;
	Type type;
	int id, flags, dmg, dmg_premia;
	float range, size, size2, speed, explode_range;
	VEC2 sound_dist, sound_dist2;
	btCollisionShape* shape;
	Animesh* mesh;

	Spell(int _id, cstring _name, Type _type, int _flags, int _dmg, int dmg_premia, const VEC2& _cooldown, float _range, float _speed, cstring _tex_id, cstring _tex2_id, cstring _tex3_id,
		float _size, float _size2, cstring _sound, cstring _sound2_id, const VEC2& sound_dist, const VEC2& sound_dist2, float explode_range, cstring mesh_id) : id(_id), name(_name), sound_id(_sound),
		sound(NULL), cooldown(_cooldown), type(_type), flags(_flags), dmg(_dmg), range(_range), sound2_id(_sound2_id), sound2(NULL), tex_id(_tex_id), tex2_id(_tex2_id), tex3_id(_tex3_id), tex(NULL),
		tex2(NULL), tex3(NULL), speed(_speed), sound_dist(sound_dist), sound_dist2(sound_dist2), size(_size), size2(_size2), shape(NULL), explode_range(explode_range), mesh_id(mesh_id), mesh(NULL),
		dmg_premia(dmg_premia)
	{

	}
};
extern Spell g_spells[];
extern const uint n_spells;

//-----------------------------------------------------------------------------
Spell* FindSpell(cstring name);
