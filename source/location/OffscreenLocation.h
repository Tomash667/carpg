#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LocationPart.h"

//-----------------------------------------------------------------------------
// Special location that is used to store units that are not inside any normal location
struct OffscreenLocation : public Location, public LocationPart
{
	OffscreenLocation();
	void Apply(vector<std::reference_wrapper<LocationPart>>& parts) override { assert(0); }
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	void Write(BitStreamWriter& f) override { assert(0); }
	bool Read(BitStreamReader& f) { assert(0); return false; }

	Unit* CreateUnit(UnitData& data, int level = -1);
};
