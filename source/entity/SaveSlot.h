#pragma once

//-----------------------------------------------------------------------------
#include "Date.h"

//-----------------------------------------------------------------------------
struct SaveSlot
{
	static const int MAX_SLOTS = 11;

	string text, playerName, location;
	vector<string> mpPlayers;
	Date gameDate;
	int loadVersion;
	Class* playerClass;
	time_t saveDate;
	uint imgSize, imgOffset;
	bool valid, hardcore, onWorldmap;
};
