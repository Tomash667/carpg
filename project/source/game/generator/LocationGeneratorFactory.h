#pragma once

class LocationGeneratorFactory
{
public:
	void InitOnce();
	void Clear();
	LocationGenerator* Get(Location* loc);

private:
	CaveGenerator* cave;
	CityGenerator* city;
	DungeonGenerator* dungeon;
	EncounterGenerator* encounter;
	ForestGenerator* forest;
	LabyrinthGenerator* labyrinth;
};
