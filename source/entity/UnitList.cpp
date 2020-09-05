#include "Pch.h"
#include "UnitList.h"

bool UnitList::IsInside(Unit* unit) const
{
	return ::IsInside(units, unit);
}

void UnitList::Save(GameWriter& f)
{
	f << units.size();
	for(Entity<Unit> unit : units)
		f << unit;
}

void UnitList::Load(GameReader& f)
{
	uint count;
	f >> count;
	units.resize(count);
	for(Entity<Unit>& unit : units)
		f >> unit;
}
