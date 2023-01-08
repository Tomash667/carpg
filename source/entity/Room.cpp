#include "Pch.h"
#include "Room.h"

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
void Room::AddTile(const Int2& pt)
{
	if(pt.x < pos.x)
	{
		size.x += pos.x - pt.x;
		pos.x = pt.x;
	}
	else if(pt.x > pos.x + size.x)
		size.x = pt.x - pos.x;
	if(pt.y < pos.y)
	{
		size.y += pos.y - pt.y;
		pos.y = pt.y;
	}
	else if(pt.y > pos.y + size.y)
		size.y = pt.y - pos.y;
}

//=================================================================================================
void Room::Save(GameWriter& f)
{
	f << pos;
	f << size;
	f << connected.size();
	for(Room* room : connected)
		f << room->index;
	f << target;
	f << group;
}

//=================================================================================================
void Room::Load(GameReader& f)
{
	f >> pos;
	f >> size;
	connected.resize(f.Read<uint>());
	for(Room*& room : connected)
		room = reinterpret_cast<Room*>(f.Read<int>());
	f >> target;
	f >> group;
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
		room = reinterpret_cast<Room*>((int)f.Read<byte>());
	f.ReadCasted<byte>(target);
}

//=================================================================================================
bool RoomGroup::IsConnected(int groupIndex) const
{
	for(const Connection& c : connections)
	{
		if(c.groupIndex == groupIndex)
			return true;
	}
	return false;
}

//=================================================================================================
bool RoomGroup::HaveRoom(int roomIndex) const
{
	for(int index : rooms)
	{
		if(index == roomIndex)
			return true;
	}
	return false;
}

//=================================================================================================
void RoomGroup::Save(GameWriter& f)
{
	f << connections;
	f << rooms;
	f << target;
}

//=================================================================================================
void RoomGroup::Load(GameReader& f)
{
	f >> connections;
	f >> rooms;
	f >> target;
}

//=================================================================================================
void RoomGroup::SetRoomGroupConnections(vector<RoomGroup>& groups, vector<Room*>& rooms)
{
	for(RoomGroup& group : groups)
	{
		for(int roomIndex : group.rooms)
		{
			Room* room = rooms[roomIndex];
			for(Room* room2 : room->connected)
			{
				if(!group.HaveRoom(room2->index))
					group.connections.push_back({ room2->group, roomIndex, room2->index });
			}
		}
	}
}
