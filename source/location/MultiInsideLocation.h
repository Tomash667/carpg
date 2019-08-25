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
		bool cleared, reset, loaded_resources;
	};
	vector<InsideLocationLevel*> levels;
	vector<LevelInfo> infos;
	int active_level, generated;
	InsideLocationLevel* active;

	explicit MultiInsideLocation(int level_count);
	~MultiInsideLocation();

	// from Location
	void Apply(vector<std::reference_wrapper<LevelArea>>& areas) override;
	void Save(GameWriter& f, bool local) override;
	void Load(GameReader& f, bool local, LOCATION_TOKEN token) override;
	bool FindUnit(Unit* unit, int* level) override;
	Unit* FindUnit(UnitData* unit, int& at_level) override;
	Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) override;
	Chest* FindChestWithQuestItem(int quest_id, int& at_level, int* index = nullptr) override;
	LOCATION_TOKEN GetToken() const override { return LT_MULTI_DUNGEON; }
	bool CheckUpdate(int& days_passed, int worldtime) override;
	int GetRandomLevel() const override;
	int GetLastLevel() const override { return levels.size() - 1; }
	bool RequireLoadingResources(bool* to_set) override;
	// from InsideLocation
	void SetActiveLevel(int level) override
	{
		assert(InRange(level, 0, (int)levels.size()));
		active_level = level;
		active = levels[level];
	}
	bool HaveUpStairs() const override { return !(from_portal && active_level == 0); }
	bool HaveDownStairs() const override { return (active_level + 1 < (int)levels.size()); }
	InsideLocationLevel& GetLevelData() override { return *active; }
	bool IsMultilevel() const override { return true; }
	InsideLocationLevel* GetLastLevelData() override
	{
		if(last_visit == -1 || generated == levels.size())
			return levels.back();
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
