// podziemia z jednym poziomem
#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
struct SingleInsideLocation : public InsideLocation, public InsideLocationLevel
{
	inline void SetActiveLevel(int _level)
	{
		assert(_level == 0);
	}
	inline bool HaveUpStairs() const
	{
		return !from_portal;
	}
	inline bool HaveDownStairs() const
	{
		return false;
	}
	inline InsideLocationLevel& GetLevelData()
	{
		return *this;
	}
	bool IsMultilevel() const
	{
		return false;
	}

	void ApplyContext(LevelContext& ctx);
	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);
	virtual void BuildRefidTable();
	virtual void RemoveUnit(Unit* unit, int level);
	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_SINGLE_DUNGEON;
	}
	InsideLocationLevel* GetLastLevelData()
	{
		if(last_visit != -1)
			return this;
		else
			return NULL;
	}
};
