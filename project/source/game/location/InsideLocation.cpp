#include "Pch.h"
#include "GameCore.h"
#include "InsideLocation.h"
#include "GameFile.h"
#include "BitStreamFunc.h"

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
}

//=================================================================================================
void InsideLocation::Write(BitStreamWriter& f)
{
	InsideLocationLevel& lvl = GetLevelData();
	f.WriteCasted<byte>(target);
	f << from_portal;
	// map
	f.WriteCasted<byte>(lvl.w);
	f.Write((cstring)lvl.map, sizeof(Pole)*lvl.w*lvl.h);
	// lights
	f.WriteCasted<byte>(lvl.lights.size());
	for(Light& light : lvl.lights)
		light.Write(f);
	// rooms
	f.WriteCasted<byte>(lvl.rooms.size());
	for(Room& room : lvl.rooms)
		room.Write(f);
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
}

//=================================================================================================
bool InsideLocation::Read(BitStreamReader& f)
{
	InsideLocationLevel& lvl = GetLevelData();
	f.ReadCasted<byte>(target);
	f >> from_portal;
	f.ReadCasted<byte>(lvl.w);
	if(!f.Ensure(lvl.w * lvl.w * sizeof(Pole)))
	{
		Error("Read level: Broken packet for inside location.");
		return false;
	}

	// map
	lvl.h = lvl.w;
	if(!lvl.map)
		lvl.map = new Pole[lvl.w * lvl.h];
	f.Read(lvl.map, lvl.w * lvl.w * sizeof(Pole));

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
	for(Room& room : lvl.rooms)
		room.Read(f);
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
		if(!chest->user)
			chest->RemoveItem(index);
		return true;
	}
}
