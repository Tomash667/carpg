#include "Pch.h"
#include "ForestGenerator.h"

#include "Level.h"
#include "OutsideLocation.h"
#include "QuestManager.h"
#include "Quest_Sawmill.h"
#include "Team.h"
#include "UnitGroup.h"
#include "World.h"

#include <Terrain.h>
#include <Perlin.h>

//=================================================================================================
void ForestGenerator::Generate()
{
	CreateMap();
	RandomizeTerrainTexture();
	terrain->SetHeightMap(outside->h);
	float hmax = Random(8.f, 12.f);
	int octaves = Random(4, 8);
	float frequency = Random(3.f, 6.f);
	RandomizeHeight(octaves, frequency, 0.f, hmax);
	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
void ForestGenerator::RandomizeTerrainTexture()
{
	Perlin perlin2(4, 4);
	for(uint i = 0, y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x, ++i)
		{
			const float v = perlin2.GetNormalized(1.f / 256 * x, 1.f / 256 * y);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 0.25f)
				t = TT_GRASS2;
			else if(v < 0.7f)
				t = TT_GRASS;
			else
				t = TT_GRASS3;
		}
	}
}

//=================================================================================================
int ForestGenerator::HandleUpdate(int days)
{
	if(gameLevel->location != questMgr->questSawmill->targetLoc)
		return 0;

	// sawmill quest
	if(questMgr->questSawmill->sawmillState == Quest_Sawmill::State::InBuild
		&& questMgr->questSawmill->buildState == Quest_Sawmill::BuildState::LumberjackLeft)
	{
		questMgr->questSawmill->GenerateSawmill(true);
		haveSawmill = true;
		gameLevel->location->loadedResources = false;
		return PREVENT_RESET | PREVENT_RESPAWN_UNITS;
	}
	else if(questMgr->questSawmill->sawmillState == Quest_Sawmill::State::Working
		&& questMgr->questSawmill->buildState != Quest_Sawmill::BuildState::Finished)
	{
		questMgr->questSawmill->GenerateSawmill(false);
		haveSawmill = true;
		gameLevel->location->loadedResources = false;
		return PREVENT_RESET | PREVENT_RESPAWN_UNITS;
	}
	else
		return 0;
}

//=================================================================================================
void ForestGenerator::GenerateObjects()
{
	SpawnForestObjects();
}

//=================================================================================================
void ForestGenerator::GenerateUnits()
{
	if(loc->group->IsEmpty())
		return;

	LocationPart& locPart = *gameLevel->localPart;
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	const int level = gameLevel->GetDifficultyLevel();
	TmpUnitGroupList tmp;
	tmp.Fill(loc->group, level);
	static vector<Vec2> poss;
	poss.clear();
	poss.push_back(Vec2(teamPos.x, teamPos.z));

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
				gameLevel->SpawnUnitNearLocation(locPart, pos3, *ud_hunter, nullptr, enemy_level, 6.f);
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
void ForestGenerator::GenerateItems()
{
	SpawnForestItems(haveSawmill ? -1 : 0);
}
