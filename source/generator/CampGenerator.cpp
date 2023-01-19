#include "Pch.h"
#include "CampGenerator.h"

#include "BaseObject.h"
#include "Chest.h"
#include "ItemHelper.h"
#include "Level.h"
#include "Net.h"
#include "Object.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "UnitGroup.h"
#include "UnitData.h"

#include <Perlin.h>
#include <Terrain.h>

//=================================================================================================
void CampGenerator::InitOnce()
{
	objs[CAMPFIRE] = BaseObject::Get("campfire");
	objs[CAMPFIRE_OFF] = BaseObject::Get("campfireOff");
	objs[TENT] = BaseObject::Get("tent");
	objs[BEDDING] = BaseObject::Get("bedding");
	objs[BENCH] = BaseObject::Get("bench");
	objs[BARREL] = BaseObject::Get("barrel");
	objs[BARRELS] = BaseObject::Get("barrels");
	objs[BOX] = BaseObject::Get("box");
	objs[BOXES] = BaseObject::Get("boxes");
	objs[BOW_TARGET] = BaseObject::Get("bowTarget");
	objs[CHEST] = BaseObject::Get("chest");
	objs[TORCH] = BaseObject::Get("torch");
	objs[TORCH_OFF] = BaseObject::Get("torchOff");
	objs[TANNING_RACK] = BaseObject::Get("tanningRack");
	objs[HAY] = BaseObject::Get("hay");
	objs[FIREWOOD] = BaseObject::Get("firewood");
	objs[MELEE_TARGET] = BaseObject::Get("meleeTarget");
	objs[ANVIL] = BaseObject::Get("anvil");
	objs[CAULDRON] = BaseObject::Get("cauldron");
}

//=================================================================================================
void CampGenerator::Init()
{
	OutsideLocationGenerator::Init();
	if(Net::IsLocal())
	{
		isEmpty = gameLevel->location->group->IsEmpty()
			&& outside->target != HUNTERS_CAMP;
	}
}

