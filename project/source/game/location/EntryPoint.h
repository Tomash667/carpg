#pragma once

//-----------------------------------------------------------------------------
struct EntryPoint
{
	BOX2D spawn_area;
	float spawn_rot;
	BOX2D exit_area;
	float exit_y;
};

//-----------------------------------------------------------------------------
// City gates
#define GATE_NORTH 0x01
#define GATE_SOUTH 0x02
#define GATE_WEST 0x04
#define GATE_EAST 0x08
