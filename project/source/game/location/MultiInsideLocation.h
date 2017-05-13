// podziemia z wieloma poziomami
#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
struct MultiInsideLocation : public InsideLocation
{
	struct LevelInfo
	{
		int last_visit;
		uint seed;
		bool cleared, reset;
	};
	vector<InsideLocationLevel> levels;
	vector<LevelInfo> infos;
	int active_level, generated;
	InsideLocationLevel* active;

	explicit MultiInsideLocation(int _levels);

	// from ILevel
	void ApplyContext(LevelContext& ctx) override;
	// from Location
	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	void BuildRefidTable() override;
	bool FindUnit(Unit* unit, int* level) override;
	Unit* FindUnit(UnitData* unit, int& at_level) override;
	Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) override;
	Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr) override;
	LOCATION_TOKEN GetToken() const override { return LT_MULTI_DUNGEON; }
	bool CheckUpdate(int& days_passed, int worldtime) override;
	int GetRandomLevel() const override;
	int GetLastLevel() const override { return levels.size() - 1; }
	// from InsideLocation
	void SetActiveLevel(int _level) override
	{
		assert(in_range(_level, 0, (int)levels.size()));
		active_level = _level;
		active = &levels[_level];
	}
	bool HaveUpStairs() const override { return !(from_portal && active_level == 0); }
	bool HaveDownStairs() const override { return (active_level+1 < (int)levels.size()); }
	InsideLocationLevel& GetLevelData() override { return *active; }
	bool IsMultilevel() const override { return true; }
	InsideLocationLevel* GetLastLevelData() override
	{
		if(last_visit == -1 || generated == levels.size())
			return &levels.back();
		else
			return nullptr;
	}

	bool IsLevelClear() const
	{
		return infos[active_level].cleared;
	}
	bool LevelCleared();
	void Reset();
};
