#include "Pch.h"
#include "DungeonMapGenerator.h"

#include "GameCommon.h"

#define H(_x,_y) map[(_x)+(_y)*map_w].type
#define HR(_x,_y) map_rooms[(_x)+(_y)*map_w]

//=================================================================================================
void DungeonMapGenerator::Generate(MapSettings& settings, bool recreate)
{
	assert(settings.map_w && settings.map_h && settings.room_size.x >= 4 && settings.room_size.y >= settings.room_size.x
		&& InRange(settings.corridor_chance, 0, 100) && settings.rooms);
	assert(settings.corridor_chance == 0 || (settings.corridor_size.x >= 3 && settings.corridor_size.y >= settings.corridor_size.x));

	map_w = settings.map_w;
	map_h = settings.map_h;
	if(!recreate)
		map = new Tile[map_w * map_h];
	this->settings = &settings;
	empty.clear();
	joined.clear();

	SetLayout();
	GenerateRooms();
	if(settings.remove_dead_end_corridors)
		RemoveDeadEndCorridors();
	SetRoomIndices();
	if(settings.corridor_join_chance > 0)
		JoinCorridors();
	else
		MarkCorridors();
	AddHoles();

	if(settings.stop)
		return;

	Finalize();
}

//=================================================================================================
void DungeonMapGenerator::SetLayout()
{
	memset(map, 0, sizeof(Tile)*map_w*map_h);
	map_rooms.resize(map_w*map_h);
	memset(map_rooms.data(), 0, sizeof(Room*)*map_w*map_h);

	if(settings->shape == MapSettings::SHAPE_SQUARE)
	{
		for(int x = 0; x < map_w; ++x)
		{
			H(x, 0) = BLOCKADE;
			H(x, map_h - 1) = BLOCKADE;
		}

		for(int y = 0; y < map_h; ++y)
		{
			H(0, y) = BLOCKADE;
			H(map_w - 1, y) = BLOCKADE;
		}
	}
	else
	{
		int w = (map_w - 3) / 2,
			h = (map_h - 3) / 2;

		for(int y = 0; y < map_h; ++y)
		{
			for(int x = 0; x < map_w; ++x)
			{
				if(Distance(float(x - 1) / w, float(y - 1) / h, 1.f, 1.f) > 1.f)
					map[x + y * map_w].type = BLOCKADE;
			}
		}
	}

	// add existing/start rooms
	if(!settings->rooms->empty())
	{
		for(Room* room : *settings->rooms)
			SetRoom(room);
	}
	else
	{
		int w = settings->room_size.Random(),
			h = settings->room_size.Random();
		AddRoom((map_w - w) / 2, (map_h - h) / 2, w, h, false, nullptr);
	}
}

//=================================================================================================
void DungeonMapGenerator::GenerateRooms()
{
	while(!empty.empty())
	{
		int index = Rand() % empty.size();
		pair<int, Room*> entry = empty[index];
		if(index != empty.size() - 1)
			std::iter_swap(empty.begin() + index, empty.end() - 1);
		empty.pop_back();

		GameDirection dir = CheckFreePath(entry.first);
		if(dir == GDIR_INVALID)
			continue;

		Int2 pt(entry.first % map_w, entry.first / map_w);
		if(Rand() % 100 < settings->corridor_chance)
			AddCorridor(pt, dir, entry.second);
		else
		{
			int w = settings->room_size.Random(),
				h = settings->room_size.Random();

			if(dir == GDIR_LEFT)
			{
				pt.x -= w - 1;
				pt.y -= h / 2;
			}
			else if(dir == GDIR_RIGHT)
				pt.y -= h / 2;
			else if(dir == GDIR_UP)
				pt.x -= w / 2;
			else
			{
				pt.x -= w / 2;
				pt.y -= h - 1;
			}

			if(CheckRoom(pt.x, pt.y, w, h))
			{
				map[entry.first].type = DOORS;
				AddRoom(pt.x, pt.y, w, h, false, entry.second);
			}
		}
	}
}

