#pragma once

#include "LocationGenerator.h"

class InsideLocationGenerator : public LocationGenerator
{
public:
	void OnEnter() override;
	virtual bool HandleUpdate(int days) { return true; }

protected:
	InsideLocationLevel& GetLevelData();
};
