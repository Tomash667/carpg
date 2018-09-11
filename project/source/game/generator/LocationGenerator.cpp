#include "Pch.h"
#include "GameCore.h"
#include "LocationGenerator.h"
#include "Location.h"
#include "Level.h"
#include "AIController.h"
#include "Game.h"

//=================================================================================================
int LocationGenerator::GetNumberOfSteps()
{
	int steps = 3; // common txEnteringLocation, txGeneratingMinimap, txLoadingComplete
	if(loc->state != LS_ENTERED && loc->state != LS_CLEARED)
		++steps; // txGeneratingMap
	return steps;
}

//=================================================================================================
void LocationGenerator::RespawnUnits()
{
	Game& game = Game::Get();

	for(LevelContext& ctx : L.ForEachContext())
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			Unit* u = *it;
			if(u->player)
				continue;

			// model
			u->action = A_NONE;
			u->talking = false;
			u->CreateMesh(Unit::CREATE_MESH::NORMAL);

			// fizyka
			game.CreateUnitPhysics(*u, true);

			// ai
			AIController* ai = new AIController;
			ai->Init(u);
			game.ais.push_back(ai);
		}
	}
}