//=================================================================================================
GameDirection DungeonMapGenerator::CheckFreePath(int id)
{
	int x = id % map_w,
		y = id / map_w;
	if(x == 0 || y == 0 || x == map_w - 1 || y == map_h - 1)
		return GDIR_INVALID;

	GameDirection dir = GDIR_INVALID;
	int count = 0;

	if(map[id - 1].type == EMPTY)
	{
		++count;
		dir = GDIR_LEFT;
	}
	if(map[id + 1].type == EMPTY)
	{
		++count;
		dir = GDIR_RIGHT;
	}
	if(map[id + map_w].type == EMPTY)
	{
		++count;
		dir = GDIR_UP;
	}
	if(map[id - map_w].type == EMPTY)
	{
		++count;
		dir = GDIR_DOWN;
	}

	if(count != 1)
		return GDIR_INVALID;

	dir = Reversed(dir);
	const Int2& od = DirToPos(dir);
	if(map[id + od.x + od.y*map_w].type != UNUSED)
		return GDIR_INVALID;

	return dir;
}

//=================================================================================================
void DungeonMapGenerator::AddRoom(int x, int y, int w, int h, bool is_corridor, Room* parent)
{
	assert(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < map_w && y + h < map_h);

	Room* room = Room::Get();
	room->pos.x = x;
	room->pos.y = y;
	room->size.x = w;
	room->size.y = h;
	room->target = (is_corridor ? RoomTarget::Corridor : RoomTarget::None);
	room->type = nullptr;
	room->connected.clear();
	settings->rooms->push_back(room);

	if(parent)
	{
		parent->connected.push_back(room);
		room->connected.push_back(parent);
	}

	SetRoom(room);
}

//=================================================================================================
void DungeonMapGenerator::SetRoom(Room* room)
{
	int x = room->pos.x,
		y = room->pos.y,
		w = room->size.x,
		h = room->size.y;

	for(int yy = y + 1; yy < y + h - 1; ++yy)
	{
		for(int xx = x + 1; xx < x + w - 1; ++xx)
		{
			assert(H(xx, yy) == UNUSED);
			H(xx, yy) = EMPTY;
			HR(xx, yy) = room;
		}
	}

	for(int i = 0; i < w; ++i)
	{
		SetWall(H(x + i, y));
		HR(x + i, y) = room;
		SetWall(H(x + i, y + h - 1));
		HR(x + i, y + h - 1) = room;
	}

	for(int i = 0; i < h; ++i)
	{
		SetWall(H(x, y + i));
		HR(x, y + i) = room;
		SetWall(H(x + w - 1, y + i));
		HR(x + w - 1, y + i) = room;
	}

	for(int i = 1; i < w - 1; ++i)
	{
		if(CheckFreePath(x + i + y * map_w) != GDIR_INVALID)
			empty.push_back({ x + i + y * map_w, room });
		if(CheckFreePath(x + i + (y + h - 1)*map_w) != GDIR_INVALID)
			empty.push_back({ x + i + (y + h - 1)*map_w, room });
	}
	for(int i = 1; i < h - 1; ++i)
	{
		if(CheckFreePath(x + (y + i)*map_w) != GDIR_INVALID)
			empty.push_back({ x + (y + i)*map_w, room });
		if(CheckFreePath(x + w - 1 + (y + i)*map_w) != GDIR_INVALID)
			empty.push_back({ x + w - 1 + (y + i)*map_w, room });
	}
}

//=================================================================================================
void DungeonMapGenerator::AddCorridor(Int2& pt, GameDirection dir, Room* parent)
{
	int len = settings->corridor_size.Random();
	int w, h;
	Int2 pos(pt);

	if(dir == GDIR_LEFT)
	{
		pos.x -= len - 1;
		pos.y -= 1;
		w = len;
		h = 3;
	}
	else if(dir == GDIR_RIGHT)
	{
		pos.y -= 1;
		w = len;
		h = 3;
	}
	else if(dir == GDIR_UP)
	{
		pos.x -= 1;
		w = 3;
		h = len;
	}
	else
	{
		pos.y -= len - 1;
		pos.x -= 1;
		w = 3;
		h = len;
	}

	if(CheckRoom(pos.x, pos.y, w, h))
	{
		H(pt.x, pt.y) = DOORS;
		AddRoom(pos.x, pos.y, w, h, true, parent);
	}
}

