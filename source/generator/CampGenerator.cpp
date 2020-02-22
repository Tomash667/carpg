#include "Pch.h"
#include "CampGenerator.h"
#include "Terrain.h"
#include "OutsideLocation.h"
#include "Perlin.h"
#include "Level.h"
#include "Chest.h"
#include "UnitGroup.h"
#include "ItemHelper.h"
#include "BaseObject.h"

cstring camp_objs[] = {
	"barrel",
	"barrels",
	"box",
	"boxes",
	"bow_target",
	"torch",
	"torch_off",
	"tanning_rack",
	"hay",
	"firewood",
	"bench",
	"chest",
	"melee_target",
	"anvil",
	"cauldron"
};
const uint n_camp_objs = countof(camp_objs);
BaseObject* camp_objs_ptrs[n_camp_objs];

//=================================================================================================
void CampGenerator::Generate()
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
int CampGenerator::HandleUpdate(int days)
{
	return PREVENT_RESET;
}

//=================================================================================================
void CampGenerator::GenerateObjects()
{
	SpawnForestObjects();

	vector<Vec2> pts;
	BaseObject* campfire = BaseObject::Get("campfire"),
		*campfire_off = BaseObject::Get("campfire_off"),
		*tent = BaseObject::Get("tent"),
		*bedding = BaseObject::Get("bedding");

	if(!camp_objs_ptrs[0])
	{
		for(uint i = 0; i < n_camp_objs; ++i)
			camp_objs_ptrs[i] = BaseObject::Get(camp_objs[i]);
	}

	for(int i = 0; i < 20; ++i)
	{
		Vec2 pt = Vec2::Random(Vec2(96, 96), Vec2(256 - 96, 256 - 96));

		// check if not too close to other bonfire
		bool ok = true;
		for(vector<Vec2>::iterator it = pts.begin(), end = pts.end(); it != end; ++it)
		{
			if(Vec2::Distance(pt, *it) < 16.f)
			{
				ok = false;
				break;
			}
		}
		if(!ok)
			continue;

		pts.push_back(pt);

		// campfire
		if(game_level->SpawnObjectNearLocation(*game_level->local_area, Rand() % 5 == 0 ? campfire_off : campfire, pt, Random(MAX_ANGLE)))
		{
			for(int j = 0, count = Random(4, 7); j < count; ++j)
			{
				float angle = Random(MAX_ANGLE);
				if(Rand() % 2 == 0)
					game_level->SpawnObjectNearLocation(*game_level->local_area, tent, pt + Vec2(sin(angle), cos(angle))*Random(4.f, 5.5f), pt);
				else
					game_level->SpawnObjectNearLocation(*game_level->local_area, bedding, pt + Vec2(sin(angle), cos(angle))*Random(3.f, 4.f), pt);
			}
		}
	}

	for(int i = 0; i < 100; ++i)
	{
		Vec2 pt = Vec2::Random(Vec2(90, 90), Vec2(256 - 90, 256 - 90));
		bool ok = true;
		for(vector<Vec2>::iterator it = pts.begin(), end = pts.end(); it != end; ++it)
		{
			if(Vec2::Distance(*it, pt) < 4.f)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			BaseObject* obj = camp_objs_ptrs[Rand() % n_camp_objs];
			auto e = game_level->SpawnObjectNearLocation(*game_level->local_area, obj, pt, Random(MAX_ANGLE), 2.f);
			if(e.IsChest() && !game_level->location->group->IsEmpty()) // empty chests for empty camps
			{
				int gold, level = game_level->location->st;
				Chest* chest = (Chest*)e;

				ItemHelper::GenerateTreasure(level, 5, chest->items, gold, false);
				InsertItemBare(chest->items, Item::gold, (uint)gold);
				SortItems(chest->items);
			}
		}
	}
}

//=================================================================================================
void CampGenerator::GenerateUnits()
{
	static vector<Vec2> poss;
	poss.clear();
	int level = outside->st;

	if(outside->group->IsEmpty())
		return; // spawn empty camp, no units

	Pooled<TmpUnitGroup> group;
	group->Fill(outside->group, level);

	for(int added = 0, tries = 50; added < 5 && tries>0; --tries)
	{
		Vec2 pos = Vec2::Random(Vec2(90, 90), Vec2(256 - 90, 256 - 90));

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 8.f)
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
			for(TmpUnitGroup::Spawn& spawn : group->Roll(level, 2))
			{
				if(!game_level->SpawnUnitNearLocation(*game_level->local_area, pos3, *spawn.first, nullptr, spawn.second, 6.f))
					break;
			}
		}
	}
}

//=================================================================================================
void CampGenerator::GenerateItems()
{
	SpawnForestItems(-1);
}
