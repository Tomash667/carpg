#pragma once

//-----------------------------------------------------------------------------
enum MUSIC_TYPE
{
	MUSIC_TITLE,
	MUSIC_FOREST,
	MUSIC_CITY,
	MUSIC_CRYPT,
	MUSIC_DUNGEON,
	MUSIC_BOSS,
	MUSIC_TRAVEL,
	MUSIC_MOONWELL,
	MUSIC_MISSING
};

//-----------------------------------------------------------------------------
struct Music
{
	int id;
	cstring file;
	SOUND snd;
	MUSIC_TYPE type;
};

//-----------------------------------------------------------------------------
extern Music g_musics[];
extern const uint n_musics;
