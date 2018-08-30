#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"

//-----------------------------------------------------------------------------
struct Camp : public OutsideLocation
{
	int create_time;

	void Save(GameWriter& f, bool local) override;
	void Load(GameReader& f, bool local, LOCATION_TOKEN token) override;
	LOCATION_TOKEN GetToken() const override
	{
		return LT_CAMP;
	}
};
