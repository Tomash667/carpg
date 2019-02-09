#include "Pch.h"
#include "GameCore.h"
#include "ForestGenerator.h"
#include "OutsideLocation.h"
#include "Terrain.h"
#include "Perlin.h"
#include "QuestManager.h"
#include "Quest_Sawmill.h"
#include "World.h"
#include "Level.h"
#include "Team.h"
#include "UnitGroup.h"
#include "Game.h"

//=================================================================================================
void ForestGenerator::Generate()
{
	CreateMap();
	RandomizeTerrainTexture();

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();
	Perlin perlin(4, 4, 1);

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
		{
			if(x < 15 || x > s - 15 || y < 15 || y > s - 15)
				h[x + y * (s + 1)] += Random(10.f, 15.f);
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
			h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 5;
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
int ForestGenerator::HandleUpdate()
{
	if(L.location_index != QM.quest_sawmill->target_loc)
		return 1;

	// sawmill quest
	if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
		&& QM.quest_sawmill->build_state == Quest_Sawmill::BuildState::LumberjackLeft)
	{
		QM.quest_sawmill->GenerateSawmill(true);
		have_sawmill = true;
		L.location->loaded_resources = false;
		return -1;
	}
	else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working
		&& QM.quest_sawmill->build_state != Quest_Sawmill::BuildState::Finished)
	{
		QM.quest_sawmill->GenerateSawmill(false);
		have_sawmill = true;
		L.location->loaded_resources = false;
		return -1;
	}
	else
		return 1;
}

//=================================================================================================
void ForestGenerator::GenerateObjects()
{
	SpawnForestObjects();
}

//=================================================================================================
void ForestGenerator::GenerateUnits()
{
	// zbierz grupy
	static UnitGroup* groups[4] = {
		UnitGroup::TryGet("wolfs"),
		UnitGroup::TryGet("spiders"),
		UnitGroup::TryGet("rats"),
		UnitGroup::TryGet("animals")
	};
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	const int level = L.GetDifficultyLevel();
	TmpUnitGroupList<4> tmp;
	static vector<Vec2> poss;
	tmp.Fill(groups, level);
	poss.clear();
	OutsideLocation* outside = (OutsideLocation*)L.location;
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
				L.SpawnUnitNearLocation(L.local_ctx, pos3, *ud_hunter, nullptr, enemy_level, 6.f);
			}
			for(TmpUnitGroup::Spawn& spawn : tmp.Roll(level, 2))
			{
				if(!L.SpawnUnitNearLocation(L.local_ctx, pos3, *spawn.first, nullptr, spawn.second, 6.f))
					break;
			}
		}
	}
}

//=================================================================================================
void ForestGenerator::GenerateItems()
{
	SpawnForestItems(have_sawmill ? -1 : 0);
}
