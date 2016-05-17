#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"

//-----------------------------------------------------------------------------
struct Camp : public OutsideLocation
{
	int create_time;

	virtual void Save(HANDLE file, bool local) override;
	virtual void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	virtual LOCATION_TOKEN GetToken() const override
	{
		return LT_CAMP;
	}
};
