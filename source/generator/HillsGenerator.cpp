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

	LocationPart& locPart = *game_level->localPart;
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	const int level = game_level->GetDifficultyLevel();
	TmpUnitGroupList tmp;
	tmp.Fill(loc->group, level);
	static vector<Vec2> poss;
	poss.clear();
	poss.push_back(Vec2(team_pos.x, team_pos.z));

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 24.f)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			poss.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			if(Rand() % 5 == 0 && ud_hunter->level.x <= level)
			{
				int enemy_level = Random(ud_hunter->level.x, min(ud_hunter->level.y, level));
				game_level->SpawnUnitNearLocation(locPart, pos3, *ud_hunter, nullptr, enemy_level, 6.f);
			}
			for(TmpUnitGroup::Spawn& spawn : tmp.Roll(level, 2))
			{
				if(!game_level->SpawnUnitNearLocation(locPart, pos3, *spawn.first, nullptr, spawn.second, 6.f))
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
