#include "Pch.h"
#include "InsideLocation.h"

#include "BaseLocation.h"
#include "BitStreamFunc.h"
#include "GameFile.h"
#include "Level.h"
#include "SaveState.h"

//=================================================================================================
void InsideLocation::Save(GameWriter& f)
{
	Location::Save(f);

	f << special_room;
	f << from_portal;
}

//=================================================================================================
void InsideLocation::Load(GameReader& f)
{
	Location::Load(f);

	if(LOAD_VERSION < V_0_12)
		f >> target;
	f >> special_room;
	f >> from_portal;

	if(LOAD_VERSION < V_0_8)
	{
		if(target == ANCIENT_ARMORY)
			state = LS_HIDDEN;
	}
}

//=================================================================================================
void InsideLocation::Write(BitStreamWriter& f)
{
	InsideLocationLevel& lvl = GetLevelData();
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
	f.WriteCasted<byte>(lvl.prevEntryType);
	f << lvl.prevEntryPt;
	f.WriteCasted<byte>(lvl.prevEntryDir);
	f.WriteCasted<byte>(lvl.nextEntryType);
	f << lvl.nextEntryPt;
	f.WriteCasted<byte>(lvl.nextEntryDir);

	WritePortals(f);
}

//=================================================================================================
bool InsideLocation::Read(BitStreamReader& f)
{
	SetActiveLevel(game_level->dungeon_level);
	InsideLocationLevel& lvl = GetLevelData();
	game_level->lvl = &lvl;
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
	f.ReadCasted<byte>(lvl.prevEntryType);
	f >> lvl.prevEntryPt;
	f.ReadCasted<byte>(lvl.prevEntryDir);
	f.ReadCasted<byte>(lvl.nextEntryType);
	f >> lvl.nextEntryPt;
	f.ReadCasted<byte>(lvl.nextEntryDir);
	if(!f)
	{
		Error("Read level: Broken packet for entry.");
		return false;
	}

	// portals
	if(!ReadPortals(f, game_level->dungeon_level))
	{
		Error("Read level: Broken portals.");
		return false;
	}

	return true;
}

//=================================================================================================
bool InsideLocation::RemoveQuestItemFromChest(int quest_id, int& at_level)
{
	int index;
	Chest* chest = FindChestWithQuestItem(quest_id, at_level, &index);
	if(!chest)
		return false;
	else
	{
		if(!chest->GetUser())
			chest->RemoveItem(index);
		return true;
	}
}
