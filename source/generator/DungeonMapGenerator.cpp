#include "Pch.h"
#include "DungeonMapGenerator.h"

#include "GameCommon.h"

#define H(_x,_y) map[(_x)+(_y)*mapW].type
#define HR(_x,_y) mapRooms[(_x)+(_y)*mapW]

//=================================================================================================
void DungeonMapGenerator::Generate(MapSettings& settings, bool recreate)
{
	assert(settings.mapW && settings.mapH && settings.roomSize.x >= 4 && settings.roomSize.y >= settings.roomSize.x
		&& InRange(settings.corridorChance, 0, 100) && settings.rooms);
	assert(settings.corridorChance == 0 || (settings.corridorSize.x >= 3 && settings.corridorSize.y >= settings.corridorSize.x));

	mapW = settings.mapW;
	mapH = settings.mapH;
	if(!recreate)
		map = new Tile[mapW * mapH];
	this->settings = &settings;
	empty.clear();
	joined.clear();

	SetLayout();
	GenerateRooms();
	if(settings.removeDeadEndCorridors)
		RemoveDeadEndCorridors();
	SetRoomIndices();
	if(settings.corridorJoinChance > 0)
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
	memset(map, 0, sizeof(Tile) * mapW * mapH);
	mapRooms.resize(mapW * mapH);
	memset(mapRooms.data(), 0, sizeof(Room*) * mapW * mapH);

	if(settings->shape == MapSettings::SHAPE_SQUARE)
	{
		for(int x = 0; x < mapW; ++x)
		{
			H(x, 0) = BLOCKADE;
			H(x, mapH - 1) = BLOCKADE;
		}

		for(int y = 0; y < mapH; ++y)
		{
			H(0, y) = BLOCKADE;
			H(mapW - 1, y) = BLOCKADE;
		}
	}
	else
	{
		int w = (mapW - 3) / 2,
			h = (mapH - 3) / 2;

		for(int y = 0; y < mapH; ++y)
		{
			for(int x = 0; x < mapW; ++x)
			{
				if(Distance(float(x - 1) / w, float(y - 1) / h, 1.f, 1.f) > 1.f)
					map[x + y * mapW].type = BLOCKADE;
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
		int w = settings->roomSize.Random(),
			h = settings->roomSize.Random();
		AddRoom((mapW - w) / 2, (mapH - h) / 2, w, h, false, nullptr);
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

		Int2 pt(entry.first % mapW, entry.first / mapW);
		if(Rand() % 100 < settings->corridorChance)
			AddCorridor(pt, dir, entry.second);
		else
		{
			int w = settings->roomSize.Random(),
				h = settings->roomSize.Random();

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
	int x = id % mapW,
		y = id / mapW;
	if(x == 0 || y == 0 || x == mapW - 1 || y == mapH - 1)
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
	if(map[id + mapW].type == EMPTY)
	{
		++count;
		dir = GDIR_UP;
	}
	if(map[id - mapW].type == EMPTY)
	{
		++count;
		dir = GDIR_DOWN;
	}

	if(count != 1)
		return GDIR_INVALID;

	dir = Reversed(dir);
	const Int2& od = DirToPos(dir);
	if(map[id + od.x + od.y * mapW].type != UNUSED)
		return GDIR_INVALID;

	return dir;
}

//=================================================================================================
void DungeonMapGenerator::AddRoom(int x, int y, int w, int h, bool isCorridor, Room* parent)
{
	assert(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < mapW && y + h < mapH);

	Room* room = Room::Get();
	room->pos.x = x;
	room->pos.y = y;
	room->size.x = w;
	room->size.y = h;
	room->target = (isCorridor ? RoomTarget::Corridor : RoomTarget::None);
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
		if(CheckFreePath(x + i + y * mapW) != GDIR_INVALID)
			empty.push_back({ x + i + y * mapW, room });
		if(CheckFreePath(x + i + (y + h - 1) * mapW) != GDIR_INVALID)
			empty.push_back({ x + i + (y + h - 1) * mapW, room });
	}
	for(int i = 1; i < h - 1; ++i)
	{
		if(CheckFreePath(x + (y + i) * mapW) != GDIR_INVALID)
			empty.push_back({ x + (y + i) * mapW, room });
		if(CheckFreePath(x + w - 1 + (y + i) * mapW) != GDIR_INVALID)
			empty.push_back({ x + w - 1 + (y + i) * mapW, room });
	}
}

//=================================================================================================
void DungeonMapGenerator::AddCorridor(Int2& pt, GameDirection dir, Room* parent)
{
	int len = settings->corridorSize.Random();
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
	if(!(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < mapW && y + h < mapH))
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
	LocalVector<Room*> toRemove;
	for(Room* room : *settings->rooms)
	{
		if(room->IsCorridor() && room->connected.size() == 1u)
			toRemove->push_back(room);
	}

	while(!toRemove->empty())
	{
		Room* room = toRemove->back();
		toRemove->pop_back();

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
			toRemove->push_back(connected);
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
			if(y < mapH - 1 && H(x - 1, y + 1) == EMPTY)
				ok = false;
		}
		if(x < mapW - 1)
		{
			if(H(x + 1, y) == EMPTY)
				ok = false;
			if(y > 0 && H(x + 1, y - 1) == EMPTY)
				ok = false;
			if(y < mapH - 1 && H(x + 1, y + 1) == EMPTY)
				ok = false;
		}
		if(y > 0 && H(x, y - 1) == EMPTY)
			ok = false;
		if(y < mapH - 1 && H(x, y + 1) == EMPTY)
			ok = false;

		if(ok)
		{
			if(tile == BLOCKADE_WALL)
				tile = BLOCKADE;
			else
				tile = UNUSED;
		}
	}

	Room*& tileRoom = HR(x, y);
	if(tileRoom == room)
		tileRoom = nullptr;
}

//=================================================================================================
void DungeonMapGenerator::SetRoomIndices()
{
	for(int i = 0, count = (int)settings->rooms->size(); i < count; ++i)
		settings->rooms->at(i)->index = i;

	for(int i = 0; i < mapW * mapH; ++i)
	{
		Room* room = mapRooms[i];
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
				Tile& p = map[x + room->pos.x + (y + room->pos.y) * mapW];
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
			if(!room2->IsCorridor() || Rand() % 100 >= settings->corridorJoinChance)
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
					int index = x + y * mapW;
					Tile& tile = map[index];
					if(tile.type == DOORS)
					{
						assert(mapRooms[index] == room || mapRooms[index] == room2);
						tile.type = EMPTY;
						goto removedDoors;
					}
				}
			}
		removedDoors:
			;
		}

		// mark low celling
		for(int y = 0; y < room->size.y; ++y)
		{
			for(int x = 0; x < room->size.x; ++x)
			{
				Tile& p = map[x + room->pos.x + (y + room->pos.y) * mapW];
				if(p.type == EMPTY || p.type == DOORS || p.type == BARS_FLOOR)
					p.flags = Tile::F_LOW_CEILING;
			}
		}
	}
}

