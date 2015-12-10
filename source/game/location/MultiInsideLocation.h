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

	inline void SetActiveLevel(int _level)
	{
		assert(in_range(_level, 0, (int)levels.size()));

		active_level = _level;
		active = &levels[_level];
	}
	inline bool HaveUpStairs() const
	{
		return !(from_portal && active_level == 0);
	}
	inline bool HaveDownStairs() const
	{
		return (active_level+1 < (int)levels.size());
	}
	inline InsideLocationLevel& GetLevelData()
	{
		return *active;
	}
	inline bool IsMultilevel() const
	{
		return true;
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
	virtual bool CheckUpdate(int& days_passed, int worldtime)
	{
		bool need_reset = infos[active_level].reset;
		infos[active_level].reset = false;
		if(infos[active_level].last_visit == -1)
			days_passed = -1;
		else
			days_passed = worldtime - infos[active_level].last_visit;
		infos[active_level].last_visit = worldtime;
		return need_reset;
	}
	int GetRandomLevel() const;
	inline int GetLastLevel() const
	{
		return levels.size()-1;
	}
	InsideLocationLevel* GetLastLevelData()
	{
		if(last_visit == -1 || generated == levels.size())
			return &levels.back();
		else
			return nullptr;
	}

	void ApplyContext(LevelContext& ctx);
	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);
	virtual void BuildRefidTable();
	virtual bool FindUnit(Unit* unit, int* level);
	virtual Unit* FindUnit(UnitData* unit, int& at_level);
	virtual Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr);
	virtual Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr);
	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_MULTI_DUNGEON;
	}
};
