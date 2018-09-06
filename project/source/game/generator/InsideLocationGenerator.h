#pragma once

#include "LocationGenerator.h"

class InsideLocationGenerator : public LocationGenerator
{
public:
	void OnEnter() override;
	virtual void GenerateObjects() {}
	virtual void GenerateUnits() {}
	virtual void GenerateItems() {}
	virtual bool HandleUpdate(int days) { return true; }

protected:
	InsideLocationLevel& GetLevelData();
};
