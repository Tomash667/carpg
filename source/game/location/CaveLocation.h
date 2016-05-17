// jaskinia
#pragma once

//-----------------------------------------------------------------------------
#include "SingleInsideLocation.h"
#include "BaseLocation.h"

//-----------------------------------------------------------------------------
struct CaveLocation : public SingleInsideLocation
{
	vector<INT2> holes;
	IBOX2D ext;

	CaveLocation()
	{
		target = CAVE;
	}

	// from Location
	virtual void Save(HANDLE file, bool local) override;
	virtual void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	inline virtual LOCATION_TOKEN GetToken() const override { return LT_CAVE; }

	inline INT2 GetRandomTile() const
	{
		return ext.Random();
	}
};
