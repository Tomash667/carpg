#pragma once

//-----------------------------------------------------------------------------
struct SaveSlot
{
	static const int MAX_SLOTS = 11;

	string text, player_name, location;
	int game_day, game_month, game_year, multiplayers;
	Class player_class;
	time_t save_date;
	bool valid, hardcore;
};
