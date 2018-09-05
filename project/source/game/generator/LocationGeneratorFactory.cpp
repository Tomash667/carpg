#include "Pch.h"
#include "GameCore.h"
#include "LocationGeneratorFactory.h"
#include "Location.h"
#include "CaveGenerator.h"
#include "CityGenerator.h"
#include "DungeonGenerator.h"
#include "EncounterGenerator.h"
#include "ForestGenerator.h"

void LocationGeneratorFactory::InitOnce()
{
	cave = new CaveGenerator;
	city = new CityGenerator;
	dungeon = new DungeonGenerator;
	encounter = new EncounterGenerator;
	forest = new ForestGenerator;
}

void LocationGeneratorFactory::Clear()
{
	delete cave;
	delete city;
	delete dungeon;
	delete encounter;
	delete forest;
}

LocationGenerator* LocationGeneratorFactory::Get(Location* loc)
{
	LocationGenerator* loc_gen;
	switch(loc->type)
	{
	default:
		assert(0);
	case L_CITY:
		loc_gen = city;
		break;
	case L_ENCOUNTER:
		loc_gen = encounter;
		break;
	case L_FOREST:
	case L_CAMP:
	case L_MOONWELL:
		loc_gen = forest;
		break;
	case L_DUNGEON:
	case L_CRYPT:
		loc_gen = dungeon;
		break;
	case L_CAVE:
		loc_gen = cave;
		break;
	}
	loc_gen->loc = loc;
	loc_gen->Init();
	return loc_gen;
}
