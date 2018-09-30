#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "LocationGenerator.h"

//-----------------------------------------------------------------------------
class LocationGeneratorFactory : public GameComponent
{
public:
	void InitOnce() override;
	void PostInit() override;
	void Cleanup() override;
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
