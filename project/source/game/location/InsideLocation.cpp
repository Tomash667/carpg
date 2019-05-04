#include "Pch.h"
#include "GameCore.h"
#include "InsideLocation.h"
#include "GameFile.h"
#include "BitStreamFunc.h"
#include "Level.h"
#include "SaveState.h"
#include "BaseLocation.h"

//=================================================================================================
void InsideLocation::Save(GameWriter& f, bool local)
{
	Location::Save(f, local);

	f << target;
	f << special_room;
	f << from_portal;
}

//=================================================================================================
void InsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	Location::Load(f, local, token);

	f >> target;
	f >> special_room;
	f >> from_portal;

	if(LOAD_VERSION < V_0_8)
	{
		if(target == KOPALNIA_POZIOM)
			state = LS_HIDDEN;
	}
}

//=================================================================================================
void InsideLocation::Write(BitStreamWriter& f)
{
	InsideLocationLevel& lvl = GetLevelData();
	f.WriteCasted<byte>(target);
	f << from_portal;
	// map
	f.WriteCasted<byte>(lvl.w);
	f.Write((cstring)lvl.map, sizeof(Tile)*lvl.w*lvl.h);
	// lights
	f.WriteCasted<byte>(lvl.lights.size());
	for(Light& light : lvl.lights)
		light.Write(f);
	// rooms
	f.WriteCasted<byte>(lvl.rooms.size());
	for(Room* room : lvl.rooms)
		room->Write(f);
	// traps
	f.WriteCasted<byte>(lvl.traps.size());
	for(Trap* trap : lvl.traps)
		trap->Write(f);
	// doors
	f.WriteCasted<byte>(lvl.doors.size());
	for(Door* door : lvl.doors)
		door->Write(f);
	// stairs
	f << lvl.staircase_up;
	f << lvl.staircase_down;
	f.WriteCasted<byte>(lvl.staircase_up_dir);
	f.WriteCasted<byte>(lvl.staircase_down_dir);
	f << lvl.staircase_down_in_wall;

	// usable objects
	f.WriteCasted<byte>(lvl.usables.size());
	for(Usable* usable : lvl.usables)
		usable->Write(f);
	// units
	f.WriteCasted<byte>(lvl.units.size());
	for(Unit* unit : lvl.units)
		unit->Write(f);
	// ground items
	f.WriteCasted<byte>(lvl.items.size());
	for(GroundItem* item : lvl.items)
		item->Write(f);
	// bloods
	f.WriteCasted<word>(lvl.bloods.size());
	for(Blood& blood : lvl.bloods)
		blood.Write(f);
	// objects
	f.WriteCasted<word>(lvl.objects.size());
	for(Object* object : lvl.objects)
		object->Write(f);
	// chests
	f.WriteCasted<byte>(lvl.chests.size());
	for(Chest* chest : lvl.chests)
		chest->Write(f);

	WritePortals(f);
}

//=================================================================================================
bool InsideLocation::Read(BitStreamReader& f)
{
	SetActiveLevel(L.dungeon_level);
	InsideLocationLevel& lvl = GetLevelData();
	f.ReadCasted<byte>(target);
	f >> from_portal;
	f.ReadCasted<byte>(lvl.w);
	if(!f.Ensure(lvl.w * lvl.w * sizeof(Tile)))
	{
		Error("Read level: Broken packet for inside location.");
		return false;
	}

	// map
	lvl.h = lvl.w;
	if(!lvl.map)
		lvl.map = new Tile[lvl.w * lvl.h];
	f.Read(lvl.map, lvl.w * lvl.w * sizeof(Tile));

	// lights
	byte count;
	f >> count;
	if(!f.Ensure(count * Light::MIN_SIZE))
	{
		Error("Read level: Broken packet for inside location light count.");
		return false;
	}
	lvl.lights.resize(count);
	for(Light& light : lvl.lights)
		light.Read(f);
	if(!f)
	{
		Error("Read level: Broken packet for inside location light.");
		return false;
	}

	// rooms
	f >> count;
	if(!f.Ensure(count * Room::MIN_SIZE))
	{
		Error("Read level: Broken packet for inside location room count.");
		return false;
	}
	lvl.rooms.resize(count);
	int index = 0;
	for(Room*& room : lvl.rooms)
	{
		room = Room::Get();
		room->Read(f);
		room->index = index++;
	}
	for(Room* room : lvl.rooms)
	{
		for(Room*& c : room->connected)
			c = lvl.rooms[(int)c];
	}
	if(!f)
	{
		Error("Read level: Broken packet for inside location room.");
		return false;
	}

	// traps
	f >> count;
	if(!f.Ensure(count * Trap::MIN_SIZE))
	{
		Error("Read level: Broken packet for inside location trap count.");
		return false;
	}
	lvl.traps.resize(count);
	for(Trap*& trap : lvl.traps)
	{
		trap = new Trap;
		if(!trap->Read(f))
		{
			Error("Read level: Broken packet for inside location trap.");
			return false;
		}
	}

	// doors
	f >> count;
	if(!f.Ensure(count * Door::MIN_SIZE))
	{
		Error("Read level: Broken packet for inside location door count.");
		return false;
	}
	lvl.doors.resize(count);
	for(Door*& door : lvl.doors)
	{
		door = new Door;
		if(!door->Read(f))
		{
			Error("Read level: Broken packet for inside location door.");
			return false;
		}
	}

	// stairs
	f >> lvl.staircase_up;
	f >> lvl.staircase_down;
	f.ReadCasted<byte>(lvl.staircase_up_dir);
	f.ReadCasted<byte>(lvl.staircase_down_dir);
	f >> lvl.staircase_down_in_wall;
	if(!f)
	{
		Error("Read level: Broken packet for stairs.");
		return false;
	}

	// usable objects
	f >> count;
	if(!f.Ensure(count * Usable::MIN_SIZE))
	{
		Error("Read level: Broken usable object count.");
		return false;
	}
	lvl.usables.resize(count);
	for(Usable*& usable : lvl.usables)
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
	lvl.units.resize(count);
	for(Unit*& unit : lvl.units)
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
	lvl.items.resize(count);
	for(GroundItem*& item : lvl.items)
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
	lvl.bloods.resize(count2);
	for(Blood& blood : lvl.bloods)
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
	lvl.objects.resize(count2);
	for(Object*& object : lvl.objects)
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
	lvl.chests.resize(count);
	for(Chest*& chest : lvl.chests)
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
bool InsideLocation::RemoveItemFromChest(const Item* item, int& at_level)
{
	assert(item);

	int index;
	Chest* chest = FindChestWithItem(item, at_level, &index);
	if(!chest)
		return false;
	else
	{
		chest->RemoveItem(index);
		return true;
	}
}

//=================================================================================================
bool InsideLocation::RemoveQuestItemFromChest(int quest_refid, int& at_level)
{
	int index;
	Chest* chest = FindChestWithQuestItem(quest_refid, at_level, &index);
	if(!chest)
		return false;
	else
	{
		if(!chest->GetUser())
			chest->RemoveItem(index);
		return true;
	}
}
