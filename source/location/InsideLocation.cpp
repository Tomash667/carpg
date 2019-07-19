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

	lvl.Write(f);

	// map
	f.WriteCasted<byte>(lvl.w);
	f.Write((cstring)lvl.map, sizeof(Tile)*lvl.w*lvl.h);
	
	// rooms
	f.WriteCasted<byte>(lvl.rooms.size());
	for(Room* room : lvl.rooms)
		room->Write(f);

	// stairs
	f << lvl.staircase_up;
	f << lvl.staircase_down;
	f.WriteCasted<byte>(lvl.staircase_up_dir);
	f.WriteCasted<byte>(lvl.staircase_down_dir);
	f << lvl.staircase_down_in_wall;

	WritePortals(f);
}

//=================================================================================================
bool InsideLocation::Read(BitStreamReader& f)
{
	SetActiveLevel(L.dungeon_level);
	InsideLocationLevel& lvl = GetLevelData();
	f.ReadCasted<byte>(target);
	f >> from_portal;

	if(!lvl.Read(f))
		return false;

	// map
	f.ReadCasted<byte>(lvl.w);
	if(!f.Ensure(lvl.w * lvl.w * sizeof(Tile)))
	{
		Error("Read level: Broken packet for inside location.");
		return false;
	}
	lvl.h = lvl.w;
	if(!lvl.map)
		lvl.map = new Tile[lvl.w * lvl.h];
	f.Read(lvl.map, lvl.w * lvl.w * sizeof(Tile));

	// rooms
	byte count;
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
