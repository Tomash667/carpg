#pragma once

class Level
{
public:
	Location* location; // same as W.current_location
	int location_index; // same as W.current_location_index
};
extern Level L;