//=================================================================================================
bool DungeonMapGenerator::CheckRoom(int x, int y, int w, int h)
{
	if(!(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < map_w && y + h < map_h))
		return false;

	for(int yy = y + 1; yy < y + h - 1; ++yy)
	{
		for(int xx = x + 1; xx < x + w - 1; ++xx)
		{
			if(H(xx, yy) != UNUSED)
				return false;
		}
	}

	for(int i = 0; i < w; ++i)
	{
		TILE_TYPE p = H(x + i, y);
		if(p != UNUSED && p != WALL && p != BLOCKADE && p != BLOCKADE_WALL)
			return false;
		p = H(x + i, y + h - 1);
		if(p != UNUSED && p != WALL && p != BLOCKADE && p != BLOCKADE_WALL)
			return false;
	}

	for(int i = 0; i < h; ++i)
	{
		TILE_TYPE p = H(x, y + i);
		if(p != UNUSED && p != WALL && p != BLOCKADE && p != BLOCKADE_WALL)
			return false;
		p = H(x + w - 1, y + i);
		if(p != UNUSED && p != WALL && p != BLOCKADE && p != BLOCKADE_WALL)
			return false;
	}

	return true;
}

//=================================================================================================
void DungeonMapGenerator::SetWall(TILE_TYPE& tile)
{
	assert(tile == UNUSED || tile == WALL || tile == BLOCKADE || tile == BLOCKADE_WALL || tile == DOORS);

	if(tile == UNUSED)
		tile = WALL;
	else if(tile == BLOCKADE)
		tile = BLOCKADE_WALL;
}

//=================================================================================================
void DungeonMapGenerator::RemoveDeadEndCorridors()
{
	LocalVector<Room*> to_remove;
	for(Room* room : *settings->rooms)
	{
		if(room->IsCorridor() && room->connected.size() == 1u)
			to_remove->push_back(room);
	}

	while(!to_remove->empty())
	{
		Room* room = to_remove->back();
		to_remove->pop_back();

		int x = room->pos.x,
			y = room->pos.y,
			w = room->size.x,
			h = room->size.y;

		for(int yy = y + 1; yy < y + h - 1; ++yy)
		{
			for(int xx = x + 1; xx < x + w - 1; ++xx)
			{
				H(xx, yy) = UNUSED;
				HR(xx, yy) = nullptr;
			}
		}

		for(int i = 0; i < w; ++i)
		{
			RemoveWall(x + i, y, room);
			RemoveWall(x + i, y + h - 1, room);
		}

		for(int i = 0; i < h; ++i)
		{
			RemoveWall(x, y + i, room);
			RemoveWall(x + w - 1, y + i, room);
		}

		RemoveElement(*settings->rooms, room);
		Room* connected = room->connected.front();
		RemoveElement(connected->connected, room);
		if(connected->IsCorridor() && connected->connected.size() == 1u)
			to_remove->push_back(connected);
		room->Free();
	}
}

//=================================================================================================
void DungeonMapGenerator::RemoveWall(int x, int y, Room* room)
{
	TILE_TYPE& tile = H(x, y);
	if(tile == DOORS)
		tile = WALL;
	else if(tile == WALL || tile == BLOCKADE_WALL)
	{
		bool ok = true;
		if(x > 0)
		{
			if(H(x - 1, y) == EMPTY)
				ok = false;
			if(y > 0 && H(x - 1, y - 1) == EMPTY)
				ok = false;
			if(y < map_h - 1 && H(x - 1, y + 1) == EMPTY)
				ok = false;
		}
		if(x < map_w - 1)
		{
			if(H(x + 1, y) == EMPTY)
				ok = false;
			if(y > 0 && H(x + 1, y - 1) == EMPTY)
				ok = false;
			if(y < map_h - 1 && H(x + 1, y + 1) == EMPTY)
				ok = false;
		}
		if(y > 0 && H(x, y - 1) == EMPTY)
			ok = false;
		if(y < map_h - 1 && H(x, y + 1) == EMPTY)
			ok = false;

		if(ok)
		{
			if(tile == BLOCKADE_WALL)
				tile = BLOCKADE;
			else
				tile = UNUSED;
		}
	}

	Room*& tile_room = HR(x, y);
	if(tile_room == room)
		tile_room = nullptr;
}

//=================================================================================================
void DungeonMapGenerator::SetRoomIndices()
{
	for(int i = 0, count = (int)settings->rooms->size(); i < count; ++i)
		settings->rooms->at(i)->index = i;

	for(int i = 0; i < map_w*map_h; ++i)
	{
		Room* room = map_rooms[i];
		map[i].room = (room ? room->index : 0);
	}
}

//=================================================================================================
void DungeonMapGenerator::MarkCorridors()
{
	for(Room* room : *settings->rooms)
	{
		if(!room->IsCorridor())
			continue;

		for(int y = 0; y < room->size.y; ++y)
		{
			for(int x = 0; x < room->size.x; ++x)
			{
				Tile& p = map[x + room->pos.x + (y + room->pos.y)*map_w];
				if(p.type == EMPTY || p.type == DOORS || p.type == BARS_FLOOR)
					p.flags = Tile::F_LOW_CEILING;
			}
		}
	}
}

