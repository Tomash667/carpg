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
		cel = CAVE;
	}

	inline INT2 GetRandomTile() const
	{
		return ext.Random();
	}

	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);

	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_CAVE;
	}
};
