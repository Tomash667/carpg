#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"

//-----------------------------------------------------------------------------
struct Camp : public OutsideLocation
{
	int create_time;

	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	LOCATION_TOKEN GetToken() const override
	{
		return LT_CAMP;
	}
};
