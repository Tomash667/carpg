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
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;

	Int2 GetRandomTile() const
	{
		return ext.Random();
	}
};
