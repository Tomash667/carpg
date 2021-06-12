#pragma once

//-----------------------------------------------------------------------------
enum class MusicType
{
	Default = -1,
	Intro,
	Title,
	Forest,
	City,
	Crypt,
	Dungeon,
	Boss,
	Travel,
	Moonwell,
	Death,
	Max
};

//-----------------------------------------------------------------------------
extern MusicList* musicLists[(int)MusicType::Max];
