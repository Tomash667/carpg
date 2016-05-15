#pragma once

#include "ContentLoader.h"

class BuildingLoader : public ContentLoader
{
public:
	BuildingLoader();
	~BuildingLoader();
	
	void Init() override;
	int Load() override;
	void Cleanup() override;

	bool LoadBuilding();
	bool LoadBuildingGroups();
};
