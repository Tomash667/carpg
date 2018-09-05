#include "Pch.h"
#include "GameCore.h"
#include "ForestGenerator.h"

int ForestGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 3; // txGeneratingObjects, txGeneratingUnits, txGeneratingItems
	else if(!reenter)
		steps += 2; // txGeneratingUnits, txGeneratingPhysics
	if(!reenter)
		++steps; // txRecreatingObjects
	return steps;
}
