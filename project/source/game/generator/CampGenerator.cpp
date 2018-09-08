#include "Pch.h"
#include "GameCore.h"
#include "CampGenerator.h"
#include "Terrain.h"
#include "OutsideLocation.h"
#include "Perlin.h"
#include "Game.h"

void CampGenerator::Generate()
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

int CampGenerator::HandleUpdate()
{
	return 0;
}

void CampGenerator::GenerateObjects()
{
	SpawnForestObjects();
	Game::Get().SpawnCampObjects();
}

void CampGenerator::GenerateUnits()
{
	Game::Get().SpawnCampUnits();
}

void CampGenerator::GenerateItems()
{
	SpawnForestItems(-1);
}
