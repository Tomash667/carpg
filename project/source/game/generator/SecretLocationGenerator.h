#pragma once

#include "OutsideLocationGenerator.h"

class SecretLocationGenerator : public OutsideLocationGenerator
{
public:
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	void SpawnTeam() override;
};