//=================================================================================================
void CampGenerator::Generate()
{
	CreateMap();
	RandomizeTerrainTexture();

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();
	Perlin perlin(4, 4);

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

	used.clear();
	int count = Random(8, 10);
	for(int i = 0; i < 20 && count > 0; ++i)
	{
		Vec2 pos = Vec2::Random(Vec2(96, 96), Vec2(256 - 96, 256 - 96));

		bool ok = true;
		for(vector<Vec2>::iterator it = used.begin(), end = used.end(); it != end; ++it)
		{
			if(Vec2::DistanceSquared(pos, *it) < Pow2(16.f))
			{
				ok = false;
				break;
			}
		}
		if(!ok)
			continue;

		used.push_back(pos);
		--count;

		// mark terrain as patch
		const Int2 pt = PosToPt(pos);
		float hSum = 0;
		int hCount = 0;
		for(int y = -5; y <= 5; ++y)
		{
			for(int x = -5; x <= 5; ++x)
			{
				const Int2 pt2(pt.x + x, pt.y + y);
				if(Vec3::DistanceSquared(PtToPos(pt), PtToPos(pt2)) < Pow2(10.f))
				{
					TerrainTile& tile = outside->tiles[pt2.x + pt2.y * s];
					tile.t = TT_SAND;
					hSum += outside->h[pt2.x + pt2.y * (s + 1)];
					++hCount;
				}
			}
		}

		// round height under camp
		hSum /= hCount;
		for(int y = -5; y <= 5; ++y)
		{
			for(int x = -5; x <= 5; ++x)
			{
				const Int2 pt2(pt.x + x, pt.y + y);
				if(Vec3::DistanceSquared(PtToPos(pt), PtToPos(pt2)) < Pow2(10.f))
					outside->h[pt2.x + pt2.y * (s + 1)] = hSum;
			}
		}
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
	LocationPart& locPart = *gameLevel->localPart;
	tents.clear();

	// bonfire with tents/beddings
	uint count = used.size() / 2;
	if(used.size() % 2 != 0)
		++count;
	for(uint i = 0; i < count; ++i)
	{
		Vec2 pos = used[i];

		// campfire
		gameLevel->SpawnObjectNearLocation(locPart, objs[isEmpty ? CAMPFIRE_OFF : CAMPFIRE], pos, Random(MAX_ANGLE));

		// bench
		if(Rand() % 3 == 0)
		{
			const float angle = Random(MAX_ANGLE);
			gameLevel->SpawnObjectNearLocation(locPart, objs[BENCH], pos + Vec2(sin(angle), cos(angle)) * 2.5f, pos);
		}

		// tents/beddings/benches around
		for(int j = 0, count = Random(3, 5); j < count; ++j)
		{
			const float angle = Random(MAX_ANGLE);
			ObjId id;
			float dist;
			if(j == 0 || Rand() % 2 == 0)
			{
				id = TENT;
				dist = 5.5f;
			}
			else
			{
				id = BEDDING;
				dist = 5.f;
			}
			ObjectEntity entity = gameLevel->SpawnObjectNearLocation(locPart, objs[id], pos + Vec2(sin(angle), cos(angle)) * dist, pos);
			if(id == TENT && entity)
				tents.push_back(entity);
		}
	}

	// stuff
	bool haveMeleeTarget = false, haveRangedTarget = false, haveCrafting = false;
	for(uint i = count; i < used.size(); ++i)
	{
		Vec2 pos = used[i];

		int type = Rand() % 4;
		switch(type)
		{
		case 1:
			if(haveMeleeTarget)
				type = 0;
			haveMeleeTarget = true;
			break;
		case 2:
			if(haveRangedTarget)
				type = 0;
			haveRangedTarget = true;
			break;
		case 3:
			if(haveCrafting)
				type = 0;
			haveCrafting = true;
			break;
		}

		// torch
		gameLevel->SpawnObjectNearLocation(locPart, objs[isEmpty ? TORCH_OFF : TORCH], pos, Random(MAX_ANGLE), 0.f);

		switch(type)
		{
		case 0: // chest & boxes/barrels/hay
			gameLevel->SpawnObjectNearLocation(locPart, objs[CHEST], pos, Random(MAX_ANGLE), 10.f);
			for(int j = 0, count = Random(5, 10); j < count; ++j)
			{
				const ObjId ids[] = { BOX, BOXES, BARREL, BARRELS, HAY };
				int count2 = Random(2, 4);
				const Vec2 pos2 = pos + Vec2::RandomCirclePt(7);
				ObjId id = ids[Rand() % countof(ids)];
				for(int k = 0; k < count2; ++k)
					gameLevel->SpawnObjectNearLocation(locPart, objs[id], pos2, Random(MAX_ANGLE), 2, 0);
			}
			break;
		case 1: // melee targets
			for(int j = 0, count = Random(2, 4); j < count; ++j)
				gameLevel->SpawnObjectNearLocation(locPart, objs[MELEE_TARGET], pos + Vec2::RandomCirclePt(7), Random(MAX_ANGLE));
			break;
		case 2: // bow targets
			{
				const float angle = Random(MAX_ANGLE);
				for(int j = 0, count = Random(2, 4); j < count; ++j)
					gameLevel->SpawnObjectNearLocation(locPart, objs[BOW_TARGET], pos + Vec2::RandomCirclePt(7), angle);
			}
			break;
		case 3: // crafting
			{
				const ObjId ids[] = { CHEST, ANVIL, CAULDRON, TANNING_RACK, TANNING_RACK };
				for(uint i = 0; i < countof(ids); ++i)
					gameLevel->SpawnObjectNearLocation(locPart, objs[ids[i]], pos + Vec2::RandomCirclePt(7), Random(MAX_ANGLE));
			}
			break;
		}
	}

	// chests
	for(int i = 0; i < 3 && !tents.empty(); ++i)
	{
		const float dist = 2.f;
		int index = Rand() % tents.size();
		Object* tent = tents[index];
		tents.erase(tents.begin() + index);
		float angle = tent->rot.y + (Rand() % 2 == 0 ? -1 : +1) * PI / 2;
		Vec2 pos = tent->pos.XZ() + Vec2(sin(angle) * dist, cos(angle) * dist);
		if(!gameLevel->SpawnObjectNearLocation(locPart, objs[CHEST], pos, Clip(angle + PI)))
		{
			angle += PI;
			pos = tent->pos.XZ() + Vec2(sin(angle) * dist, cos(angle) * dist);
			if(!gameLevel->SpawnObjectNearLocation(locPart, objs[CHEST], pos, Clip(angle + PI)))
				--i;
		}
	}

	// fill chests
	ItemHelper::GenerateTreasure(locPart.chests, outside->st, 3);

	// trees
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		TERRAIN_TILE tile = outside->tiles[pt.x + pt.y * OutsideLocation::size].t;
		if(tile != TT_GRASS && tile != TT_GRASS3)
			continue;

		Vec2 pos2d(Random(2.f) + 2.f * pt.x, Random(2.f) + 2.f * pt.y);
		bool ok = true;
		for(vector<Vec2>::iterator it = used.begin(), end = used.end(); it != end; ++it)
		{
			if(Vec2::DistanceSquared(pos2d, *it) < Pow2(10.f))
			{
				ok = false;
				break;
			}
		}
		if(!ok)
			continue;

		Vec3 pos = pos2d.XZ();
		pos.y = terrain->GetH(pos);
		OutsideObject* o;
		if(tile == TT_GRASS)
			o = &trees[Rand() % nTrees];
		else
		{
			int type;
			if(Rand() % 12 == 0)
				type = 3;
			else
				type = Rand() % 3;
			o = &trees2[type];
		}
		gameLevel->SpawnObjectEntity(locPart, o->obj, pos, Random(MAX_ANGLE), o->scale.Random());
	}

	// other objects
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(outside->tiles[pt.x + pt.y * OutsideLocation::size].t == TT_SAND)
			continue;

		Vec2 pos2d(Random(2.f) + 2.f * pt.x, Random(2.f) + 2.f * pt.y);
		bool ok = true;
		for(vector<Vec2>::iterator it = used.begin(), end = used.end(); it != end; ++it)
		{
			if(Vec2::DistanceSquared(pos2d, *it) < Pow2(10.f))
			{
				ok = false;
				break;
			}
		}
		if(!ok)
			continue;

		Vec3 pos = pos2d.XZ();
		pos.y = terrain->GetH(pos);
		OutsideObject& o = misc[Rand() % nMisc];
		gameLevel->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
	}
}

