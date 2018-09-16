#pragma once

#include "OutsideLocationGenerator.h"

class SecretLocationGenerator final : public OutsideLocationGenerator
{
public:
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	void SpawnTeam() override;
	int HandleUpdate() override;
};
