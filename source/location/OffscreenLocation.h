#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LevelArea.h"

//-----------------------------------------------------------------------------
// Special location that is used to store units that are not inside any normal location
struct OffscreenLocation : public Location, public LevelArea
{
	OffscreenLocation();
	void Apply(vector<std::reference_wrapper<LevelArea>>& areas) override { assert(0); }
	void Save(GameWriter& f, bool local) override;
	void Load(GameReader& f, bool local) override;
	void Write(BitStreamWriter& f) override { assert(0); }
	bool Read(BitStreamReader& f) { assert(0); return false; }
	bool FindUnit(Unit* unit, int* level = nullptr) override { assert(0); return false; }
	Unit* FindUnit(UnitData* data, int& at_level) override { assert(0); return nullptr; }

	Unit* CreateUnit(UnitData& data, int level = -1);
};
