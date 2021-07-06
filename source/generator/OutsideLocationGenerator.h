#pragma once

//-----------------------------------------------------------------------------
#include "LocationGenerator.h"

//-----------------------------------------------------------------------------
class OutsideLocationGenerator : public LocationGenerator
{
public:
	static void InitOnce();
	void Init() override;
	int GetNumberOfSteps() override;
	void OnEnter() override;
	void SpawnForestObjects(int road_dir = -1); //-1 none, 0 horizontal, 1 vertical
	void SpawnForestItems(int countMod);
	int HandleUpdate(int days);
	virtual void SpawnTeam();
	void CreateMinimap() override;
	void OnLoad() override;

protected:
	void SetOutsideParams();
	void CreateMap();
	virtual void RandomizeTerrainTexture();
	void RandomizeHeight(int octaves, float frequency, float hmin, float hmax);
	void ApplyTiles();
	void SpawnOutsideBariers();
	void SpawnCityPhysics();

	static const uint s;
	static OutsideObject trees[];
	static const uint n_trees;
	static OutsideObject trees2[];
	static const uint n_trees2;
	static OutsideObject misc[];
	static const uint n_misc;
	OutsideLocation* outside;
	Terrain* terrain;
	Vec3 team_pos;
	float team_dir;
};
