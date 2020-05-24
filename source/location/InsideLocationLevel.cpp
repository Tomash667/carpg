#include "Pch.h"
#include "InsideLocationLevel.h"

#include "GameFile.h"
#include "SaveState.h"

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
Room* InsideLocationLevel::FindEscapeRoom(const Vec3& my_pos, const Vec3& enemy_pos)
{
	Room* my_room = GetNearestRoom(my_pos),
		*enemy_room = GetNearestRoom(enemy_pos);

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
		dist = Vec3::Distance(my_pos, mid) - Vec3::Distance(enemy_pos, mid);
		if(dist < best_dist)
		{
			best_dist = dist;
			best_room = room;
		}
	}

	return best_room;
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

				if(IsBlocking(map[tile.x + (tile.y - 1)*w]) && !IsBlocking2(map[tile.x + tile.y*w]) && (nocol || !IsBlocking2(map[tile.x + (tile.y + 1)*w])))
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

				if(IsBlocking(map[tile.x + 1 + tile.y*w]) && !IsBlocking2(map[tile.x + tile.y*w]) && (nocol || !IsBlocking2(map[tile.x - 1 + tile.y*w])))
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

				if(IsBlocking(map[tile.x + (tile.y + 1)*w]) && !IsBlocking2(map[tile.x + tile.y*w]) && (nocol || !IsBlocking2(map[tile.x + (tile.y - 1)*w])))
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

				if(IsBlocking(map[tile.x - 1 + tile.y*w]) && !IsBlocking2(map[tile.x + tile.y*w]) && (nocol || !IsBlocking2(map[tile.x + 1 + tile.y*w])))
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
	f.Write(map, sizeof(Tile)*w*h);

	LevelArea::Save(f);

	// rooms
	f << rooms.size();
	for(Room* room : rooms)
		room->Save(f);

	// room groups
	f << groups.size();
	for(RoomGroup& group : groups)
		group.Save(f);

	f << staircase_up;
	f << staircase_down;
	f << staircase_up_dir;
	f << staircase_down_dir;
	f << staircase_down_in_wall;
}

//=================================================================================================
void InsideLocationLevel::LoadLevel(GameReader& f)
{
	f >> w;
	f >> h;
	map = new Tile[w*h];
	f.Read(map, sizeof(Tile)*w*h);

	if(LOAD_VERSION >= V_0_11)
	{
		LevelArea::Load(f);

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
		LevelArea::Load(f, old::LoadCompatibility::InsideLocationLevel);

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
		if(LOAD_VERSION >= V_0_8)
		{
			groups.resize(f.Read<uint>());
			for(RoomGroup& group : groups)
			{
				group.Load(f);
				group.index = index++;
			}
		}
		else
		{
			groups.resize(rooms.size());
			for(int i = 0; i < (int)rooms.size(); ++i)
			{
				groups[i].rooms.push_back(i);
				groups[i].target = rooms[i]->target;
				groups[i].index++;
			}
		}
		RoomGroup::SetRoomGroupConnections(groups, rooms);

		LevelArea::Load(f, old::LoadCompatibility::InsideLocationLevelTraps);
	}

	f >> staircase_up;
	f >> staircase_down;
	f >> staircase_up_dir;
	f >> staircase_down_dir;
	f >> staircase_down_in_wall;
}

//=================================================================================================
// If no_target is true it will ignore rooms that have target
Room& InsideLocationLevel::GetFarRoom(bool have_down_stairs, bool no_target)
{
	if(have_down_stairs)
	{
		Room* up_stairs = GetNearestRoom(Vec3(2.f*staircase_up.x + 1, 0, 2.f*staircase_up.y + 1));
		Room* down_stairs = GetNearestRoom(Vec3(2.f*staircase_down.x + 1, 0, 2.f*staircase_down.y + 1));
		int best_dist, dist;
		Room* best = nullptr;

		for(Room* room : rooms)
		{
			if(room->IsCorridor() || (no_target && room->target != RoomTarget::None))
				continue;
			dist = Int2::Distance(room->pos, up_stairs->pos) + Int2::Distance(room->pos, down_stairs->pos);
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
		Room* up_stairs = GetNearestRoom(Vec3(2.f*staircase_up.x + 1, 0, 2.f*staircase_up.y + 1));
		int best_dist, dist;
		Room* best = nullptr;

		for(Room* room : rooms)
		{
			if(room->IsCorridor() || (no_target && room->target != RoomTarget::None))
				continue;
			dist = Int2::Distance(room->pos, up_stairs->pos);
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

	return map[pt.x - 1 + pt.y*w].IsWall()
		|| map[pt.x + 1 + pt.y*w].IsWall()
		|| map[pt.x + (pt.y - 1)*w].IsWall()
		|| map[pt.x + (pt.y + 1)*w].IsWall();
}

//=================================================================================================
bool InsideLocationLevel::IsTileNearWall(const Int2& pt, int& dir) const
{
	assert(pt.x > 0 && pt.y > 0 && pt.x < w - 1 && pt.y < h - 1);

	int kierunek = 0;

	if(map[pt.x - 1 + pt.y*w].IsWall())
		kierunek |= (1 << GDIR_LEFT);
	if(map[pt.x + 1 + pt.y*w].IsWall())
		kierunek |= (1 << GDIR_RIGHT);
	if(map[pt.x + (pt.y - 1)*w].IsWall())
		kierunek |= (1 << GDIR_DOWN);
	if(map[pt.x + (pt.y + 1)*w].IsWall())
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
Room& InsideLocationLevel::GetRoom(RoomTarget target, bool down_stairs)
{
	if(target == RoomTarget::None)
		return GetFarRoom(down_stairs, true);
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
		if(c.group_index == group2->index)
			return std::make_pair(rooms[c.my_room], rooms[c.other_room]);
	}

	return pair<Room*, Room*>(nullptr, nullptr);
}
