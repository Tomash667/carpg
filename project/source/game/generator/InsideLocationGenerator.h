#pragma once

#include "LocationGenerator.h"

class InsideLocationGenerator : public LocationGenerator
{
public:
	void Init() override;
	void OnEnter() override;
	virtual bool HandleUpdate(int days) { return true; }
	void CreateMinimap() override;
	void OnLoad() override;

protected:
	InsideLocationLevel& GetLevelData();
	void GenerateTraps();
	void RegenerateTraps();
	void RespawnTraps();

	InsideLocation* inside;
};
