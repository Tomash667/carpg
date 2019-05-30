#include "Pch.h"
#include "GameCore.h"
#include "MoonwellGenerator.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "Terrain.h"
#include "Perlin.h"
#include "Level.h"
#include "UnitGroup.h"

//=================================================================================================
void MoonwellGenerator::Generate()
{
	CreateMap();
	RandomizeTerrainTexture();
	terrain->SetHeightMap(outside->h);
	float* h = terrain->GetHeightMap();
	float hmax = 5.f;
	int octaves = 3;
	float frequency = 4.f;
	RandomizeHeight(octaves, frequency, 0.f, hmax);

	// set green hill in middle
	for(uint y = 40; y < s - 40; ++y)
	{
		for(uint x = 40; x < s - 40; ++x)
		{
			float d;
			if((d = Distance(float(x), float(y), 64.f, 64.f)) < 8.f)
			{
				outside->tiles[x + y * s].t = TT_GRASS;
				h[x + y * (s + 1)] += (1.f - d / 8.f) * 5;
			}
		}
	}

	// no grass under moon well
	for(int i=0; i<2; ++i)
		for(int j=0; j<2; ++j)
			outside->tiles[63 + i + (63 + j) * s].mode = TM_NO_GRASS;

	terrain->RoundHeight();
	terrain->RoundHeight();
	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
void MoonwellGenerator::GenerateObjects()
{
	Vec3 pos(128.f, 0, 128.f);
	terrain->SetH(pos);
	pos.y -= 0.2f;
	L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("moonwell"), pos, 0.f);
	L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("moonwell_phy"), pos, 0.f);

	TerrainTile* tiles = ((OutsideLocation*)L.location)->tiles;

	// trees
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 64.f) > 5.f)
		{
			TERRAIN_TILE tile = tiles[pt.x + pt.y*OutsideLocation::size].t;
			if(tile == TT_GRASS)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = trees[Rand() % n_trees];
				L.SpawnObjectEntity(L.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
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
				L.SpawnObjectEntity(L.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}

	// other objects
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 64.f) > 5.f)
		{
			if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = misc[Rand() % n_misc];
				L.SpawnObjectEntity(L.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}
}

//=================================================================================================
void MoonwellGenerator::GenerateUnits()
{
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	const int level = L.GetDifficultyLevel();
	TmpUnitGroupList tmp;
	tmp.Fill(loc->group, level);
	static vector<Vec2> poss;
	poss.clear();
	poss.push_back(Vec2(team_pos.x, team_pos.z));

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();
		if(Vec2::Distance(pos, Vec2(128.f, 128.f)) < 12.f)
			continue;

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

			// spawn units
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
void MoonwellGenerator::GenerateItems()
{
	SpawnForestItems(1);
}
