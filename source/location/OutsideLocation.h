#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LevelArea.h"
#include "TerrainTile.h"

//-----------------------------------------------------------------------------
struct OutsideLocation : public Location, public LevelArea
{
	TerrainTile* tiles;
	float* h;
	static const int size = 16 * 8;

	OutsideLocation();
	~OutsideLocation();

	// from Location
	void Apply(vector<std::reference_wrapper<LevelArea>>& areas) override;
	void Save(GameWriter& f, bool local) override;
	void Load(GameReader& f, bool local, LOCATION_TOKEN token) override;
	void Write(BitStreamWriter& f) override;
	bool Read(BitStreamReader& f) override;
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
