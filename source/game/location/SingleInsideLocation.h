// podziemia z jednym poziomem
#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
struct SingleInsideLocation : public InsideLocation, public InsideLocationLevel
{
	// from ILevel
	virtual void ApplyContext(LevelContext& ctx) override;
	// from Location
	virtual void Save(HANDLE file, bool local) override;
	virtual void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	virtual void BuildRefidTable() override;
	virtual bool FindUnit(Unit* unit, int* level) override;
	virtual Unit* FindUnit(UnitData* unit, int& at_level) override;
	virtual LOCATION_TOKEN GetToken() const override { return LT_SINGLE_DUNGEON; }
	// from InsideLocation
	virtual Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) override;
	virtual Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr) override;	
	inline virtual InsideLocationLevel* GetLastLevelData() override { return (last_visit != -1 ? this : nullptr); }
	inline virtual void SetActiveLevel(int level) override { assert(level == 0); }
	inline virtual bool HaveUpStairs() const override { return !from_portal; }
	inline virtual bool HaveDownStairs() const override { return false; }
	inline virtual InsideLocationLevel& GetLevelData() override { return *this; }
	inline virtual int GetRandomLevel() const override { return 0; }
	inline virtual bool IsMultilevel() const override { return false; }
};
