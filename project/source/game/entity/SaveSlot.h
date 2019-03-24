#pragma once

//-----------------------------------------------------------------------------
struct SaveSlot
{
	static const int MAX_SLOTS = 11;

	string text, player_name, location;
	vector<string> mp_players;
	int game_day, game_month, game_year, load_version;
	Class player_class;
	time_t save_date;
	uint img_size, img_offset;
	bool valid, hardcore, on_worldmap;
};
