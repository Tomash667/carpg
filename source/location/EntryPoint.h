#pragma once

//-----------------------------------------------------------------------------
struct EntryPoint
{
	Box2d spawnRegion;
	float spawnRot;
	Box2d exitRegion;
	float exitY;
};

//-----------------------------------------------------------------------------
// City gates
enum GateDir
{
	GATE_NORTH = 0x01,
	GATE_SOUTH = 0x02,
	GATE_WEST = 0x04,
	GATE_EAST = 0x08
};
