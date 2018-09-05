#pragma once

class LocationGeneratorFactory
{
public:
	void InitOnce();
	void Clear();
	LocationGenerator* Get(Location* loc);

private:
	CityGenerator* city;
	DungeonGenerator* dungeon;
	EncounterGenerator* encounter;
	ForestGenerator* forest;
};
