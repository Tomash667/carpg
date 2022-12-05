#include "Pch.h"
#include "SecretLocationGenerator.h"

#include "BaseObject.h"
#include "GameCommon.h"
#include "Level.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "UnitData.h"

#include <Perlin.h>
#include <Terrain.h>

//=================================================================================================
void SecretLocationGenerator::Generate()
{
	questMgr->questSecret->state = Quest_Secret::SECRET_GENERATED2;

	CreateMap();
	RandomizeTerrainTexture();

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();
	Perlin perlin(4, 4);

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

	terrain->RemoveHeightMap();
}

//=================================================================================================
void SecretLocationGenerator::GenerateObjects()
{
	LocationPart& locPart = *gameLevel->localPart;

	Vec3 pos(128.f, 0, 96.f * 2);
	terrain->SetY(pos);
	BaseObject* o = BaseObject::Get("tomashu_dom");
	pos.y += 0.05f;
	gameLevel->SpawnObjectEntity(locPart, o, pos, 0);
	gameLevel->ProcessBuildingObjects(locPart, nullptr, nullptr, o->mesh, nullptr, 0.f, GDIR_DOWN, Vec3(0, 0, 0), nullptr, nullptr, false);

	pos.z = 64.f;
	terrain->SetY(pos);
	gameLevel->SpawnObjectEntity(locPart, BaseObject::Get("portal"), pos, 0);

	Portal* portal = new Portal;
	portal->at_level = -1;
	portal->next_portal = nullptr;
	portal->pos = pos;
	portal->rot = 0.f;
	portal->index = 0;
	portal->target_loc = questMgr->questSecret->where;
	gameLevel->location->portal = portal;

	TerrainTile* tiles = ((OutsideLocation*)gameLevel->location)->tiles;

	// trees
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 32.f) > 4.f
			&& Distance(float(pt.x), float(pt.y), 64.f, 96.f) > 12.f)
		{
			TERRAIN_TILE tile = tiles[pt.x + pt.y*OutsideLocation::size].t;
			if(tile == TT_GRASS)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = trees[Rand() % n_trees];
				gameLevel->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
			else if(tile == TT_GRASS3)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				int type;
				if(Rand() % 12 == 0)
					type = 3;
				else
					type = Rand() % 3;
				OutsideObject& o = trees2[type];
				gameLevel->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}

	// other objects
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 32.f) > 4.f
			&& Distance(float(pt.x), float(pt.y), 64.f, 96.f) > 12.f)
		{
			if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = misc[Rand() % n_misc];
				gameLevel->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}
}

//=================================================================================================
void SecretLocationGenerator::GenerateUnits()
{
	LocationPart& locPart = *gameLevel->localPart;
	UnitData* golem = UnitData::Get("golem_adamantine");
	static vector<Vec2> poss;

	poss.push_back(Vec2(128.f, 64.f));
	poss.push_back(Vec2(128.f, 256.f - 64.f));

	for(int added = 0, tries = 50; added < 10 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 32.f)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			gameLevel->SpawnUnitNearLocation(locPart, Vec3(pos.x, 0, pos.y), *golem, nullptr, -2);
			poss.push_back(pos);
			++added;
		}
	}

	poss.clear();
}

//=================================================================================================
void SecretLocationGenerator::GenerateItems()
{
	SpawnForestItems(0);
}

//=================================================================================================
void SecretLocationGenerator::SpawnTeam()
{
	gameLevel->AddPlayerTeam(Vec3(128.f, 0.f, 66.f), PI);
}

//=================================================================================================
int SecretLocationGenerator::HandleUpdate(int days)
{
	return PREVENT_RESET;
}
