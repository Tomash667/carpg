#include "Pch.h"
#include "GameCore.h"
#include "LocationGeneratorFactory.h"
#include "InsideLocation.h"
#include "CampGenerator.h"
#include "CaveGenerator.h"
#include "CityGenerator.h"
#include "DungeonGenerator.h"
#include "EncounterGenerator.h"
#include "ForestGenerator.h"
#include "LabyrinthGenerator.h"
#include "MoonwellGenerator.h"
#include "SecretLocationGenerator.h"
#include "BaseLocation.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Secret.h"

void LocationGeneratorFactory::InitOnce()
{
	camp = new CampGenerator;
	cave = new CaveGenerator;
	city = new CityGenerator;
	dungeon = new DungeonGenerator;
	encounter = new EncounterGenerator;
	forest = new ForestGenerator;
	labyrinth = new LabyrinthGenerator;
	moonwell = new MoonwellGenerator;
	secret = new SecretLocationGenerator;

	OutsideLocationGenerator::InitOnce();
}

void LocationGeneratorFactory::Clear()
{
	delete camp;
	delete cave;
	delete city;
	delete dungeon;
	delete encounter;
	delete forest;
	delete labyrinth;
	delete moonwell;
	delete secret;
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
		if(L.location_index == QM.quest_secret->where2)
			loc_gen = secret;
		else
			loc_gen = forest;
		break;
	case L_CAMP:
		loc_gen = camp;
		break;
	case L_MOONWELL:
		loc_gen = moonwell;
		break;
	case L_DUNGEON:
	case L_CRYPT:
		{
			InsideLocation* inside = (InsideLocation*)loc;
			BaseLocation& base = g_base_locations[inside->target];
			if(IS_SET(base.options, BLO_LABIRYNTH))
				loc_gen = labyrinth;
			else
				loc_gen = dungeon;
		}
		break;
	case L_CAVE:
		loc_gen = cave;
		break;
	}
	loc_gen->loc = loc;
	return loc_gen;
}
