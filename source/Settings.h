#pragma once

//-----------------------------------------------------------------------------
#include "SaveSlot.h"

//-----------------------------------------------------------------------------
// quickstart mode
enum QUICKSTART
{
	QUICKSTART_NONE,
	QUICKSTART_SINGLE,
	QUICKSTART_HOST,
	QUICKSTART_JOIN_LAN,
	QUICKSTART_JOIN_IP,
	QUICKSTART_LOAD,
	QUICKSTART_LOAD_MP
};

//-----------------------------------------------------------------------------
struct Settings
{
	Settings() : grass_range(40.f), quickstart(QUICKSTART_NONE), quickstart_slot(SaveSlot::MAX_SLOTS), testing(false) {}

	int mouse_sensitivity;
	float mouse_sensitivity_f;
	float grass_range;
	QUICKSTART quickstart;
	int quickstart_slot;
	bool testing;

	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys(Config& cfg);
	void LoadGameKeys(Config& cfg);
};