//=================================================================================================
void DungeonMapGenerator::JoinCorridors()
{
	for(Room* room : *settings->rooms)
	{
		if(!room->IsCorridor())
			continue;

		for(Room* room2 : room->connected)
		{
			if(!room2->IsCorridor() || Rand() % 100 >= settings->corridor_join_chance)
				continue;

			int x1 = max(room->pos.x, room2->pos.x),
				x2 = min(room->pos.x + room->size.x, room2->pos.x + room2->size.x),
				y1 = max(room->pos.y, room2->pos.y),
				y2 = min(room->pos.y + room->size.y, room2->pos.y + room2->size.y);

			assert(x1 < x2 && y1 < y2);
			for(int y = y1; y < y2; ++y)
			{
				for(int x = x1; x < x2; ++x)
				{
					int index = x + y * map_w;
					Tile& tile = map[index];
					if(tile.type == DOORS)
					{
						assert(map_rooms[index] == room || map_rooms[index] == room2);
						tile.type = EMPTY;
						goto removed_doors;
					}
				}
			}
		removed_doors:
			;
		}

		// mark low celling
		for(int y = 0; y < room->size.y; ++y)
		{
			for(int x = 0; x < room->size.x; ++x)
			{
				Tile& p = map[x + room->pos.x + (y + room->pos.y)*map_w];
				if(p.type == EMPTY || p.type == DOORS || p.type == BARS_FLOOR)
					p.flags = Tile::F_LOW_CEILING;
			}
		}
	}
}

//=================================================================================================
void DungeonMapGenerator::AddHoles()
{
	if(settings->bars_chance <= 0)
		return;
	for(int y = 1; y < map_w - 1; ++y)
	{
		for(int x = 1; x < map_h - 1; ++x)
		{
			Tile& tile = map[x + y * map_w];
			if(tile.type == EMPTY && Rand() % 100 < settings->bars_chance)
			{
				if(!IsSet(tile.flags, Tile::F_LOW_CEILING))
				{
					int j = Rand() % 3;
					if(j == 0)
						tile.type = BARS_FLOOR;
					else if(j == 1)
						tile.type = BARS_CEILING;
					else
						tile.type = BARS;
				}
				else if(Rand() % 3 == 0)
					tile.type = BARS_FLOOR;
			}
		}
	}
}

//=================================================================================================
void DungeonMapGenerator::Finalize()
{
	if(settings->room_join_chance > 0)
		JoinRooms();

	if(settings->groups)
	{
		CreateRoomGroups();
		RoomGroup::SetRoomGroupConnections(*settings->groups, *settings->rooms);
	}

	if(settings->prevEntryLoc != MapSettings::ENTRY_NONE || settings->nextEntryLoc != MapSettings::ENTRY_NONE)
		GenerateEntry();

	SetRoomGroupTargets();
	Tile::SetupFlags(map, Int2(map_w, map_h), settings->prevEntryType, settings->nextEntryType);

	if(settings->devmode)
		Tile::DebugDraw(map, Int2(map_w, map_h));
}

//=================================================================================================
void DungeonMapGenerator::JoinRooms()
{
	for(Room* room : *settings->rooms)
	{
		if(!room->CanJoinRoom())
			continue;

		for(Room* room2 : room->connected)
		{
			if(!room2->CanJoinRoom() || Rand() % 100 >= settings->room_join_chance)
				continue;

			joined.push_back({ room, room2 });

			// find common area
			int x1 = max(room->pos.x, room2->pos.x),
				x2 = min(room->pos.x + room->size.x, room2->pos.x + room2->size.x),
				y1 = max(room->pos.y, room2->pos.y),
				y2 = min(room->pos.y + room->size.y, room2->pos.y + room2->size.y);

			if(x1 == 0)
				++x1;
			if(y1 == 0)
				++y1;
			if(x2 == map_w - 1)
				--x2;
			if(y2 == map_h - 1)
				--y2;
			assert(x1 < x2 && y1 < y2);

			for(int y = y1; y < y2; ++y)
			{
				for(int x = x1; x < x2; ++x)
				{
					if(IsConnectingWall(x, y, room, room2))
					{
						Tile& tile = map[x + y * map_w];
						if(tile.type == WALL || tile.type == DOORS)
							tile.type = EMPTY;
					}
				}
			}
		}
	}
}

