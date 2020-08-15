#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class CampGenerator final : public OutsideLocationGenerator
{
	enum ObjId
	{
		CAMPFIRE,
		CAMPFIRE_OFF,
		TENT,
		BEDDING,
		BENCH,
		BARREL,
		BARRELS,
		BOX,
		BOXES,
		BOW_TARGET,
		CHEST,
		TORCH,
		TORCH_OFF,
		TANNING_RACK,
		HAY,
		FIREWOOD,
		MELEE_TARGET,
		ANVIL,
		CAULDRON,
		MAX
	};

public:
	void InitOnce();
	void Init() override;
	void Generate() override;
	int HandleUpdate(int days) override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;

private:
	BaseObject* objs[MAX];
	vector<Vec2> used;
	vector<Object*> tents;
	bool isEmpty;
};