//=================================================================================================
void DungeonMapGenerator::AddHoles()
{
	if(settings->barsChance <= 0)
		return;
	for(int y = 1; y < mapW - 1; ++y)
	{
		for(int x = 1; x < mapH - 1; ++x)
		{
			Tile& tile = map[x + y * mapW];
			if(tile.type == EMPTY && Rand() % 100 < settings->barsChance)
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
	if(settings->roomJoinChance > 0)
		JoinRooms();

	if(settings->groups)
	{
		CreateRoomGroups();
		RoomGroup::SetRoomGroupConnections(*settings->groups, *settings->rooms);
	}

	if(settings->prevEntryLoc != MapSettings::ENTRY_NONE || settings->nextEntryLoc != MapSettings::ENTRY_NONE)
		GenerateEntry();

	SetRoomGroupTargets();
	Tile::SetupFlags(map, Int2(mapW, mapH), settings->prevEntryType, settings->nextEntryType);

	if(settings->devmode)
		Tile::DebugDraw(map, Int2(mapW, mapH));
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
			if(!room2->CanJoinRoom() || Rand() % 100 >= settings->roomJoinChance)
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
			if(x2 == mapW - 1)
				--x2;
			if(y2 == mapH - 1)
				--y2;
			assert(x1 < x2 && y1 < y2);

			for(int y = y1; y < y2; ++y)
			{
				for(int x = x1; x < x2; ++x)
				{
					if(IsConnectingWall(x, y, room, room2))
					{
						Tile& tile = map[x + y * mapW];
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
	if(mapRooms[x - 1 + y * mapW] == room1 && IsEmpty(map[x - 1 + y * mapW].type))
	{
		if(mapRooms[x + 1 + y * mapW] == room2 && IsEmpty(map[x + 1 + y * mapW].type))
			return true;
	}
	else if(mapRooms[x - 1 + y * mapW] == room2 && IsEmpty(map[x - 1 + y * mapW].type))
	{
		if(mapRooms[x + 1 + y * mapW] == room1 && IsEmpty(map[x + 1 + y * mapW].type))
			return true;
	}
	if(mapRooms[x + (y - 1) * mapW] == room1 && IsEmpty(map[x + (y - 1) * mapW].type))
	{
		if(mapRooms[x + (y + 1) * mapW] == room2 && IsEmpty(map[x + (y + 1) * mapW].type))
			return true;
	}
	else if(mapRooms[x + (y - 1) * mapW] == room2 && IsEmpty(map[x + (y - 1) * mapW].type))
	{
		if(mapRooms[x + (y + 1) * mapW] == room1 && IsEmpty(map[x + (y + 1) * mapW].type))
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
	char* ce = new char[mapW * mapH];
	memset(ce, ' ', sizeof(char) * mapW * mapH);
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
					ce[x + room->pos.x + (y + room->pos.y) * mapW] = s;
			}
		}
	}
	for(int y = mapH - 1; y >= 0; --y)
	{
		for(int x = 0; x < mapW; ++x)
			putchar(ce[x + y * mapW]);
		putchar('\n');
	}
	delete[] ce;
}

//=================================================================================================
void DungeonMapGenerator::GenerateEntry()
{
	mapRooms.clear();
	for(Room* room : *settings->rooms)
	{
		if(room->target == RoomTarget::None)
			mapRooms.push_back(room);
	}

	if(settings->prevEntryLoc != MapSettings::ENTRY_NONE)
	{
		GenerateEntry(mapRooms, settings->prevEntryLoc, settings->prevEntryType, settings->prevEntryRoom,
			settings->prevEntryPt, settings->prevEntryDir, true);
	}

	if(settings->nextEntryLoc != MapSettings::ENTRY_NONE)
	{
		GenerateEntry(mapRooms, settings->nextEntryLoc, settings->nextEntryType, settings->nextEntryRoom,
			settings->nextEntryPt, settings->nextEntryDir, false);
	}

	mapRooms.clear();
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
			const Int2 farPt = Int2(mapW / 2, mapH / 2);
			if(AddEntryFarFromPoint(rooms, farPt, room, type, pt, dir, isPrev))
				return;
		}
		break;
	case MapSettings::ENTRY_FAR_FROM_PREV:
		{
			int index = 0;
			for(int i = 0, count = (int)mapRooms.size(); i < count; ++i)
			{
				if(mapRooms[i]->target == RoomTarget::EntryPrev)
				{
					index = i;
					break;
				}
			}

			const Int2 farPt = mapRooms[index]->CenterTile();
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
		int pId = p.back().x;
		p.pop_back();
		room = rooms[pId];
		if(AddEntry(*room, type, pt, dir, isPrev))
			return true;
	}

	return false;
}

//=================================================================================================
bool DungeonMapGenerator::AddEntry(Room& room, EntryType& type, Int2& pt, GameDirection& dir, bool isPrev)
{
#define B(_xx,_yy) (IsBlocking(map[x+_xx+(y+_yy)*mapW].type))
#define P(_xx,_yy) (!IsBlocking2(map[x+_xx+(y+_yy)*mapW].type))

	struct PosDir
	{
		Int2 pt;
		int dir, prio;
		bool inWall;

		PosDir(int x, int y, int dir, bool inWall, const Room& room) : pt(x, y), dir(dir), prio(0), inWall(inWall)
		{
			if(inWall)
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

	for(int y = max(1, room.pos.y); y < min(mapH - 1, room.size.y + room.pos.y); ++y)
	{
		for(int x = max(1, room.pos.x); x < min(mapW - 1, room.size.x + room.pos.x); ++x)
		{
			Tile& p = map[x + y * mapW];
			if(p.type == EMPTY)
			{
				const bool left = (x > 0);
				const bool right = (x<int(mapW - 1));
				const bool top = (y<int(mapH - 1));
				const bool bottom = (y > 0);

				if(!isDoor && left && right && top && bottom)
				{
					// ###
					// #>
					// #
					if(B(-1, 1) && B(0, 1) && B(-1, 0) && B(1, 1) && B(-1, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_DOWN) | Bit(GDIR_RIGHT), false, room));

					// ###
					//  >#
					//   #
					if(B(0, 1) && B(1, 1) && B(1, 0) && B(-1, 1) && B(1, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_DOWN) | Bit(GDIR_LEFT), false, room));

					// #
					// #>
					// ###
					if(B(-1, 0) && B(-1, -1) && B(0, -1) && B(-1, 1) && B(1, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_UP) | Bit(GDIR_RIGHT), false, room));

					//   #
					//  >#
					// ###
					if(B(1, 0) && B(0, -1) && B(1, -1) && B(1, 1) && B(-1, -1))
						choice.push_back(PosDir(x, y, Bit(GDIR_LEFT) | Bit(GDIR_UP), false, room));

					//  ___
					//  _>_
					//  ___
					if(P(-1, -1) && P(0, -1) && P(1, -1) && P(-1, 0) && P(1, 0) && P(-1, 1) && P(0, 1) && P(1, 1))
						choice.push_back(PosDir(x, y, Bit(GDIR_DOWN) | Bit(GDIR_LEFT) | Bit(GDIR_UP) | Bit(GDIR_RIGHT), false, room));
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
					// >#
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
			else if(!isDoor && (p.type == WALL || p.type == BLOCKADE_WALL) && (x > 0) && (x<int(mapW - 1)) && (y > 0) && (y<int(mapH - 1)))
			{
				// ##_
				// #>_
				// ##_
				if(B(-1, 1) && B(0, 1) && B(-1, 0) && B(-1, -1) && B(0, -1) && P(1, -1) && P(1, 0) && P(1, 1))
					choice.push_back(PosDir(x, y, Bit(GDIR_RIGHT), true, room));

				// _##
				// _>#
				// _##
				if(B(0, 1) && B(1, 1) && B(1, 0) && B(0, -1) && B(1, -1) && P(-1, -1) && P(-1, 0) && P(-1, 1))
					choice.push_back(PosDir(x, y, Bit(GDIR_LEFT), true, room));

				// ###
				// #>#
				// ___
				if(B(-1, 1) && B(0, 1) && B(1, 1) && B(-1, 0) && B(1, 0) && P(-1, -1) && P(0, -1) && P(1, -1))
					choice.push_back(PosDir(x, y, Bit(GDIR_DOWN), true, room));

				// ___
				// #>#
				// ###
				if(B(-1, 0) && B(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1) && P(-1, 1) && P(0, 1) && P(1, 1))
					choice.push_back(PosDir(x, y, Bit(GDIR_UP), true, room));
			}
		}
	}

	if(choice.empty())
		return false;

	std::sort(choice.begin(), choice.end());

	int bestPrio = choice.front().prio;
	int count = 0;

	vector<PosDir>::iterator it = choice.begin(), end = choice.end();
	while(it != end && it->prio == bestPrio)
	{
		++it;
		++count;
	}

	PosDir& pd = choice[Rand() % count];
	pt = pd.pt;
	map[pd.pt.x + pd.pt.y * mapW].type = (isPrev ? ENTRY_PREV : ENTRY_NEXT);
	map[pd.pt.x + pd.pt.y * mapW].room = room.index;
	room.target = (isPrev ? RoomTarget::EntryPrev : RoomTarget::EntryNext);
	room.AddTile(pd.pt);
	if(type == ENTRY_STAIRS_DOWN && pd.inWall)
		type = ENTRY_STAIRS_DOWN_IN_WALL;

	for(int y = max(0, pd.pt.y - 1); y <= min(mapH, pd.pt.y + 1); ++y)
	{
		for(int x = max(0, pd.pt.x - 1); x <= min(mapW, pd.pt.x + 1); ++x)
		{
			TILE_TYPE& p = map[x + y * mapW].type;
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
			Tile& tile = map[x + y * mapW];
			if(tile.type == EMPTY || tile.type == DOORS)
				return Int2(x, y);
		}
	}

	assert(0 && "Missing common area!");
	return Int2(-1, -1);
}