//=================================================================================================
bool DungeonMapGenerator::IsConnectingWall(int x, int y, Room* room1, Room* room2)
{
	if(map_rooms[x - 1 + y * map_w] == room1 && IsEmpty(map[x - 1 + y * map_w].type))
	{
		if(map_rooms[x + 1 + y * map_w] == room2 && IsEmpty(map[x + 1 + y * map_w].type))
			return true;
	}
	else if(map_rooms[x - 1 + y * map_w] == room2 && IsEmpty(map[x - 1 + y * map_w].type))
	{
		if(map_rooms[x + 1 + y * map_w] == room1 && IsEmpty(map[x + 1 + y * map_w].type))
			return true;
	}
	if(map_rooms[x + (y - 1)*map_w] == room1 && IsEmpty(map[x + (y - 1)*map_w].type))
	{
		if(map_rooms[x + (y + 1)*map_w] == room2 && IsEmpty(map[x + (y + 1)*map_w].type))
			return true;
	}
	else if(map_rooms[x + (y - 1)*map_w] == room2 && IsEmpty(map[x + (y - 1)*map_w].type))
	{
		if(map_rooms[x + (y + 1)*map_w] == room1 && IsEmpty(map[x + (y + 1)*map_w].type))
			return true;
	}
	return false;
}

//=================================================================================================
void DungeonMapGenerator::CreateRoomGroups()
{
	if(joined.empty())
	{
		settings->groups->resize(settings->rooms->size());
		for(int i = 0; i < (int)settings->rooms->size(); ++i)
		{
			RoomGroup& group = settings->groups->at(i);
			group.rooms.push_back(i);
			group.index = i;

			settings->rooms->at(i)->group = i;
		}
		return;
	}

	// DFS
	vector<bool> visited(settings->rooms->size(), false);
	std::function<void(RoomGroup&, Room* room)> c = [&](RoomGroup& group, Room* room)
	{
		visited[room->index] = true;
		group.rooms.push_back(room->index);
		room->group = group.index;
		for(Room* room2 : room->connected)
		{
			if(!visited[room2->index])
			{
				for(pair<Room*, Room*>& join : joined)
				{
					if((join.first == room && join.second == room2) || (join.first == room2 && join.second == room))
					{
						c(group, room2);
						break;
					}
				}
			}
		}
	};

	int index = 0;
	for(Room* room : *settings->rooms)
	{
		if(!visited[room->index])
		{
			RoomGroup& group = Add1(settings->groups);
			group.index = index++;
			c(group, room);
		}
	}

	//if(settings->devmode)
	//	DrawRoomGroups();
}

//=================================================================================================
void DungeonMapGenerator::DrawRoomGroups()
{
	char* ce = new char[map_w*map_h];
	memset(ce, ' ', sizeof(char)*map_w*map_h);
	for(int i = 0; i < (int)settings->groups->size(); i++)
	{
		int tmp = i % 36;
		char s;
		if(tmp < 10)
			s = '0' + tmp;
		else
			s = 'A' + (tmp - 10);
		RoomGroup& group = settings->groups->at(i);
		for(int j : group.rooms)
		{
			Room* room = settings->rooms->at(j);
			for(int x = 0; x < room->size.x; ++x)
			{
				for(int y = 0; y < room->size.y; ++y)
					ce[x + room->pos.x + (y + room->pos.y)*map_w] = s;
			}
		}
	}
	for(int y = map_h - 1; y >= 0; --y)
	{
		for(int x = 0; x < map_w; ++x)
			putchar(ce[x + y * map_w]);
		putchar('\n');
	}
	delete[] ce;
}

//=================================================================================================
void DungeonMapGenerator::GenerateEntry()
{
	map_rooms.clear();
	for(Room* room : *settings->rooms)
	{
		if(room->target == RoomTarget::None)
			map_rooms.push_back(room);
	}

	if(settings->prevEntryLoc != MapSettings::ENTRY_NONE)
	{
		GenerateEntry(map_rooms, settings->prevEntryLoc, settings->prevEntryType, settings->prevEntryRoom,
			settings->prevEntryPt, settings->prevEntryDir, true);
	}

	if(settings->nextEntryLoc != MapSettings::ENTRY_NONE)
	{
		GenerateEntry(map_rooms, settings->nextEntryLoc, settings->nextEntryType, settings->nextEntryRoom,
			settings->nextEntryPt, settings->nextEntryDir, false);
	}

	map_rooms.clear();
}

