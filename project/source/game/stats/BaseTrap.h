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
	TRAP_FIREBALL
};

//-----------------------------------------------------------------------------
// Base trap
struct BaseTrap
{
	cstring id;
	int dmg;
	TRAP_TYPE type;
	cstring mesh_id, mesh_id2;
	MeshPtr mesh, mesh2;
	bool alpha;
	float rw, h;
	cstring sound_id, sound_id2, sound_id3;
	SoundPtr sound, sound2, sound3;
	ResourceState state;

	BaseTrap(cstring id, int dmg, TRAP_TYPE type, cstring mesh_id, cstring mesh_id2, bool alpha, cstring sound_id, cstring sound_id2, cstring sound_id3) : id(id), dmg(dmg),
		type(type), mesh_id(mesh_id), mesh_id2(mesh_id2), mesh(nullptr), mesh2(nullptr), alpha(alpha), rw(0), h(0), sound_id(sound_id), sound_id2(sound_id2),
		sound_id3(sound_id3), sound(sound), sound2(sound2), sound3(sound3), state(ResourceState::NotLoaded)
	{
	}

	static BaseTrap traps[];
	static const uint n_traps;
};
