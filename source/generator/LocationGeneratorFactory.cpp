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
	LocationGenerator* locGen;
	switch(loc->type)
	{
	default:
		assert(0);
	case L_CITY:
		locGen = city;
		break;
	case L_ENCOUNTER:
		locGen = encounter;
		break;
	case L_OUTSIDE:
		if(gameLevel->locationIndex == questMgr->questSecret->where2)
		{
			locGen = secret;
			break;
		}
		switch(loc->target)
		{
		default:
		case FOREST:
			locGen = forest;
			break;
		case HILLS:
			locGen = hills;
			break;
		case MOONWELL:
			locGen = moonwell;
			break;
		case ACADEMY:
			locGen = academy;
			break;
		case HUNTERS_CAMP:
			locGen = camp;
			break;
		}
		break;
	case L_CAMP:
		locGen = camp;
		break;
	case L_DUNGEON:
		{
			InsideLocation* inside = (InsideLocation*)loc;
			BaseLocation& base = gBaseLocations[inside->target];
			if(inside->target == TUTORIAL_FORT)
				locGen = tutorial;
			else if(IsSet(base.options, BLO_LABYRINTH))
				locGen = labyrinth;
			else
				locGen = dungeon;
		}
		break;
	case L_CAVE:
		locGen = cave;
		break;
	}
	locGen->loc = loc;
	locGen->dungeonLevel = gameLevel->dungeonLevel;
	locGen->first = first;
	locGen->Init();
	return locGen;
}
