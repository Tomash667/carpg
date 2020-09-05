#pragma once

//-----------------------------------------------------------------------------
#include "Unit.h"

//-----------------------------------------------------------------------------
struct UnitList : public ObjectPoolProxy<UnitList>
{
	void Clear() { units.clear(); }
	void Add(Unit* unit) { units.push_back(unit); }
	bool IsInside(Unit* unit) const;
	void Save(GameWriter& f);
	void Load(GameReader& f);

private:
	vector<Entity<Unit>> units;
};
