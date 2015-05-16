#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"

//-----------------------------------------------------------------------------
struct Camp : public OutsideLocation
{
	int create_time;

	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);
	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_CAMP;
	}
};
