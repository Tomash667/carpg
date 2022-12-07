#include "Pch.h"
#include "InsideLocationLevel.h"

//=================================================================================================
InsideLocationLevel::~InsideLocationLevel()
{
	delete[] map;
	Room::Free(rooms);
}

//=================================================================================================
Room* InsideLocationLevel::GetNearestRoom(const Vec3& pos)
{
	if(rooms.empty())
		return nullptr;

	float best_dist = 1000.f;
	Room* best_room = nullptr;

	for(Room* room : rooms)
	{
		float dist = room->Distance(pos);
		if(dist < best_dist)
		{
			if(dist == 0.f)
				return room;
			best_dist = dist;
			best_room = room;
		}
	}

	return best_room;
}

//=================================================================================================
Room* InsideLocationLevel::FindEscapeRoom(const Vec3& myPos, const Vec3& enemyPos)
{
	Room* my_room = GetNearestRoom(myPos),
		*enemy_room = GetNearestRoom(enemyPos);

	if(!my_room)
		return nullptr;

	Room* best_room = nullptr;
	float best_dist = 0.f, dist;
	Vec3 mid;

	for(Room* room : my_room->connected)
	{
		if(room == enemy_room)
			continue;

		Vec3 mid = room->Center();
		dist = Vec3::Distance(myPos, mid) - Vec3::Distance(enemyPos, mid);
		if(dist < best_dist)
		{
			best_dist = dist;
			best_room = room;
		}
	}

	return best_room;
}

//=================================================================================================
Int2 InsideLocationLevel::GetPrevEntryFrontTile() const
{
	Int2 pt = prevEntryPt;
	if(prevEntryType != ENTRY_DOOR)
		pt += DirToPos(prevEntryDir);
	return pt;
}

//=================================================================================================
Int2 InsideLocationLevel::GetNextEntryFrontTile() const
{
	Int2 pt = nextEntryPt;
	if(nextEntryType != ENTRY_DOOR)
		pt += DirToPos(nextEntryDir);
	return pt;
}

//=================================================================================================
Room* InsideLocationLevel::GetRoom(const Int2& pt)
{
	Room* room = rooms[map[pt(w)].room];
	if(room->IsInside(pt))
		return room;
	return nullptr;
}

//=================================================================================================
Room* InsideLocationLevel::GetRandomRoom(RoomTarget target, delegate<bool(Room&)> clbk, int* out_index, int* out_group)
{
	int group_index = Rand() % groups.size(),
		group_start = group_index;
	while(true)
	{
		RoomGroup& group = groups[group_index];
		if(group.target == target)
		{
			int index = Rand() % group.rooms.size(),
				start = index;
			while(true)
			{
				int room_index = group.rooms[index];
				Room& room = *rooms[room_index];
				if(clbk(room))
				{
					if(out_index)
						*out_index = room_index;
					if(out_group)
						*out_group = group_index;
					return &room;
				}
				index = (index + 1) % group.rooms.size();
				if(index == start)
					break;
			}
		}
		group_index = (group_index + 1) % groups.size();
		if(group_index == group_start)
			return nullptr;
	}
}

