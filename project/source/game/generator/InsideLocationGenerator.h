#pragma once

#include "LocationGenerator.h"

class InsideLocationGenerator : public LocationGenerator
{
public:
	void Init() override;
	void OnEnter() override;
	virtual bool HandleUpdate(int days) { return true; }

protected:
	InsideLocationLevel& GetLevelData();
	void GenerateTraps();
	void RegenerateTraps();

	InsideLocation* inside;
};
