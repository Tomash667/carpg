#include "Pch.h"
#include "GameCore.h"
#include "LocationGenerator.h"
#include "Location.h"

int LocationGenerator::GetNumberOfSteps()
{
	int steps = 3; // common txEnteringLocation, txGeneratingMinimap, txLoadingComplete
	if(loc->state != LS_ENTERED && loc->state != LS_CLEARED)
		++steps; // txGeneratingMap
	return steps;
}