//=================================================================================================
void DungeonMapGenerator::GenerateEntry(vector<Room*>& rooms, MapSettings::EntryLocation loc, EntryType& type,
	Room*& room, Int2& pt, GameDirection& dir, bool isPrev)
{
	switch(loc)
	{
	case MapSettings::ENTRY_RANDOM:
		while(!rooms.empty())
		{
			uint id = Rand() % rooms.size();
			room = rooms.at(id);
			if(id != rooms.size() - 1)
				std::iter_swap(rooms.begin() + id, rooms.end() - 1);
			rooms.pop_back();

			if(AddEntry(*room, type, pt, dir, isPrev))
				return;
		}
		break;
	case MapSettings::ENTRY_FAR_FROM_ROOM:
		{
			const Int2 farPt = rooms[0]->CenterTile();
			if(AddEntryFarFromPoint(rooms, farPt, room, type, pt, dir, isPrev))
				return;
		}
		break;
	case MapSettings::ENTRY_BORDER:
		{
			const Int2 farPt = Int2(map_w / 2, map_h / 2);
			if(AddEntryFarFromPoint(rooms, farPt, room, type, pt, dir, isPrev))
				return;
		}
		break;
	case MapSettings::ENTRY_FAR_FROM_PREV:
		{
			int index = 0;
			for(int i = 0, count = (int)map_rooms.size(); i < count; ++i)
			{
				if(map_rooms[i]->target == RoomTarget::EntryPrev)
				{
					index = i;
					break;
				}
			}

			const Int2 farPt = map_rooms[index]->CenterTile();
			if(AddEntryFarFromPoint(rooms, farPt, room, type, pt, dir, isPrev))
				return;
		}
		break;
	}

	throw "Failed to generate dungeon entry point.";
}

//=================================================================================================
bool DungeonMapGenerator::AddEntryFarFromPoint(vector<Room*>& rooms, const Int2& farPt, Room*& room, EntryType& type,
	Int2& pt, GameDirection& dir, bool isPrev)
{
	vector<Int2> p;
	int index = 0;
	for(vector<Room*>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it, ++index)
		p.push_back(Int2(index, Int2::Distance(farPt, (*it)->CenterTile())));
	std::sort(p.begin(), p.end(), [](Int2& a, Int2& b) { return a.y < b.y; });

	while(!p.empty())
	{
		int p_id = p.back().x;
		p.pop_back();
		room = rooms[p_id];
		if(AddEntry(*room, type, pt, dir, isPrev))
			return true;
	}

	return false;
}

