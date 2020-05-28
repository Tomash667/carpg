#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"

//-----------------------------------------------------------------------------
// Camp location
struct Camp : public OutsideLocation
{
	int create_time;

	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
};
