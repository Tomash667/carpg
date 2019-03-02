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
struct Music
{
	SoundPtr music;
	MusicType type;

	static vector<Music*> musics;
	static uint Load(uint& errors);
	static void Cleanup();
};
