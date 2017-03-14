// podziemia z jednym poziomem
#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
struct SingleInsideLocation : public InsideLocation, public InsideLocationLevel
{
	// from ILevel
	void ApplyContext(LevelContext& ctx) override;
	// from Location
	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	void BuildRefidTable() override;
	bool FindUnit(Unit* unit, int* level) override;
	Unit* FindUnit(UnitData* unit, int& at_level) override;
	LOCATION_TOKEN GetToken() const override { return LT_SINGLE_DUNGEON; }
	// from InsideLocation
	Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) override;
	Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr) override;	
	InsideLocationLevel* GetLastLevelData() override { return (last_visit != -1 ? this : nullptr); }
	void SetActiveLevel(int level) override { assert(level == 0); }
	bool HaveUpStairs() const override { return !from_portal; }
	bool HaveDownStairs() const override { return false; }
	InsideLocationLevel& GetLevelData() override { return *this; }
	int GetRandomLevel() const override { return 0; }
	bool IsMultilevel() const override { return false; }
};
