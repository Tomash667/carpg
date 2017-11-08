#pragma once

//-----------------------------------------------------------------------------
struct EntryPoint
{
	Box2d spawn_area;
	float spawn_rot;
	Box2d exit_area;
	float exit_y;
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
