#pragma once

#include "Location.h"

// new inside location using more 3d dungeon
class InsideLocation2 : public Location
{
public:
	inline InsideLocation2() : Location(GT_INSIDE2)
	{

	}

	void ApplyContext(LevelContext& ctx);
	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	void BuildRefidTable();
	bool FindUnit(Unit* unit, int* level = nullptr);
	Unit* FindUnit(UnitData* data, int& at_level);

	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_INSIDE_LOCATION2;
	}
};
