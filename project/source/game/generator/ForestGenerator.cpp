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
#include "Game.h"

void ForestGenerator::Generate()
{
	CreateMap();

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

int ForestGenerator::HandleUpdate()
{
	if(L.location_index != QM.quest_sawmill->target_loc)
		return 1;

	// sawmill quest
	if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
		&& QM.quest_sawmill->build_state == Quest_Sawmill::BuildState::LumberjackLeft)
	{
		Game::Get().GenerateSawmill(true);
		have_sawmill = true;
		L.location->loaded_resources = false;
		return -1;
	}
	else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working
		&& QM.quest_sawmill->build_state != Quest_Sawmill::BuildState::Finished)
	{
		Game::Get().GenerateSawmill(false);
		have_sawmill = true;
		L.location->loaded_resources = false;
		return -1;
	}
	else
		return 1;
}

void ForestGenerator::GenerateObjects()
{
	SpawnForestObjects();
}

void ForestGenerator::GenerateUnits()
{
	Game::Get().SpawnForestUnits(team_pos);
}

void ForestGenerator::GenerateItems()
{
	SpawnForestItems(have_sawmill ? -1 : 0);
}
