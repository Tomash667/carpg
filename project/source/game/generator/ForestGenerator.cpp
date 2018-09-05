#include "Pch.h"
#include "GameCore.h"
#include "ForestGenerator.h"
#include "OutsideLocation.h"
#include "Terrain.h"
#include "Perlin.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Level.h"
#include "Game.h"

void ForestGenerator::Init()
{
	if(loc->type == L_FOREST)
	{
		if(L.location_index == QM.quest_secret->where2)
			type = SECRET;
		else
			type = FOREST;
	}
	else if(loc->type == L_CAMP)
		type = CAMP;
	else
		type = MOONWELL;
}

int ForestGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 3; // txGeneratingObjects, txGeneratingUnits, txGeneratingItems
	else if(!reenter)
		steps += 2; // txGeneratingUnits, txGeneratingPhysics
	if(!reenter)
		++steps; // txRecreatingObjects
	return steps;
}

void ForestGenerator::Generate()
{
	OutsideLocation* outside = (OutsideLocation*)loc;
	Terrain* terrain = Game::Get().terrain;

	if(type == SECRET)
		QM.quest_secret->state = Quest_Secret::SECRET_GENERATED2;

	// create map
	const int s = OutsideLocation::size;
	outside->tiles = new TerrainTile[s*s];
	outside->h = new float[(s + 1)*(s + 1)];
	memset(outside->tiles, 0, sizeof(TerrainTile)*s*s);

	// set random grass texture
	Perlin perlin2(4, 4, 1);
	for(int i = 0, y = 0; y < s; ++y)
	{
		for(int x = 0; x < s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();
	Perlin perlin(4, 4, 1);

	if(type != SECRET)
	{
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

		// set green hill in middle
		if(type == MOONWELL)
		{
			for(int y = 40; y < s - 40; ++y)
			{
				for(int x = 40; x < s - 40; ++x)
				{
					float d;
					if((d = Distance(float(x), float(y), 64.f, 64.f)) < 8.f)
					{
						outside->tiles[x + y * s].t = TT_GRASS;
						h[x + y * (s + 1)] += (1.f - d / 8.f) * 5;
					}
				}
			}

			terrain->RoundHeight();
			terrain->RoundHeight();
		}

		terrain->RoundHeight();
	}
	else
	{
		// flatten terrain around portal & building
		terrain->RoundHeight();
		terrain->RoundHeight();

		for(uint y = 0; y < s; ++y)
		{
			for(uint x = 0; x < s; ++x)
				h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 5;
		}

		terrain->RoundHeight();

		float h1 = h[64 + 32 * (s + 1)],
			h2 = h[64 + 96 * (s + 2)];

		for(uint y = 0; y < s; ++y)
		{
			for(uint x = 0; x < s; ++x)
			{
				if(Distance(float(x), float(y), 64.f, 32.f) < 4.f)
					h[x + y * (s + 1)] = h1;
				else if(Distance(float(x), float(y), 64.f, 96.f) < 12.f)
					h[x + y * (s + 1)] = h2;
			}
		}

		terrain->RoundHeight();

		for(uint y = 0; y < s; ++y)
		{
			for(uint x = 0; x < s; ++x)
			{
				if(x < 15 || x > s - 15 || y < 15 || y > s - 15)
					h[x + y * (s + 1)] += Random(10.f, 15.f);
			}
		}

		terrain->RoundHeight();

		for(uint y = 0; y < s; ++y)
		{
			for(uint x = 0; x < s; ++x)
			{
				if(Distance(float(x), float(y), 64.f, 32.f) < 4.f)
					h[x + y * (s + 1)] = h1;
				else if(Distance(float(x), float(y), 64.f, 96.f) < 12.f)
					h[x + y * (s + 1)] = h2;
			}
		}
	}

	terrain->RemoveHeightMap();
}

void ForestGenerator::CreateTiles()
{

}
