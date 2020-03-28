#pragma once

//-----------------------------------------------------------------------------
#include "LocationGenerator.h"

//-----------------------------------------------------------------------------
class LocationGeneratorFactory
{
public:
	LocationGeneratorFactory();
	~LocationGeneratorFactory();
	void Init();
	LocationGenerator* Get(Location* loc, bool first = false);

private:
	AcademyGenerator* academy;
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
