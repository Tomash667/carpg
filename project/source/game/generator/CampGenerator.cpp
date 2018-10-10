#include "Pch.h"
#include "GameCore.h"
#include "CampGenerator.h"
#include "Terrain.h"
#include "OutsideLocation.h"
#include "Perlin.h"
#include "Level.h"
#include "Chest.h"
#include "UnitGroup.h"
#include "ItemHelper.h"


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
int CampGenerator::HandleUpdate()
{
	return 0;
}

//=================================================================================================
void CampGenerator::GenerateObjects()
{
	SpawnForestObjects();

	vector<Vec2> pts;
	BaseObject* ognisko = BaseObject::Get("campfire"),
		*ognisko_zgaszone = BaseObject::Get("campfire_off"),
		*namiot = BaseObject::Get("tent"),
		*poslanie = BaseObject::Get("bedding");

	if(!camp_objs_ptrs[0])
	{
		for(uint i = 0; i < n_camp_objs; ++i)
			camp_objs_ptrs[i] = BaseObject::Get(camp_objs[i]);
	}

	for(int i = 0; i < 20; ++i)
	{
		Vec2 pt = Vec2::Random(Vec2(96, 96), Vec2(256 - 96, 256 - 96));

		// sprawdŸ czy nie ma w pobli¿u ogniska
		bool jest = false;
		for(vector<Vec2>::iterator it = pts.begin(), end = pts.end(); it != end; ++it)
		{
			if(Vec2::Distance(pt, *it) < 16.f)
			{
				jest = true;
				break;
			}
		}
		if(jest)
			continue;

		pts.push_back(pt);

		// ognisko
		if(L.SpawnObjectNearLocation(L.local_ctx, Rand() % 5 == 0 ? ognisko_zgaszone : ognisko, pt, Random(MAX_ANGLE)))
		{
			// namioty / pos³ania
			for(int j = 0, count = Random(4, 7); j < count; ++j)
			{
				float kat = Random(MAX_ANGLE);
				bool czy_namiot = (Rand() % 2 == 0);
				if(czy_namiot)
					L.SpawnObjectNearLocation(L.local_ctx, namiot, pt + Vec2(sin(kat), cos(kat))*Random(4.f, 5.5f), pt);
				else
					L.SpawnObjectNearLocation(L.local_ctx, poslanie, pt + Vec2(sin(kat), cos(kat))*Random(3.f, 4.f), pt);
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
			auto e = L.SpawnObjectNearLocation(L.local_ctx, obj, pt, Random(MAX_ANGLE), 2.f);
			if(e.IsChest() && L.location->spawn != SG_NONE) // empty chests for empty camps
			{
				int gold, level = L.location->st;
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
	static TmpUnitGroup group;
	static vector<Vec2> poss;
	poss.clear();
	int level = outside->st;
	cstring group_name;

	switch(outside->spawn)
	{
	default:
		assert(0);
	case SG_BANDITS:
		group_name = "bandits";
		break;
	case SG_ORCS:
		group_name = "orcs";
		break;
	case SG_GOBLINS:
		group_name = "goblins";
		break;
	case SG_MAGES:
		group_name = "mages";
		break;
	case SG_EVIL:
		group_name = "evil";
		break;
	case SG_NONE:
		// spawn empty camp, no units
		return;
	}

	// ustal wrogów
	group.group = UnitGroup::TryGet(group_name);
	group.Fill(level);

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
			// losuj grupe
			poss.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			int levels = level * 2;
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
void CampGenerator::GenerateItems()
{
	SpawnForestItems(-1);
}
