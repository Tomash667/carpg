#include "Pch.h"
#include "GameCore.h"
#include "OutsideLocation.h"
#include "SaveState.h"
#include "Unit.h"
#include "Object.h"
#include "Chest.h"
#include "GroundItem.h"
#include "GameFile.h"
#include "BitStreamFunc.h"
#include "Level.h"

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
void OutsideLocation::Save(GameWriter& f, bool local)
{
	Location::Save(f, local);

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

		// terrain
		f.Write(tiles, sizeof(TerrainTile)*size*size);
		int size2 = size + 1;
		size2 *= size2;
		f.Write(h, sizeof(float)*size2);
	}
}

//=================================================================================================
void OutsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	Location::Load(f, local, token);

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

		// terrain
		int size2 = size + 1;
		size2 *= size2;
		h = new float[size2];
		tiles = new TerrainTile[size*size];
		if(LOAD_VERSION >= V_0_3)
		{
			f.Read(tiles, sizeof(TerrainTile)*size*size);
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
			f.Read(old_tiles, sizeof(OLD::TERRAIN_TILE)*size*size);
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
		f.Read(h, sizeof(float)*size2);
	}

	if(LOAD_VERSION < V_0_8 && st == 20 && type == L_FOREST)
		state = LS_HIDDEN;
}

//=================================================================================================
void OutsideLocation::Write(BitStreamWriter& f)
{
	f.Write((cstring)tiles, sizeof(TerrainTile)*size*size);
	f.Write((cstring)h, sizeof(float)*(size + 1)*(size + 1));
	f << L.light_angle;

	// usable objects
	f.WriteCasted<byte>(usables.size());
	for(Usable* usable : usables)
		usable->Write(f);
	// units
	f.WriteCasted<byte>(units.size());
	for(Unit* unit : units)
		unit->Write(f);
	// ground items
	f.WriteCasted<byte>(items.size());
	for(GroundItem* item : items)
		item->Write(f);
	// bloods
	f.WriteCasted<word>(bloods.size());
	for(Blood& blood : bloods)
		blood.Write(f);
	// objects
	f.WriteCasted<word>(objects.size());
	for(Object* object : objects)
		object->Write(f);
	// chests
	f.WriteCasted<byte>(chests.size());
	for(Chest* chest : chests)
		chest->Write(f);

	WritePortals(f);
}

//=================================================================================================
bool OutsideLocation::Read(BitStreamReader& f)
{
	int size11 = size*size;
	int size22 = size + 1;
	size22 *= size22;
	if(!tiles)
		tiles = new TerrainTile[size11];
	if(!h)
		h = new float[size22];
	f.Read((char*)tiles, sizeof(TerrainTile)*size11);
	f.Read((char*)h, sizeof(float)*size22);
	f >> L.light_angle;
	if(!f)
	{
		Error("Read level: Broken packet for terrain.");
		return false;
	}

	// usable objects
	byte count;
	f >> count;
	if(!f.Ensure(count * Usable::MIN_SIZE))
	{
		Error("Read level: Broken usable object count.");
		return false;
	}
	usables.resize(count);
	for(Usable*& usable : usables)
	{
		usable = new Usable;
		if(!usable->Read(f))
		{
			Error("Read level: Broken usable object.");
			return false;
		}
	}

	// units
	f >> count;
	if(!f.Ensure(count * Unit::MIN_SIZE))
	{
		Error("Read level: Broken unit count.");
		return false;
	}
	units.resize(count);
	for(Unit*& unit : units)
	{
		unit = new Unit;
		if(!unit->Read(f))
		{
			Error("Read level: Broken unit.");
			return false;
		}
	}

	// ground items
	f >> count;
	if(!f.Ensure(count * GroundItem::MIN_SIZE))
	{
		Error("Read level: Broken ground item count.");
		return false;
	}
	items.resize(count);
	for(GroundItem*& item : items)
	{
		item = new GroundItem;
		if(!item->Read(f))
		{
			Error("Read level: Broken ground item.");
			return false;
		}
	}

	// bloods
	word count2;
	f >> count2;
	if(!f.Ensure(count2 * Blood::MIN_SIZE))
	{
		Error("Read level: Broken blood count.");
		return false;
	}
	bloods.resize(count2);
	for(Blood& blood : bloods)
		blood.Read(f);
	if(!f)
	{
		Error("Read level: Broken blood.");
		return false;
	}

	// objects
	f >> count2;
	if(!f.Ensure(count2 * Object::MIN_SIZE))
	{
		Error("Read level: Broken object count.");
		return false;
	}
	objects.resize(count2);
	for(Object*& object : objects)
	{
		object = new Object;
		if(!object->Read(f))
		{
			Error("Read level: Broken object.");
			return false;
		}
	}

	// chests
	f >> count;
	if(!f.Ensure(count * Chest::MIN_SIZE))
	{
		Error("Read level: Broken chest count.");
		return false;
	}
	chests.resize(count);
	for(Chest*& chest : chests)
	{
		chest = new Chest;
		if(!chest->Read(f))
		{
			Error("Read level: Broken chest.");
			return false;
		}
	}

	// portals
	if(!ReadPortals(f, L.dungeon_level))
	{
		Error("Read level: Broken portals.");
		return false;
	}

	return true;
}

//=================================================================================================
void OutsideLocation::BuildRefidTables()
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