//=================================================================================================
void CampGenerator::GenerateUnits()
{
	LocationPart& locPart = *gameLevel->localPart;
	static vector<Vec2> poss;
	poss.clear();
	int level = outside->st;

	if(outside->target == HUNTERS_CAMP)
	{
		UnitData* hunter = UnitData::Get("hunter"),
			*hunterLeader = UnitData::Get("hunterLeader");

		// leader
		const Vec2 center(128, 128);
		Vec2 bestPos = used[0];
		float bestDist = Vec2::DistanceSquared(bestPos, center);
		for(uint i = 1; i < used.size(); ++i)
		{
			float dist = Vec2::DistanceSquared(used[i], center);
			if(dist < bestDist)
			{
				bestPos = used[i];
				bestDist = dist;
			}
		}
		gameLevel->SpawnUnitNearLocation(locPart, bestPos.XZ(), *hunterLeader, nullptr, -1, 8.f);

		// hunters
		for(int i = 0; i < 7; ++i)
		{
			const Vec3 pos = used[Rand() % used.size()].XZ();
			gameLevel->SpawnUnitNearLocation(locPart, pos, *hunter, nullptr, -1, 8.f);
		}
		return;
	}

	if(outside->group->IsEmpty())
		return; // spawn empty camp, no units

	Pooled<TmpUnitGroup> group;
	group->Fill(outside->group, level);

	for(int i = 0; i < 5; ++i)
	{
		int index = Rand() % used.size();
		Vec3 pos = used[index].XZ();
		used.erase(used.begin() + index);
		for(TmpUnitGroup::Spawn& spawn : group->Roll(level, 2))
		{
			if(!gameLevel->SpawnUnitNearLocation(locPart, pos, *spawn.first, nullptr, spawn.second, 8.f))
				break;
		}
	}
}

//=================================================================================================
void CampGenerator::GenerateItems()
{
	SpawnForestItems(-1);
}
