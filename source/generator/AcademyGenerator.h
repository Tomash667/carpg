#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class AcademyGenerator final : public OutsideLocationGenerator
{
private:
	void Init() override;
	void Generate() override;
	void SpawnBuilding(bool first);
	void GenerateObjects() override;
	void GenerateItems() override;
	void GenerateUnits() override;
	void OnEnter() override;
	void SpawnTeam() override;
	void OnLoad() override;

	Building* building;
	Vec3 unit_pos;
};