//=================================================================================================
bool DungeonMapGenerator::AddEntry(Room& room, EntryType& type, Int2& pt, GameDirection& dir, bool isPrev)
{
#define B(_xx,_yy) (IsBlocking(map[x+_xx+(y+_yy)*map_w].type))
#define P(_xx,_yy) (!IsBlocking2(map[x+_xx+(y+_yy)*map_w].type))

	struct PosDir
	{
		Int2 pt;
		int dir, prio;
		bool in_wall;

		PosDir(int x, int y, int dir, bool in_wall, const Room& room) : pt(x, y), dir(dir), prio(0), in_wall(in_wall)
		{
			if(in_wall)
				prio += (room.size.x + room.size.y) * 2;
			x -= room.pos.x;
			y -= room.pos.y;
			prio -= abs(room.size.x / 2 - x) + abs(room.size.y / 2 - y);
		}

		bool operator < (const PosDir& p)
		{
			return prio > p.prio;
		}
	};

	static vector<PosDir> choice;
	choice.clear();

	const bool isDoor = (type == ENTRY_DOOR);

	for(int y = max(1, room.pos.y); y < min(map_h - 1, room.size.y + room.pos.y); ++y)
	{
		for(int x = max(1, room.pos.x); x < min(map_w - 1, room.size.x + room.pos.x); ++x)
		{
			Tile& p = map[x + y * map_w];
			if(p.type == EMPTY)
			{
				const bool left = (x > 0);
				const bool right = (x<int(map_w - 1));
				const bool top = (y<int(map_h - 1));
				const bool bottom = (y > 0);

				if(!isDoor)
				{
					if(left && top)
					{
						// ##
						// #>
						if(B(-1, 1) && B(0, 1) && B(-1, 0))
							choice.push_back(PosDir(x, y, Bit(GDIR_DOWN) | Bit(GDIR_RIGHT), false, room));
					}
					if(right && top)
					{
						// ##
						// >#
						if(B(0, 1) && B(1, 1) && B(1, 0))
							choice.push_back(PosDir(x, y, Bit(GDIR_DOWN) | Bit(GDIR_LEFT), false, room));
					}
					if(left && bottom)
					{
						// #>
						// ##
						if(B(-1, 0) && B(-1, -1) && B(0, -1))
							choice.push_back(PosDir(x, y, Bit(GDIR_UP) | Bit(GDIR_RIGHT), false, room));
					}
					if(right && bottom)
					{
						// <#
						// ##
						if(B(1, 0) && B(0, -1) && B(1, -1))
							choice.push_back(PosDir(x, y, Bit(GDIR_LEFT) | Bit(GDIR_UP), false, room));
					}
					if(left && right && top && bottom)
					{
						//  ___
						//  _<_
						//  ___
						if(P(-1, -1) && P(0, -1) && P(1, -1) && P(-1, 0) && P(1, 0) && P(-1, 1) && P(0, 1) && P(1, 1))
							choice.push_back(PosDir(x, y, Bit(GDIR_DOWN) | Bit(GDIR_LEFT) | Bit(GDIR_UP) | Bit(GDIR_RIGHT), false, room));
					}
				}
				if(left && top && bottom)
				{
					// #_
					// #>
					// #_
					if(B(-1, 1) && P(0, 1) && B(-1, 0) && B(-1, -1) && P(0, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_RIGHT), false, room));
				}
				if(right && top && bottom)
				{
					// _#
					// <#
					// _#
					if(P(0, 1) && B(1, 1) && B(1, 0) && P(0, -1) && B(1, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_LEFT), false, room));
				}
				if(top && left && right)
				{
					// ###
					// _>_
					if(B(-1, 1) && B(0, 1) && B(1, 1) && P(-1, 0) && P(1, 0))
						choice.push_back(PosDir(x, y, Bit(GDIR_DOWN), false, room));
				}
				if(bottom && left && right)
				{
					// _>_
					// ###
					if(P(-1, 0) && P(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_UP), false, room));
				}
			}
			else if(!isDoor && (p.type == WALL || p.type == BLOCKADE_WALL) && (x > 0) && (x<int(map_w - 1)) && (y > 0) && (y<int(map_h - 1)))
			{
				// ##
				// #>_
				// ##
				if(B(-1, 1) && B(0, 1) && B(-1, 0) && P(1, 0) && B(-1, -1) && B(0, -1))
					choice.push_back(PosDir(x, y, Bit(GDIR_RIGHT), true, room));

				//  ##
				// _<#
				//  ##
				if(B(0, 1) && B(1, 1) && P(-1, 0) && B(1, 0) && B(0, -1) && B(1, -1))
					choice.push_back(PosDir(x, y, Bit(GDIR_LEFT), true, room));

				// ###
				// #>#
				//  _
				if(B(-1, 1) && B(0, 1) && B(1, 1) && B(-1, 0) && B(1, 0) && P(0, -1))
					choice.push_back(PosDir(x, y, Bit(GDIR_DOWN), true, room));

				//  _
				// #<#
				// ###
				if(P(0, 1) && B(-1, 0) && B(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1))
					choice.push_back(PosDir(x, y, Bit(GDIR_UP), true, room));
			}
		}
	}

	if(choice.empty())
		return false;

	std::sort(choice.begin(), choice.end());

	int best_prio = choice.front().prio;
	int count = 0;

	vector<PosDir>::iterator it = choice.begin(), end = choice.end();
	while(it != end && it->prio == best_prio)
	{
		++it;
		++count;
	}

	PosDir& pd = choice[Rand() % count];
	pt = pd.pt;
	map[pd.pt.x + pd.pt.y*map_w].type = (isPrev ? ENTRY_PREV : ENTRY_NEXT);
	map[pd.pt.x + pd.pt.y*map_w].room = room.index;
	room.target = (isPrev ? RoomTarget::EntryPrev : RoomTarget::EntryNext);
	room.AddTile(pd.pt);
	if(type == ENTRY_STAIRS_DOWN && pd.in_wall)
		type = ENTRY_STAIRS_DOWN_IN_WALL;

	for(int y = max(0, pd.pt.y - 1); y <= min(map_h, pd.pt.y + 1); ++y)
	{
		for(int x = max(0, pd.pt.x - 1); x <= min(map_w, pd.pt.x + 1); ++x)
		{
			TILE_TYPE& p = map[x + y * map_w].type;
			if(p == UNUSED)
				p = WALL;
			else if(p == BLOCKADE)
				p = BLOCKADE_WALL;
		}
	}

	switch(pd.dir)
	{
	case 0:
		assert(0);
		return false;
	case Bit(GDIR_DOWN):
		dir = GDIR_DOWN;
		break;
	case Bit(GDIR_LEFT):
		dir = GDIR_LEFT;
		break;
	case Bit(GDIR_UP):
		dir = GDIR_UP;
		break;
	case Bit(GDIR_RIGHT):
		dir = GDIR_RIGHT;
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_LEFT):
		if(Rand() % 2 == 0)
			dir = GDIR_DOWN;
		else
			dir = GDIR_LEFT;
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_UP):
		if(Rand() % 2 == 0)
			dir = GDIR_DOWN;
		else
			dir = GDIR_UP;
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_RIGHT):
		if(Rand() % 2 == 0)
			dir = GDIR_DOWN;
		else
			dir = GDIR_RIGHT;
		break;
	case Bit(GDIR_LEFT) | Bit(GDIR_UP):
		if(Rand() % 2 == 0)
			dir = GDIR_LEFT;
		else
			dir = GDIR_UP;
		break;
	case Bit(GDIR_LEFT) | Bit(GDIR_RIGHT):
		if(Rand() % 2 == 0)
			dir = GDIR_LEFT;
		else
			dir = GDIR_RIGHT;
		break;
	case Bit(GDIR_UP) | Bit(GDIR_RIGHT):
		if(Rand() % 2 == 0)
			dir = GDIR_UP;
		else
			dir = GDIR_RIGHT;
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_LEFT) | Bit(GDIR_UP):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = GDIR_DOWN;
			else if(t == 1)
				dir = GDIR_LEFT;
			else
				dir = GDIR_UP;
		}
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_LEFT) | Bit(GDIR_RIGHT):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = GDIR_DOWN;
			else if(t == 1)
				dir = GDIR_LEFT;
			else
				dir = GDIR_RIGHT;
		}
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_UP) | Bit(GDIR_RIGHT):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = GDIR_DOWN;
			else if(t == 1)
				dir = GDIR_UP;
			else
				dir = GDIR_RIGHT;
		}
		break;
	case Bit(GDIR_LEFT) | Bit(GDIR_UP) | Bit(GDIR_RIGHT):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = GDIR_LEFT;
			else if(t == 1)
				dir = GDIR_UP;
			else
				dir = GDIR_RIGHT;
		}
		break;
	case Bit(GDIR_DOWN) | Bit(GDIR_LEFT) | Bit(GDIR_UP) | Bit(GDIR_RIGHT):
		dir = (GameDirection)(Rand() % 4);
		break;
	default:
		assert(0);
		return false;
	}

	return true;

