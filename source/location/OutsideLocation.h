#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LocationPart.h"
#include "TerrainTile.h"

//-----------------------------------------------------------------------------
enum OutsideLocationTarget
{
	FOREST,
	MOONWELL,
	ACADEMY,
	HUNTERS_CAMP,
	HILLS
};

//-----------------------------------------------------------------------------
struct OutsideLocation : public Location, public LocationPart
{
	TerrainTile* tiles;
	float* h;
	static const int size = 16 * 8;

	OutsideLocation();
	~OutsideLocation();

	// from Location
	void Apply(vector<std::reference_wrapper<LocationPart>>& parts) override;
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	void Write(BitStreamWriter& f) override;
	bool Read(BitStreamReader& f) override;

	bool IsInside(int x, int y) const
	{
		return x >= 0 && y >= 0 && x < size && y < size;
	}
	bool IsInside(const Int2& pt) const
	{
		return IsInside(pt.x, pt.y);
	}
	Vec2 GetRandomPos() const
	{
		return Vec2(Random(40.f, 256.f - 40.f), Random(40.f, 256.f - 40.f));
	}
};
