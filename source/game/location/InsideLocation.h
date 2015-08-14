// podziemia
#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "InsideLocationLevel.h"

//-----------------------------------------------------------------------------
struct InsideLocation : public Location
{
	int cel, specjalny_pokoj;
	bool from_portal;

	InsideLocation() : Location(false), specjalny_pokoj(-1), from_portal(false)
	{

	}

	virtual void SetActiveLevel(int _level) = 0;
	virtual bool HaveUpStairs() const = 0;
	virtual bool HaveDownStairs() const = 0;
	virtual InsideLocationLevel& GetLevelData() = 0;
	virtual bool IsMultilevel() const = 0;
	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);
	// return last level data if it was generated
	virtual InsideLocationLevel* GetLastLevelData() = 0;

	inline Room* FindChaseRoom(const VEC3& pos)
	{
		InsideLocationLevel& lvl = GetLevelData();
		if(lvl.rooms.size() < 2)
			return NULL;
		else
			return lvl.GetNearestRoom(pos);
	}
};
