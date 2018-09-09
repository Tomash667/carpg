#pragma once

#include "OutsideLocationGenerator.h"

class EncounterGenerator final : public OutsideLocationGenerator
{
public:
	int GetNumberOfSteps() override;
	void Generate() override;
	void OnEnter() override;

private:
	void SpawnEncounterUnits(GameDialog*& dialog, Unit*& talker, Quest*& quest);
	void SpawnEncounterTeam();

	int enc_kierunek; // kierunek z której strony nadesz³a dru¿yna w czasie spotkania [tymczasowe]
	bool far_encounter; // czy dru¿yna gracza jest daleko w czasie spotkania [tymczasowe]
};
