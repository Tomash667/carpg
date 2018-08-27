// zewnêtrzna lokacja
#include "Pch.h"
#include "GameCore.h"
#include "OutsideLocation.h"
#include "SaveState.h"
#include "Unit.h"
#include "Object.h"
#include "Chest.h"
#include "GroundItem.h"
#include "GameFile.h"

namespace OLD
{
	enum TERRAIN_TILE : byte
	{
		TT_GRASS,
		TT_GRASS2,
		TT_GRASS3,
		TT_SAND,
		TT_ROAD,
		TT_BUILD_GRASS,
		TT_BUILD_SAND,
		TT_BUILD_ROAD
	};

	const int TM_BUILDING_NO_PHY = 7;
}

//=================================================================================================
OutsideLocation::~OutsideLocation()
{
	delete[] tiles;
	delete[] h;
	DeleteElements(objects);
	DeleteElements(units);
	DeleteElements(chests);
	DeleteElements(usables);
	DeleteElements(items);
}

//=================================================================================================
void OutsideLocation::ApplyContext(LevelContext& ctx)
{
	ctx.units = &units;
	ctx.objects = &objects;
	ctx.chests = &chests;
	ctx.traps = nullptr;
	ctx.doors = nullptr;
	ctx.items = &items;
	ctx.usables = &usables;
	ctx.bloods = &bloods;
	ctx.lights = nullptr;
	ctx.have_terrain = true;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Outside;
	ctx.building_id = -1;
	ctx.mine = Int2(0, 0);
	ctx.maxe = Int2(size, size);
	ctx.tmp_ctx = nullptr;
	ctx.masks = nullptr;
}

//=================================================================================================
void OutsideLocation::Save(HANDLE file, bool local)
{
	GameWriter f(file);
	Location::Save(file, local);

	if(last_visit != -1)
	{
		// units
		f << units.size();
		for(Unit* unit : units)
			unit->Save(f, local);

		// objects
		f << objects.size();
		for(Object* object : objects)
			object->Save(f);

		// chests
		f << chests.size();
		for(Chest* chest : chests)
			chest->Save(f, local);

		// ground items
		f << items.size();
		for(GroundItem* item : items)
			item->Save(f);

		// usable objects
		f << usables.size();
		for(Usable* usable : usables)
			usable->Save(f, local);

		// blood
		f << bloods.size();
		for(Blood& blood : bloods)
			blood.Save(f);

		// teren
		WriteFile(file, tiles, sizeof(TerrainTile)*size*size, &tmp, nullptr);
		int size2 = size + 1;
		size2 *= size2;
		WriteFile(file, h, sizeof(float)*size2, &tmp, nullptr);
	}
}

//=================================================================================================
void OutsideLocation::Load(HANDLE file, bool local, LOCATION_TOKEN token)
{
	GameReader f(file);
	Location::Load(file, local, token);

	if(last_visit != -1)
	{
		// units
		units.resize(f.Read<uint>());
		for(Unit*& unit : units)
		{
			unit = new Unit;
			Unit::AddRefid(unit);
			unit->Load(f, local);
		}

		// objects
		objects.resize(f.Read<uint>());
		for(Object*& object : objects)
		{
			object = new Object;
			object->Load(f);
		}

		// chests
		chests.resize(f.Read<uint>());
		for(Chest*& chest : chests)
		{
			chest = new Chest;
			chest->Load(f, local);
		}

		// ground items
		items.resize(f.Read<uint>());
		for(GroundItem*& item : items)
		{
			item = new GroundItem;
			item->Load(f);
		}

		// usable objects
		usables.resize(f.Read<uint>());
		for(Usable*& usable : usables)
		{
			usable = new Usable;
			Usable::AddRefid(usable);
			usable->Load(f, local);
		}

		// bloods
		bloods.resize(f.Read<uint>());
		for(Blood& blood : bloods)
			blood.Load(f);

		// teren
		int size2 = size + 1;
		size2 *= size2;
		h = new float[size2];
		tiles = new TerrainTile[size*size];
		if(LOAD_VERSION >= V_0_3)
		{
			ReadFile(file, tiles, sizeof(TerrainTile)*size*size, &tmp, nullptr);
			if(LOAD_VERSION < V_0_5)
			{
				for(int i = 0; i < size*size; ++i)
				{
					TerrainTile& tt = tiles[i];
					if(tt.mode == OLD::TM_BUILDING_NO_PHY)
						tt.mode = TM_BUILDING;
				}
			}
		}
		else
		{
			OLD::TERRAIN_TILE* old_tiles = new OLD::TERRAIN_TILE[size*size];
			ReadFile(file, old_tiles, sizeof(OLD::TERRAIN_TILE)*size*size, &tmp, nullptr);
			for(int i = 0; i < size*size; ++i)
			{
				TerrainTile& tt = tiles[i];
				tt.t2 = TT_ROAD;
				tt.alpha = 0;
				switch(old_tiles[i])
				{
				case OLD::TT_GRASS:
					tt.t = TT_GRASS;
					tt.mode = TM_NORMAL;
					break;
				case OLD::TT_GRASS2:
					tt.t = TT_GRASS2;
					tt.mode = TM_NORMAL;
					break;
				case OLD::TT_GRASS3:
					tt.t = TT_GRASS3;
					tt.mode = TM_NORMAL;
					break;
				case OLD::TT_SAND:
					tt.t = TT_SAND;
					tt.mode = TM_PATH;
					break;
				case OLD::TT_ROAD:
					tt.t = TT_ROAD;
					tt.mode = TM_ROAD;
					break;
				case OLD::TT_BUILD_GRASS:
					tt.t = TT_GRASS;
					tt.mode = TM_BUILDING_BLOCK;
					break;
				case OLD::TT_BUILD_SAND:
					tt.t = TT_SAND;
					tt.mode = TM_BUILDING_BLOCK;
					break;
				case OLD::TT_BUILD_ROAD:
					tt.t = TT_ROAD;
					tt.mode = TM_BUILDING_BLOCK;
					break;
				}
			}
			delete[] old_tiles;
		}
		ReadFile(file, h, sizeof(float)*size2, &tmp, nullptr);

		// konwersja ³awy w obrócon¹ ³awê i ustawienie wariantu
		if(LOAD_VERSION < V_0_2_20)
		{
			auto bench = BaseUsable::Get("bench"),
				bench_dir = BaseUsable::Get("bench_dir");
			for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
			{
				Usable& u = **it;
				if(u.base == bench)
				{
					if(type == L_CITY)
					{
						u.base = bench_dir;
						u.variant = 0;
					}
					else
						u.variant = Rand() % 2 + 2;
				}
			}
		}
	}
}

//=================================================================================================
void OutsideLocation::BuildRefidTable()
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		(*it)->refid = (int)Unit::refid_table.size();
		Unit::refid_table.push_back(*it);
	}

	for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
	{
		(*it)->refid = (int)Usable::refid_table.size();
		Usable::refid_table.push_back(*it);
	}
}

//=================================================================================================
bool OutsideLocation::FindUnit(Unit* unit, int* level)
{
	assert(unit);

	for(Unit* u : units)
	{
		if(unit == u)
		{
			if(level)
				*level = -1;
			return true;
		}
	}

	return false;
}

//=================================================================================================
Unit* OutsideLocation::FindUnit(UnitData* data, int& at_level)
{
	assert(data);

	for(Unit* u : units)
	{
		if(u->data == data)
		{
			at_level = -1;
			return u;
		}
	}

	return nullptr;
}
