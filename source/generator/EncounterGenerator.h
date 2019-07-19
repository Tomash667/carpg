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

	GameDirection enter_dir; // direction from where team entered encounter
	bool far_encounter; // is team far from other units at encounter
};
