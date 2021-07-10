#include "Pch.h"
#include "Level.h"

#include "Room.h"

#include <File.h>

Level::~Level()
{
	DeleteElements(rooms);
}

void Level::Save(FileWriter& f)
{
	f.WriteStringF("LVL");
	f << (byte)0; // ver
	f << rooms.size();
	for(Room* room : rooms)
		f << room->box;
}

bool Level::Load(FileReader& f)
{
	char sign[3];
	byte ver;
	f.Read(sign, 3);
	f >> ver;
	if(!f || strncmp("LVL", sign, 3) != 0 || ver != 0)
		return false;
	uint count;
	f >> count;
	if(!f.Ensure(count * sizeof(Box)))
		return false;
	rooms.resize(count);
	for(Room*& room : rooms)
	{
		room = new Room;
		f >> room->box;
	}
	return true;
}
