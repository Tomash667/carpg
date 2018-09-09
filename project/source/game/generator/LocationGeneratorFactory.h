#pragma once

class LocationGeneratorFactory
{
public:
	void InitOnce();
	void Clear();
	LocationGenerator* Get(Location* loc);

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
};