#include "Pch.h"
#include "LocationGenerator.h"
#include "Location.h"
#include "Level.h"
#include "AIController.h"
#include "Game.h"
#include "Unit.h"

//=================================================================================================
int LocationGenerator::GetNumberOfSteps()
{
	int steps = 3; // common txEnteringLocation, txGeneratingMinimap, txLoadingComplete
	if(loc->last_visit == -1)
		++steps; // txGeneratingMap
	return steps;
}

//=================================================================================================
void LocationGenerator::RespawnUnits()
{
	for(LevelArea& area : game_level->ForEachArea())
	{
		for(Unit* u : area.units)
		{
			if(u->player)
				continue;

			// model
			u->action = A_NONE;
			u->talking = false;
			u->CreateMesh(Unit::CREATE_MESH::NORMAL);

			// fizyka
			u->CreatePhysics(true);

			// ai
			AIController* ai = new AIController;
			ai->Init(u);
			game->ais.push_back(ai);

			// refresh stock
			if(u->data->trader)
				u->RefreshStock();
		}
	}
}
