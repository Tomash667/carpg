#include "Pch.h"
#include "Base.h"
#include "Music.h"

//-----------------------------------------------------------------------------
Music g_musics[] = {
	0, "Intro.ogg", nullptr, MUSIC_INTRO,
	1, "DST-Ariely.ogg", nullptr, MUSIC_TITLE,
	2, "Intro2.ogg", nullptr, MUSIC_TITLE,
	3, "Celestial_Aeon_Project__Woods_of_Eremae.ogg", nullptr, MUSIC_FOREST,
	4, "DST-ArcOfDawn.ogg", nullptr, MUSIC_FOREST,
	5, "Forest1.ogg", nullptr, MUSIC_FOREST,
	6, "south_castle.ogg", nullptr, MUSIC_CITY,
	7, "moonlight_gravities.ogg", nullptr, MUSIC_CITY,
	8, "carpg_rynek_lub_chuj.ogg", nullptr, MUSIC_CITY,
	9, "carpg3.ogg", nullptr, MUSIC_CITY,
	10, "Project_Divinity__Cemetery.ogg", nullptr, MUSIC_CRYPT,
	11, "death.ogg", nullptr, MUSIC_CRYPT,
	12, "Outside2.ogg", nullptr, MUSIC_CRYPT,
	13, "pain_adventure.ogg", nullptr, MUSIC_DUNGEON,
	14, "night_tavern.ogg", nullptr, MUSIC_DUNGEON,
	15, "jjjjj.ogg", nullptr, MUSIC_DUNGEON,
	16, "broterhood_of_black_mythril.ogg", nullptr, MUSIC_DUNGEON,
	17, "carpg_Nazi's Chaos Crystal.ogg", nullptr, MUSIC_DUNGEON,
	18, "carpg8.ogg", nullptr, MUSIC_DUNGEON,
	19, "carpg5.ogg", nullptr, MUSIC_DUNGEON,
	20, "carpg4.ogg", nullptr, MUSIC_DUNGEON,
	21, "carpg2_1.ogg", nullptr, MUSIC_DUNGEON,
	22, "DST-BattleAxe.ogg", nullptr, MUSIC_BOSS,
	23, "carpg1.ogg", nullptr, MUSIC_TRAVEL,
	24, "journey.ogg", nullptr, MUSIC_TRAVEL,
	25, "Outside.ogg", nullptr, MUSIC_TRAVEL,
	26, "For Originz.ogg", nullptr, MUSIC_MOONWELL
};
extern const uint n_musics = countof(g_musics);
