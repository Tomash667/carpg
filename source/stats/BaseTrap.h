#pragma once

//-----------------------------------------------------------------------------
#include "Mesh.h"

//-----------------------------------------------------------------------------
// Trap types
enum TRAP_TYPE
{
	TRAP_SPEAR,
	TRAP_ARROW,
	TRAP_POISON,
	TRAP_FIREBALL,
	TRAP_BEAR,
	TRAP_MAX
};

//-----------------------------------------------------------------------------
// Base trap
struct BaseTrap
{
	cstring id;
	int attack;
	TRAP_TYPE type;
	cstring mesh_id;
	MeshPtr mesh;
	float rw, h;
	cstring sound_id, sound_id2, sound_id3;
	SoundPtr sound, sound2, sound3;
	float sound_dist, sound_dist2, sound_dist3;
	ResourceState state;

	BaseTrap(cstring id, int attack, TRAP_TYPE type, cstring mesh_id, cstring sound_id, float sound_dist, cstring sound_id2, float sound_dist2,
		cstring sound_id3, float sound_dist3) :
		id(id), attack(attack), type(type), mesh_id(mesh_id), mesh(nullptr), rw(0), h(0), sound_id(sound_id), sound_id2(sound_id2), sound_id3(sound_id3),
		sound(nullptr), sound2(nullptr), sound3(nullptr), state(ResourceState::NotLoaded), sound_dist(sound_dist), sound_dist2(sound_dist2),
		sound_dist3(sound_dist3)
	{
	}

	static BaseTrap* Get(const string& id);

	static BaseTrap traps[];
	static const uint n_traps;
};
