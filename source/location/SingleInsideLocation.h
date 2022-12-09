#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
struct SingleInsideLocation : public InsideLocation, public InsideLocationLevel
{
	SingleInsideLocation() : InsideLocationLevel(0) {}
	// from Location
	void Apply(vector<std::reference_wrapper<LocationPart>>& parts) override;
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	// from InsideLocation
	Chest* FindChestWithItem(const Item* item, int& atLevel, int* index = nullptr) override;
	Chest* FindChestWithQuestItem(int questId, int& atLevel, int* index = nullptr) override;
	InsideLocationLevel* GetLastLevelData() override { return (lastVisit != -1 ? this : nullptr); }
	void SetActiveLevel(int level) override { assert(level == 0); }
	bool HavePrevEntry() const override { return !fromPortal; }
	bool HaveNextEntry() const override { return false; }
	InsideLocationLevel& GetLevelData() override { return *this; }
	int GetRandomLevel() const override { return 0; }
	bool IsMultilevel() const override { return false; }
};
