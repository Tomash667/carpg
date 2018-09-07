#include "Pch.h"
#include "GameCore.h"
#include "ForestGenerator.h"
#include "OutsideLocation.h"
#include "Terrain.h"
#include "Perlin.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Quest_Bandits.h"
#include "Quest_Sawmill.h"
#include "World.h"
#include "Level.h"
#include "Team.h"
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

void ForestGenerator::OnEnter()
{
	Game& game = Game::Get();
	OutsideLocation* outside = (OutsideLocation*)L.location;
	game.city_ctx = nullptr;
	if(!reenter)
		game.ApplyContext(outside, game.local_ctx);

	int days;
	bool need_reset = outside->CheckUpdate(days, W.GetWorldtime());
	if(type == CAMP)
		need_reset = false;

	if(!reenter)
		game.ApplyTiles(outside->h, outside->tiles);

	game.SetOutsideParams();

	Vec3 pos;
	float dir;
	W.GetOutsideSpawnPoint(pos, dir);

	if(first)
	{
		// generate objects
		game.LoadingStep(game.txGeneratingObjects);
		switch(type)
		{
		case FOREST:
			SpawnForestObjects();
			break;
		case CAMP:
			SpawnForestObjects();
			game.SpawnCampObjects();
			break;
		case MOONWELL:
			game.SpawnMoonwellObjects();
			break;
		case SECRET:
			game.SpawnSecretLocationObjects();
			break;
		}

		// generate units
		game.LoadingStep(game.txGeneratingUnits);
		switch(type)
		{
		case FOREST:
			game.SpawnForestUnits(pos);
			break;
		case CAMP:
			game.SpawnCampUnits();
			break;
		case MOONWELL:
			game.SpawnMoonwellUnits(pos);
			break;
		case SECRET:
			game.SpawnSecretLocationUnits();
			break;
		}

		// generate items
		game.LoadingStep(game.txGeneratingItems);
		switch(type)
		{
		case FOREST:
		case SECRET:
			SpawnForestItems(0);
			break;
		case CAMP:
			SpawnForestItems(-1);
			break;
		case MOONWELL:
			SpawnForestItems(1);
			break;
		}
	}
	else if(!reenter)
	{
		if(days > 0)
			game.UpdateLocation(days, 100, false);

		if(need_reset)
		{
			// remove alive units
			for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->IsAlive())
				{
					delete *it;
					*it = nullptr;
				}
			}
			RemoveNullElements(game.local_ctx.units);
		}

		// respawn units
		game.LoadingStep(game.txGeneratingUnits);
		bool have_sawmill = false;
		if(L.location_index == QM.quest_sawmill->target_loc)
		{
			// sawmill quest
			if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
				&& QM.quest_sawmill->build_state == Quest_Sawmill::BuildState::LumberjackLeft)
			{
				game.GenerateSawmill(true);
				have_sawmill = true;
				L.location->loaded_resources = false;
			}
			else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working
				&& QM.quest_sawmill->build_state != Quest_Sawmill::BuildState::Finished)
			{
				game.GenerateSawmill(false);
				have_sawmill = true;
				L.location->loaded_resources = false;
			}
			else
				game.RespawnUnits();
		}
		else
			game.RespawnUnits();

		// recreate colliders
		game.LoadingStep(game.txGeneratingPhysics);
		game.RespawnObjectColliders();

		if(need_reset)
		{
			// spawn new units
			switch(type)
			{
			case FOREST:
				game.SpawnForestUnits(pos);
				break;
			case CAMP:
				game.SpawnCampUnits();
				break;
			case MOONWELL:
				game.SpawnMoonwellUnits(pos);
				break;
			}
		}

		if(days > 10)
		{
			switch(type)
			{
			case FOREST:
			case SECRET:
				SpawnForestItems(have_sawmill ? -1 : 0);
				break;
			case CAMP:
				SpawnForestItems(-1);
				break;
			case MOONWELL:
				SpawnForestItems(1);
				break;
			}
		}

		game.OnReenterLevel(game.local_ctx);
	}

	// create colliders
	if(!reenter)
	{
		game.LoadingStep(game.txRecreatingObjects);
		game.SpawnTerrainCollider();
		L.SpawnOutsideBariers();
	}

	// handle quest event
	if(outside->active_quest && outside->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
	{
		Quest_Event* event = outside->active_quest->GetEvent(L.location_index);
		if(event)
		{
			if(!event->done)
				game.HandleQuestEvent(event);
			L.event_handler = event->location_event_handler;
		}
	}

	// generate minimap
	game.LoadingStep(game.txGeneratingMinimap);
	game.CreateForestMinimap();

	// add player team
	if(type != SECRET)
		game.AddPlayerTeam(pos, dir, reenter, true);
	else
		game.SpawnTeamSecretLocation();

	// generate guards for bandits quest
	if(QM.quest_bandits->bandits_state == Quest_Bandits::State::GenerateGuards && L.location_index == QM.quest_bandits->target_loc)
	{
		QM.quest_bandits->bandits_state = Quest_Bandits::State::GeneratedGuards;
		UnitData* ud = UnitData::Get("guard_q_bandyci");
		int ile = Random(4, 5);
		pos += Vec3(sin(dir + PI) * 8, 0, cos(dir + PI) * 8);
		for(int i = 0; i < ile; ++i)
		{
			Unit* u = game.SpawnUnitNearLocation(game.local_ctx, pos, *ud, &Team.leader->pos, 6, 4.f);
			u->assist = true;
		}
	}
}
