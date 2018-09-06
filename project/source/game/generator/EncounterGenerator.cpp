#include "Pch.h"
#include "GameCore.h"
#include "EncounterGenerator.h"
#include "OutsideLocation.h"
#include "Perlin.h"
#include "Terrain.h"
#include "Team.h"
#include "Game.h"

int EncounterGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	steps += 4; // txGeneratingObjects, txRecreatingObjects, txGeneratingUnits, txGeneratingItems
	return steps;
}

void EncounterGenerator::Generate()
{
	Game& game = Game::Get();
	OutsideLocation* outside = (OutsideLocation*)&loc;
	Terrain* terrain = game.terrain;

	// 0 - right, 1 - up, 2 - left, 3 - down
	game.enc_kierunek = Rand() % 4;

	// create map
	const int s = OutsideLocation::size;
	if(!outside->tiles)
	{
		outside->tiles = new TerrainTile[s*s];
		outside->h = new float[(s + 1)*(s + 1)];
		memset(outside->tiles, 0, sizeof(TerrainTile)*s*s);
	}

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

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
		{
			if(x < 15 || x > s - 15 || y < 15 || y > s - 15)
				h[x + y * (s + 1)] += Random(10.f, 15.f);
		}
	}

	// create road
	if(game.enc_kierunek == 0 || game.enc_kierunek == 2)
	{
		for(int y = 62; y < 66; ++y)
		{
			for(int x = 0; x < s; ++x)
			{
				outside->tiles[x + y * s].Set(TT_SAND, TM_ROAD);
				h[x + y * (s + 1)] = 1.f;
			}
		}
		for(int x = 0; x < s; ++x)
		{
			h[x + 61 * (s + 1)] = 1.f;
			h[x + 66 * (s + 1)] = 1.f;
			h[x + 67 * (s + 1)] = 1.f;
		}
	}
	else
	{
		for(int y = 0; y < s; ++y)
		{
			for(int x = 62; x < 66; ++x)
			{
				outside->tiles[x + y * s].Set(TT_SAND, TM_ROAD);
				h[x + y * (s + 1)] = 1.f;
			}
		}
		for(int y = 0; y < s; ++y)
		{
			h[61 + y * (s + 1)] = 1.f;
			h[66 + y * (s + 1)] = 1.f;
			h[67 + y * (s + 1)] = 1.f;
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	Perlin perlin(4, 4, 1);
	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
			h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 4;
	}

	// flatten road
	if(game.enc_kierunek == 0 || game.enc_kierunek == 2)
	{
		for(int y = 61; y <= 67; ++y)
		{
			for(int x = 1; x < s - 1; ++x)
				h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1)*(s + 1)] + h[x + (y + 1)*(s + 1)]) / 5;
		}
	}
	else
	{
		for(int y = 1; y < s - 1; ++y)
		{
			for(int x = 61; x <= 67; ++x)
				h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1)*(s + 1)] + h[x + (y + 1)*(s + 1)]) / 5;
		}
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

void EncounterGenerator::OnEnter()
{
	Game& game = Game::Get();
	OutsideLocation* enc = (OutsideLocation*)loc;
	enc->loaded_resources = false;
	game.city_ctx = nullptr;
	game.ApplyContext(enc, game.local_ctx);

	game.ApplyTiles(enc->h, enc->tiles);
	game.SetOutsideParams();

	// generate objects
	game.LoadingStep(game.txGeneratingObjects);
	game.SpawnEncounterObjects();

	// create colliders
	game.LoadingStep(game.txRecreatingObjects);
	game.SpawnTerrainCollider();
	game.SpawnOutsideBariers();

	// generate units
	game.LoadingStep(game.txGeneratingUnits);
	GameDialog* dialog;
	Unit* talker;
	Quest* quest;
	game.SpawnEncounterUnits(dialog, talker, quest);

	// generate items
	game.LoadingStep(game.txGeneratingItems);
	game.SpawnForestItems(-1);

	// generate minimap
	game.LoadingStep(game.txGeneratingMinimap);
	game.CreateForestMinimap();

	// add team
	game.SpawnEncounterTeam();
	if(dialog)
	{
		DialogContext& ctx = *Team.leader->player->dialog_ctx;
		game.StartDialog2(Team.leader->player, talker, dialog);
		ctx.dialog_quest = quest;
	}
}
