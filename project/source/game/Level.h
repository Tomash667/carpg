#pragma once

enum EnterFrom
{
	ENTER_FROM_PORTAL = 0,
	ENTER_FROM_OUTSIDE = -1,
	ENTER_FROM_UP_LEVEL = -2,
	ENTER_FROM_DOWN_LEVEL = -3
};

class Level
{
public:
	Location* location; // same as W.current_location
	int location_index; // same as W.current_location_index
	LocationEventHandler* event_handler;
	int enter_from; // from where team entered level (used when spawning new player in MP)
	float light_angle; // random angle used for lighting in outside locations
	bool is_open; // is location loaded, team is inside or is on world map and can reenter
};
extern Level L;
