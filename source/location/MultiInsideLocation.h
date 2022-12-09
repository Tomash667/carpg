#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
struct MultiInsideLocation : public InsideLocation
{
	struct LevelInfo
	{
		int lastVisit;
		uint seed;
		bool cleared, reset, loadedResources;
	};
	vector<InsideLocationLevel*> levels;
	vector<LevelInfo> infos;
	int activeLevel, generated;
	InsideLocationLevel* active;

	explicit MultiInsideLocation(int levelCount);
	~MultiInsideLocation();

	// from Location
	void Apply(vector<std::reference_wrapper<LocationPart>>& parts) override;
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	Chest* FindChestWithItem(const Item* item, int& atLevel, int* index = nullptr) override;
	Chest* FindChestWithQuestItem(int questId, int& atLevel, int* index = nullptr) override;
	bool CheckUpdate(int& daysPassed, int worldtime) override;
	int GetRandomLevel() const override;
	int GetLastLevel() const override { return levels.size() - 1; }
	bool RequireLoadingResources(bool* toSet) override;
	// from InsideLocation
	void SetActiveLevel(int level) override
	{
		assert(InRange(level, 0, (int)levels.size()));
		activeLevel = level;
		active = levels[level];
	}
	bool HavePrevEntry() const override { return !(fromPortal && activeLevel == 0); }
	bool HaveNextEntry() const override { return (activeLevel + 1 < (int)levels.size()); }
	InsideLocationLevel& GetLevelData() override { return *active; }
	bool IsMultilevel() const override { return true; }
	InsideLocationLevel* GetLastLevelData() override
	{
		if(lastVisit == -1 || generated == levels.size())
			return levels.back();
		else
			return nullptr;
	}

	bool IsLevelClear() const
	{
		return infos[activeLevel].cleared;
	}
	bool LevelCleared();
	void Reset();
};