//=================================================================================================
bool InsideLocationLevel::GetRandomNearWallTile(const Room& room, Int2& tile, GameDirection& rot, bool nocol)
{
	rot = (GameDirection)(Rand() % 4);

	int tries = 0;

	do
	{
		int tries2 = 10;

		switch(rot)
		{
		case GDIR_UP:
			// górna œciana, obj \/
			do
			{
				tile.x = Random(room.pos.x + 1, room.pos.x + room.size.x - 2);
				tile.y = room.pos.y + 1;

				if(IsBlocking(map[tile.x + (tile.y - 1) * w]) && !IsBlocking2(map[tile.x + tile.y * w]) && (nocol || !IsBlocking2(map[tile.x + (tile.y + 1) * w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		case GDIR_LEFT:
			// prawa œciana, obj <
			do
			{
				tile.x = room.pos.x + room.size.x - 2;
				tile.y = Random(room.pos.y + 1, room.pos.y + room.size.y - 2);

				if(IsBlocking(map[tile.x + 1 + tile.y * w]) && !IsBlocking2(map[tile.x + tile.y * w]) && (nocol || !IsBlocking2(map[tile.x - 1 + tile.y * w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		case GDIR_DOWN:
			// dolna œciana, obj /|
			do
			{
				tile.x = Random(room.pos.x + 1, room.pos.x + room.size.x - 2);
				tile.y = room.pos.y + room.size.y - 2;

				if(IsBlocking(map[tile.x + (tile.y + 1) * w]) && !IsBlocking2(map[tile.x + tile.y * w]) && (nocol || !IsBlocking2(map[tile.x + (tile.y - 1) * w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		case GDIR_RIGHT:
			// lewa œciana, obj >
			do
			{
				tile.x = room.pos.x + 1;
				tile.y = Random(room.pos.y + 1, room.pos.y + room.size.y - 2);

				if(IsBlocking(map[tile.x - 1 + tile.y * w]) && !IsBlocking2(map[tile.x + tile.y * w]) && (nocol || !IsBlocking2(map[tile.x + 1 + tile.y * w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		}

		++tries;
		rot = (GameDirection)((rot + 1) % 4);
	}
	while(tries <= 3);

	return false;
}

//=================================================================================================
void InsideLocationLevel::SaveLevel(GameWriter& f)
{
	f << w;
	f << h;
	f.Write(map, sizeof(Tile) * w * h);

	LocationPart::Save(f);

	// rooms
	f << rooms.size();
	for(Room* room : rooms)
		room->Save(f);

	// room groups
	f << groups.size();
	for(RoomGroup& group : groups)
		group.Save(f);

	f << prevEntryType;
	f << prevEntryPt;
	f << prevEntryDir;
	f << nextEntryType;
	f << nextEntryPt;
	f << nextEntryDir;
}

//=================================================================================================
void InsideLocationLevel::LoadLevel(GameReader& f)
{
	f >> w;
	f >> h;
	map = new Tile[w * h];
	f.Read(map, sizeof(Tile) * w * h);

	if(LOAD_VERSION >= V_0_11)
	{
		LocationPart::Load(f);

		// rooms
		rooms.resize(f.Read<uint>());
		int index = 0;
		for(Room*& room : rooms)
		{
			room = Room::Get();
			room->index = index++;
			room->Load(f);
		}
		for(Room* room : rooms)
		{
			for(Room*& c : room->connected)
				c = rooms[(int)c];
		}

		// room groups
		index = 0;
		groups.resize(f.Read<uint>());
		for(RoomGroup& group : groups)
		{
			group.Load(f);
			group.index = index++;
		}
	}
	else
	{
		LocationPart::Load(f, old::LoadCompatibility::InsideLocationLevel);

		// rooms
		rooms.resize(f.Read<uint>());
		int index = 0;
		for(Room*& room : rooms)
		{
			room = Room::Get();
			room->index = index++;
			room->Load(f);
		}
		for(Room* room : rooms)
		{
			for(Room*& c : room->connected)
				c = rooms[(int)c];
		}

		// room groups
		index = 0;
		groups.resize(f.Read<uint>());
		for(RoomGroup& group : groups)
		{
			group.Load(f);
			group.index = index++;
		}
		RoomGroup::SetRoomGroupConnections(groups, rooms);

		LocationPart::Load(f, old::LoadCompatibility::InsideLocationLevelTraps);
	}

	if(LOAD_VERSION >= V_0_16)
	{
		f >> prevEntryType;
		f >> prevEntryPt;
		f >> prevEntryDir;
		f >> nextEntryType;
		f >> nextEntryPt;
		f >> nextEntryDir;
	}
	else
	{
		prevEntryType = ENTRY_STAIRS_UP;
		nextEntryType = ENTRY_STAIRS_DOWN;
		f >> prevEntryPt;
		f >> nextEntryPt;
		f >> prevEntryDir;
		f >> nextEntryDir;
		if(f.Read<bool>()) // staircase_down_in_wall
			nextEntryType = ENTRY_STAIRS_DOWN_IN_WALL;
	}
}

//=================================================================================================
// If noTarget is true it will ignore rooms that have target
Room& InsideLocationLevel::GetFarRoom(bool haveNextEntry, bool noTarget)
{
	if(haveNextEntry)
	{
		Room* prevEntryRoom = GetPrevEntryRoom();
		Room* nextEntryRoom = GetNextEntryRoom();
		int best_dist, dist;
		Room* best = nullptr;

		for(Room* room : rooms)
		{
			if(room->IsCorridor() || (noTarget && room->target != RoomTarget::None))
				continue;
			dist = Int2::Distance(room->pos, prevEntryRoom->pos) + Int2::Distance(room->pos, nextEntryRoom->pos);
			if(!best || dist > best_dist)
			{
				best_dist = dist;
				best = room;
			}
		}

		return *best;
	}
	else
	{
		Room* prevEntryRoom = GetPrevEntryRoom();
		int best_dist, dist;
		Room* best = nullptr;

		for(Room* room : rooms)
		{
			if(room->IsCorridor() || (noTarget && room->target != RoomTarget::None))
				continue;
			dist = Int2::Distance(room->pos, prevEntryRoom->pos);
			if(!best || dist > best_dist)
			{
				best_dist = dist;
				best = room;
			}
		}

		return *best;
	}
}

//=================================================================================================
Door* InsideLocationLevel::FindDoor(const Int2& pt) const
{
	for(vector<Door*>::const_iterator it = doors.begin(), end = doors.end(); it != end; ++it)
	{
		if((*it)->pt == pt)
			return *it;
	}
	return nullptr;
}

//=================================================================================================
bool InsideLocationLevel::IsTileNearWall(const Int2& pt) const
{
	assert(pt.x > 0 && pt.y > 0 && pt.x < w - 1 && pt.y < h - 1);

	return map[pt.x - 1 + pt.y * w].IsWall()
		|| map[pt.x + 1 + pt.y * w].IsWall()
		|| map[pt.x + (pt.y - 1) * w].IsWall()
		|| map[pt.x + (pt.y + 1) * w].IsWall();
}

//=================================================================================================
bool InsideLocationLevel::IsTileNearWall(const Int2& pt, int& dir) const
{
	assert(pt.x > 0 && pt.y > 0 && pt.x < w - 1 && pt.y < h - 1);

	int kierunek = 0;

	if(map[pt.x - 1 + pt.y * w].IsWall())
		kierunek |= (1 << GDIR_LEFT);
	if(map[pt.x + 1 + pt.y * w].IsWall())
		kierunek |= (1 << GDIR_RIGHT);
	if(map[pt.x + (pt.y - 1) * w].IsWall())
		kierunek |= (1 << GDIR_DOWN);
	if(map[pt.x + (pt.y + 1) * w].IsWall())
		kierunek |= (1 << GDIR_UP);

	if(kierunek == 0)
		return false;

	int i = Rand() % 4;
	while(true)
	{
		if(IsSet(kierunek, 1 << i))
		{
			dir = i;
			return true;
		}
		i = (i + 1) % 4;
	}

	return true;
}

//=================================================================================================
Room& InsideLocationLevel::GetRoom(RoomTarget target, bool haveNextEntry)
{
	if(target == RoomTarget::None)
		return GetFarRoom(haveNextEntry, true);
	else
	{
		for(Room* room : rooms)
		{
			if(room->target == target)
				return *room;
		}

		assert(0);
		return *rooms[0];
	}
}

//=================================================================================================
int InsideLocationLevel::GetTileDirFlags(const Int2& pt)
{
	int flags = 0;
	if(pt.x == 0 || IsBlocking(map[pt.x - 1 + pt.y * w]))
		flags |= GDIRF_LEFT;
	if(pt.x == w - 1 || IsBlocking(map[pt.x + 1 + pt.y * w]))
		flags |= GDIRF_RIGHT;
	if(pt.y == 0 || IsBlocking(map[pt.x + (pt.y - 1) * w]))
		flags |= GDIRF_DOWN;
	if(pt.y == h - 1 || IsBlocking(map[pt.x + (pt.y + 1) * w]))
		flags |= GDIRF_UP;
	if(pt.x == 0 || pt.y == 0 || IsBlocking(map[pt.x - 1 + (pt.y - 1) * w]))
		flags |= GDIRF_LEFT_DOWN;
	if(pt.x == 0 || pt.y == h - 1 || IsBlocking(map[pt.x - 1 + (pt.y + 1) * w]))
		flags |= GDIRF_LEFT_UP;
	if(pt.x == w - 1 || pt.y == 0 || IsBlocking(map[pt.x + 1 + (pt.y - 1) * w]))
		flags |= GDIRF_RIGHT_DOWN;
	if(pt.x == w - 1 || pt.y == h - 1 || IsBlocking(map[pt.x + 1 + (pt.y + 1) * w]))
		flags |= GDIRF_RIGHT_UP;
	return flags;
}

//=================================================================================================
pair<Room*, Room*> InsideLocationLevel::GetConnectingRooms(RoomGroup* group1, RoomGroup* group2)
{
	assert(group1 && group2 && group1->IsConnected(group2->index));

	for(RoomGroup::Connection& c : group1->connections)
	{
		if(c.groupIndex == group2->index)
			return std::make_pair(rooms[c.myRoom], rooms[c.otherRoom]);
	}

	return pair<Room*, Room*>(nullptr, nullptr);
}