#undef B
#undef P
}

//=================================================================================================
void DungeonMapGenerator::SetRoomGroupTargets()
{
	if(!settings->groups)
		return;

	for(RoomGroup& group : *settings->groups)
	{
		group.target = RoomTarget::None;
		for(int j : group.rooms)
		{
			RoomTarget target = settings->rooms->at(j)->target;
			if(target != RoomTarget::None)
				group.target = target;
		}
	}
}

//=================================================================================================
// Returns tile marked with [?], if there is more then one returns first
// ###########
// #    #    #
// # p1 ? p2 #
// #    #    #
// ###########
Int2 DungeonMapGenerator::GetConnectingTile(Room* room1, Room* room2)
{
	assert(room1->IsConnected(room2));

	// find common area
	int x1 = max(room1->pos.x, room2->pos.x),
		x2 = min(room1->pos.x + room1->size.x, room2->pos.x + room2->size.x),
		y1 = max(room1->pos.y, room2->pos.y),
		y2 = min(room1->pos.y + room1->size.y, room2->pos.y + room2->size.y);
	assert(x1 < x2 && y1 < y2);

	for(int y = y1; y < y2; ++y)
	{
		for(int x = x1; x < x2; ++x)
		{
			Tile& tile = map[x + y * map_w];
			if(tile.type == EMPTY || tile.type == DOORS)
				return Int2(x, y);
		}
	}

	assert(0 && "Missing common area!");
	return Int2(-1, -1);
}
