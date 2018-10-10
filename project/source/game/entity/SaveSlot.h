#pragma once

//-----------------------------------------------------------------------------
const int MAX_SAVE_SLOTS = 11;

//-----------------------------------------------------------------------------
struct SaveSlot
{
	string text, player_name, location;
	int game_day, game_month, game_year, multiplayers;
	Class player_class;
	time_t save_date;
	bool valid, hardcore;
};
