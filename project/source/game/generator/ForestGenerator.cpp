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
	static TmpUnitGroup groups[4] = {
		{ UnitGroup::TryGet("wolfs") },
		{ UnitGroup::TryGet("spiders") },
		{ UnitGroup::TryGet("rats") },
		{ UnitGroup::TryGet("animals") }
	};
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	const int level = L.GetDifficultyLevel();
	static vector<Vec2> poss;
	poss.clear();
	OutsideLocation* outside = (OutsideLocation*)L.location;
	poss.push_back(Vec2(team_pos.x, team_pos.z));

	// ustal wrogów
	for(int i = 0; i < 4; ++i)
		groups[i].Fill(level);

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
			// losuj grupe
			TmpUnitGroup& group = groups[Rand() % 4];
			if(group.entries.empty())
				continue;

			poss.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			int levels = level * 2;
			if(Rand() % 5 == 0 && ud_hunter->level.x <= level)
			{
				int enemy_level = Random(ud_hunter->level.x, Min(ud_hunter->level.y, levels, level));
				if(L.SpawnUnitNearLocation(L.local_ctx, pos3, *ud_hunter, nullptr, enemy_level, 6.f))
					levels -= enemy_level;
			}
			while(levels > 0)
			{
				int k = Rand() % group.total, l = 0;
				UnitData* ud = nullptr;

				for(auto& entry : group.entries)
				{
					l += entry.count;
					if(k < l)
					{
						ud = entry.ud;
						break;
					}
				}

				assert(ud);

				if(!ud || ud->level.x > levels)
					break;

				int enemy_level = Random(ud->level.x, Min(ud->level.y, levels, level));
				if(!L.SpawnUnitNearLocation(L.local_ctx, pos3, *ud, nullptr, enemy_level, 6.f))
					break;
				levels -= enemy_level;
			}
		}
	}
}

//=================================================================================================
void ForestGenerator::GenerateItems()
{
	SpawnForestItems(have_sawmill ? -1 : 0);
}
