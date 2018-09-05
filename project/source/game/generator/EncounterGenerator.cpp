#include "Pch.h"
#include "GameCore.h"
#include "EncounterGenerator.h"

int EncounterGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	steps += 4; // txGeneratingObjects, txRecreatingObjects, txGeneratingUnits, txGeneratingItems
	return steps;
}
