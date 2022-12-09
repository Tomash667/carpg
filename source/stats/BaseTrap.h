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
	cstring meshId;
	MeshPtr mesh;
	float rw, h;
	cstring soundId, soundId2, soundId3;
	SoundPtr sound, sound2, sound3;
	float soundDist, soundDist2, soundDist3;
	ResourceState state;

	BaseTrap(cstring id, int attack, TRAP_TYPE type, cstring meshId, cstring soundId, float soundDist, cstring soundId2, float soundDist2, cstring soundId3,
		float soundDist3) :
		id(id), attack(attack), type(type), meshId(meshId), mesh(nullptr), rw(0), h(0), soundId(soundId), soundId2(soundId2), soundId3(soundId3),
		sound(nullptr), sound2(nullptr), sound3(nullptr), state(ResourceState::NotLoaded), soundDist(soundDist), soundDist2(soundDist2), soundDist3(soundDist3)
	{
	}

	static BaseTrap* Get(const string& id);

	static BaseTrap traps[];
	static const uint nTraps;
};
