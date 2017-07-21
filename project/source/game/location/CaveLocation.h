// jaskinia
#pragma once

//-----------------------------------------------------------------------------
#include "SingleInsideLocation.h"
#include "BaseLocation.h"

//-----------------------------------------------------------------------------
struct CaveLocation : public SingleInsideLocation
{
	vector<Int2> holes;
	Rect ext;

	CaveLocation()
	{
		target = CAVE;
	}

	// from Location
	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	LOCATION_TOKEN GetToken() const override { return LT_CAVE; }

	Int2 GetRandomTile() const
	{
		return ext.Random();
	}
};
