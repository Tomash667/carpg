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
	int octaves = Random(2, 8);
	float frequency = Random(4.f, 16.f);
	RandomizeHeight(octaves, frequency, 0.f, hmax);
	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
int ForestGenerator::HandleUpdate(int days)
{
	if(game_level->location_index != quest_mgr->quest_sawmill->target_loc)
		return 0;

	// sawmill quest
	if(quest_mgr->quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
		&& quest_mgr->quest_sawmill->build_state == Quest_Sawmill::BuildState::LumberjackLeft)
	{
		quest_mgr->quest_sawmill->GenerateSawmill(true);
		have_sawmill = true;
		game_level->location->loaded_resources = false;
		return PREVENT_RESET | PREVENT_RESPAWN_UNITS;
	}
	else if(quest_mgr->quest_sawmill->sawmill_state == Quest_Sawmill::State::Working
		&& quest_mgr->quest_sawmill->build_state != Quest_Sawmill::BuildState::Finished)
	{
		quest_mgr->quest_sawmill->GenerateSawmill(false);
		have_sawmill = true;
		game_level->location->loaded_resources = false;
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

	LevelArea& area = *game_level->local_area;
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
				game_level->SpawnUnitNearLocation(area, pos3, *ud_hunter, nullptr, enemy_level, 6.f);
			}
			for(TmpUnitGroup::Spawn& spawn : tmp.Roll(level, 2))
			{
				if(!game_level->SpawnUnitNearLocation(area, pos3, *spawn.first, nullptr, spawn.second, 6.f))
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
