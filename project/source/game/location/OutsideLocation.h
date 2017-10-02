// zewnêtrzna lokacja
#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LevelArea.h"
#include "TerrainTile.h"
#include "Blood.h"
#include "Object.h"

//-----------------------------------------------------------------------------
struct OutsideLocation : public Location, public LevelArea
{
	vector<Object*> objects;
	vector<Chest*> chests;
	vector<Usable*> usables;
	vector<Blood> bloods;
	TerrainTile* tiles;
	float* h;
	static const int size = 16 * 8;

	OutsideLocation() : Location(true), tiles(nullptr), h(nullptr)
	{
	}
	virtual ~OutsideLocation();

	// from ILevel
	void ApplyContext(LevelContext& ctx) override;
	// from Location
	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	void BuildRefidTable() override;
	bool FindUnit(Unit* unit, int* level) override;
	Unit* FindUnit(UnitData* data, int& at_level) override;
	LOCATION_TOKEN GetToken() const override { return LT_OUTSIDE; }

	bool IsInside(int _x, int _y) const
	{
		return _x >= 0 && _y >= 0 && _x < size && _y < size;
	}
	bool IsInside(const Int2& _pt) const
	{
		return IsInside(_pt.x, _pt.y);
	}
	Vec2 GetRandomPos() const
	{
		return Vec2(Random(40.f, 256.f - 40.f), Random(40.f, 256.f - 40.f));
	}
};
