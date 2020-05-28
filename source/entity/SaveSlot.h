#pragma once

//-----------------------------------------------------------------------------
#include "Date.h"

//-----------------------------------------------------------------------------
struct SaveSlot
{
	static const int MAX_SLOTS = 11;

	string text, player_name, location;
	vector<string> mp_players;
	Date game_date;
	int load_version;
	Class* player_class;
	time_t save_date;
	uint img_size, img_offset;
	bool valid, hardcore, on_worldmap;
};
