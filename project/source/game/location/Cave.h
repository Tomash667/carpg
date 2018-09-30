#pragma once

//-----------------------------------------------------------------------------
#include "SingleInsideLocation.h"
#include "BaseLocation.h"

//-----------------------------------------------------------------------------
// Cave location
struct Cave : public SingleInsideLocation
{
	vector<Int2> holes;
	Rect ext;

	Cave()
	{
		target = CAVE;
	}

	// from Location
	void Save(GameWriter& f, bool local) override;
	void Load(GameReader& f, bool local, LOCATION_TOKEN token) override;
	LOCATION_TOKEN GetToken() const override { return LT_CAVE; }

	Int2 GetRandomTile() const
	{
		return ext.Random();
	}
};
