#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"

//-----------------------------------------------------------------------------
// Camp location
struct Camp : public OutsideLocation
{
	int createTime;

	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
};
