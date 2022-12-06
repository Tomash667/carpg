#include "Pch.h"
#include "LocationGeneratorFactory.h"

#include "AcademyGenerator.h"
#include "BaseLocation.h"
#include "CampGenerator.h"
#include "CaveGenerator.h"
#include "CityGenerator.h"
#include "DungeonGenerator.h"
#include "EncounterGenerator.h"
#include "ForestGenerator.h"
#include "HillsGenerator.h"
#include "InsideLocation.h"
#include "LabyrinthGenerator.h"
#include "Level.h"
#include "MoonwellGenerator.h"
#include "OutsideLocation.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "SecretLocationGenerator.h"
#include "TutorialLocationGenerator.h"

//=================================================================================================
LocationGeneratorFactory::LocationGeneratorFactory()
{
	academy = new AcademyGenerator;
	camp = new CampGenerator;
	cave = new CaveGenerator;
	city = new CityGenerator;
	dungeon = new DungeonGenerator;
	encounter = new EncounterGenerator;
	forest = new ForestGenerator;
	hills = new HillsGenerator;
	labyrinth = new LabyrinthGenerator;
	moonwell = new MoonwellGenerator;
	secret = new SecretLocationGenerator;
	tutorial = new TutorialLocationGenerator;
}

//=================================================================================================
LocationGeneratorFactory::~LocationGeneratorFactory()
{
	delete academy;
	delete camp;
	delete cave;
	delete city;
	delete dungeon;
	delete encounter;
	delete forest;
	delete hills;
	delete labyrinth;
	delete moonwell;
	delete secret;
	delete tutorial;
}

//=================================================================================================
void LocationGeneratorFactory::Init()
{
	OutsideLocationGenerator::InitOnce();
	camp->InitOnce();
}

//=================================================================================================
LocationGenerator* LocationGeneratorFactory::Get(Location* loc, bool first)
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
	case L_OUTSIDE:
		if(gameLevel->locationIndex == questMgr->questSecret->where2)
		{
			loc_gen = secret;
			break;
		}
		switch(loc->target)
		{
		default:
		case FOREST:
			loc_gen = forest;
			break;
		case HILLS:
			loc_gen = hills;
			break;
		case MOONWELL:
			loc_gen = moonwell;
			break;
		case ACADEMY:
			loc_gen = academy;
			break;
		case HUNTERS_CAMP:
			loc_gen = camp;
			break;
		}
		break;
	case L_CAMP:
		loc_gen = camp;
		break;
	case L_DUNGEON:
		{
			InsideLocation* inside = (InsideLocation*)loc;
			BaseLocation& base = g_base_locations[inside->target];
			if(inside->target == TUTORIAL_FORT)
				loc_gen = tutorial;
			else if(IsSet(base.options, BLO_LABYRINTH))
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
	loc_gen->dungeonLevel = gameLevel->dungeonLevel;
	loc_gen->first = first;
	loc_gen->Init();
	return loc_gen;
}
