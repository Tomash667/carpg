#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class EncounterGenerator final : public OutsideLocationGenerator
{
public:
	int GetNumberOfSteps() override;
	void Generate() override;
	void OnEnter() override;

private:
	void SpawnEncounterUnits(GameDialog*& dialog, Unit*& talker, Quest*& quest);
	void SpawnEncounterTeam();

	GameDirection enterDir; // direction from where team entered encounter
	bool farEncounter; // is team far from other units at encounter
};
