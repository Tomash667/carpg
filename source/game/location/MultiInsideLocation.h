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

	explicit MultiInsideLocation(int _levels) : active_level(-1), active(nullptr), generated(0)
	{
		levels.resize(_levels);
		LevelInfo li = {-1, false, false};
		infos.resize(_levels, li);
		for(vector<InsideLocationLevel>::iterator it = levels.begin(), end = levels.end(); it != end; ++it)
			it->map = nullptr;
	}

	// from ILevel
	virtual void ApplyContext(LevelContext& ctx) override;
	// from Location
	virtual void Save(HANDLE file, bool local) override;
	virtual void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	virtual void BuildRefidTable() override;
	virtual bool FindUnit(Unit* unit, int* level) override;
	virtual Unit* FindUnit(UnitData* unit, int& at_level) override;
	virtual Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) override;
	virtual Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr) override;
	inline virtual LOCATION_TOKEN GetToken() const override { return LT_MULTI_DUNGEON; }
	virtual bool CheckUpdate(int& days_passed, int worldtime) override;
	virtual int GetRandomLevel() const override;
	inline virtual int GetLastLevel() const override { return levels.size() - 1; }
	// from InsideLocation
	inline virtual void SetActiveLevel(int _level) override
	{
		assert(in_range(_level, 0, (int)levels.size()));

		active_level = _level;
		active = &levels[_level];
	}
	inline virtual bool HaveUpStairs() const override { return !(from_portal && active_level == 0); }
	inline virtual bool HaveDownStairs() const override { return (active_level+1 < (int)levels.size()); }
	inline virtual InsideLocationLevel& GetLevelData() override { return *active; }
	inline virtual bool IsMultilevel() const override { return true; }
	inline virtual InsideLocationLevel* GetLastLevelData() override
	{
		if(last_visit == -1 || generated == levels.size())
			return &levels.back();
		else
			return nullptr;
	}

	inline bool IsLevelClear() const
	{
		return infos[active_level].cleared;
	}
	inline bool LevelCleared()
	{
		infos[active_level].cleared = true;
		for(vector<LevelInfo>::iterator it = infos.begin(), end = infos.end(); it != end; ++it)
		{
			if(!it->cleared)
				return false;
		}
		return true;
	}
	inline void Reset()
	{
		for(vector<LevelInfo>::iterator it = infos.begin(), end = infos.end(); it != end; ++it)
		{
			it->reset = true;
			it->cleared = false;
		}
	}
};
