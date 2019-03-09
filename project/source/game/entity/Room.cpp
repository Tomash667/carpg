#include "Pch.h"
#include "GameCore.h"
#include "Room.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

const float Room::HEIGHT = 4.f;
const float Room::HEIGHT_LOW = 3.f;

//=================================================================================================
bool Room::IsConnected(Room* room)
{
	for(Room* room2 : connected)
	{
		if(room == room2)
			return true;
	}
	return false;
}

//=================================================================================================
void Room::Save(FileWriter& f)
{
	f << pos;
	f << size;
	f << connected.size();
	for(Room* room : connected)
		f << room->index;
	f << target;
}

//=================================================================================================
void Room::Load(FileReader& f)
{
	f >> pos;
	f >> size;
	connected.resize(f.Read<uint>());
	for(Room*& room : connected)
		room = (Room*)f.Read<int>();
	if(LOAD_VERSION >= V_0_5)
		f >> target;
	else
	{
		int old_target;
		bool corridor;
		f >> old_target;
		f >> corridor;
		if(old_target == 0)
			target = (corridor ? RoomTarget::Corridor : RoomTarget::None);
		else
			target = (RoomTarget)(old_target + 1);
	}
}

//=================================================================================================
void Room::Write(BitStreamWriter& f) const
{
	f << pos;
	f << size;
	f.WriteCasted<byte>(connected.size());
	for(Room* room : connected)
		f.WriteCasted<byte>(room->index);
	f.WriteCasted<byte>(target);
}

//=================================================================================================
void Room::Read(BitStreamReader& f)
{
	f >> pos;
	f >> size;
	byte count = f.Read<byte>();
	connected.resize(count);
	for(Room*& room : connected)
		room = (Room*)(int)f.Read<byte>();
	f.ReadCasted<byte>(target);
}
