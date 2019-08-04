#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
enum class MusicType
{
	None,
	Intro,
	Title,
	Forest,
	City,
	Crypt,
	Dungeon,
	Boss,
	Travel,
	Moonwell,
	Death
};

//-----------------------------------------------------------------------------
struct MusicTrack
{
	MusicPtr music;
	MusicType type;

	static vector<MusicTrack*> tracks;
	static uint Load(uint& errors);
	static void Cleanup();
};
