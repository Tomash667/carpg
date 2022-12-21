#include "Pch.h"
#include "HillsGenerator.h"

#include "Level.h"
#include "OutsideLocation.h"
#include "Team.h"
#include "UnitData.h"
#include "UnitGroup.h"
#include "World.h"

#include <Terrain.h>
#include <Perlin.h>

//=================================================================================================
void HillsGenerator::Generate()
{
	CreateMap();
	RandomizeTerrainTexture();
	terrain->SetHeightMap(outside->h);
	float hmax = 13.f;
	int octaves = Random(2, 8);
	float frequency = Random(4.f, 8.f);
	RandomizeHeight(octaves, frequency, 0.f, hmax);
	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
int HillsGenerator::HandleUpdate(int days)
{
	return 0;
}

//=================================================================================================
void HillsGenerator::GenerateObjects()
{
	SpawnForestObjects();
}

//=================================================================================================
void HillsGenerator::GenerateUnits()
{
	if(loc->group->IsEmpty())
		return;

	LocationPart& locPart = *gameLevel->localPart;
	UnitData* udHunter = UnitData::Get("wild_hunter");
	const int level = gameLevel->GetDifficultyLevel();
	TmpUnitGroupList tmp;
	tmp.Fill(loc->group, level);
	LocalVector3<Vec2> existingPositions;
	existingPositions.push_back(Vec2(teamPos.x, teamPos.z));

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();

		bool ok = true;
		for(const Vec2& existingPos : existingPositions)
		{
			if(Vec2::DistanceSquared(pos, existingPos) < Pow2(24.f))
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			existingPositions.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			if(Rand() % 5 == 0 && udHunter->level.x <= level)
			{
				int enemyLevel = Random(udHunter->level.x, min(udHunter->level.y, level));
				gameLevel->SpawnUnitNearLocation(locPart, pos3, *udHunter, nullptr, enemyLevel, 6.f);
			}
			for(TmpUnitGroup::Spawn& spawn : tmp.Roll(level, 2))
			{
				if(!gameLevel->SpawnUnitNearLocation(locPart, pos3, *spawn.first, nullptr, spawn.second, 6.f))
					break;
			}
		}
	}
}

//=================================================================================================
void HillsGenerator::GenerateItems()
{
	SpawnForestItems(0);
}
