// bazowa pu�apka
#pragma once

//-----------------------------------------------------------------------------
// Typ pu�apki
enum TRAP_TYPE
{
	TRAP_SPEAR,
	TRAP_ARROW,
	TRAP_POISON,
	TRAP_FIREBALL
};

//-----------------------------------------------------------------------------
// Bazowa pu�apka
struct BaseTrap
{
	cstring id;
	int dmg;
	TRAP_TYPE type;
	cstring mesh_id, mesh_id2;
	Mesh* mesh, *mesh2;
	bool alpha;
	float rw, h;
	cstring sound_id, sound_id2, sound_id3;
	SOUND sound, sound2, sound3;
};
extern BaseTrap g_traps[];
extern const uint n_traps;
