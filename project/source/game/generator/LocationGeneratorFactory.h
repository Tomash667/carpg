#pragma once

#include "LocationGenerator.h"

class LocationGeneratorFactory
{
public:
	void InitOnce();
	void Clear();
	LocationGenerator* Get(Location* loc, bool first = false, bool reenter = false);

private:
	CampGenerator* camp;
	CaveGenerator* cave;
	CityGenerator* city;
	DungeonGenerator* dungeon;
	EncounterGenerator* encounter;
	ForestGenerator* forest;
	LabyrinthGenerator* labyrinth;
	MoonwellGenerator* moonwell;
	SecretLocationGenerator* secret;
	TutorialLocationGenerator* tutorial;
};
