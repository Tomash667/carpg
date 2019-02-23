#include "Pch.h"
#include "GameCore.h"
#include "InsideLocationLevel.h"
#include "Game.h"
#include "SaveState.h"
#include "GameFile.h"

//=================================================================================================
InsideLocationLevel::~InsideLocationLevel()
{
	delete[] map;
	DeleteElements(objects);
	DeleteElements(units);
	DeleteElements(chests);
	DeleteElements(doors);
	DeleteElements(usables);
	DeleteElements(items);
	DeleteElements(traps);
}

//=================================================================================================
Room* InsideLocationLevel::GetNearestRoom(const Vec3& pos)
{
	if(rooms.empty())
		return nullptr;

	float dist, best_dist = 1000.f;
	Room* best_room = nullptr;

	for(vector<Room>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it)
	{
		dist = it->Distance(pos);
		if(dist < best_dist)
		{
			if(dist == 0.f)
				return &*it;
			best_dist = dist;
			best_room = &*it;
		}
	}

	return best_room;
}

//=================================================================================================
Room* InsideLocationLevel::FindEscapeRoom(const Vec3& _my_pos, const Vec3& _enemy_pos)
{
	Room* my_room = GetNearestRoom(_my_pos),
		*enemy_room = GetNearestRoom(_enemy_pos);

	if(!my_room)
		return nullptr;

	int id;
	if(enemy_room)
		id = GetRoomId(enemy_room);
	else
		id = -1;

	Room* best_room = nullptr;
	float best_dist = 0.f, dist;
	Vec3 mid;

	for(vector<int>::iterator it = my_room->connected.begin(), end = my_room->connected.end(); it != end; ++it)
	{
		if(*it == id)
			continue;

		mid = rooms[*it].Center();

		dist = Vec3::Distance(_my_pos, mid) - Vec3::Distance(_enemy_pos, mid);
		if(dist < best_dist)
		{
			best_dist = dist;
			best_room = &rooms[*it];
		}
	}

	return best_room;
}

//=================================================================================================
Room* InsideLocationLevel::GetRoom(const Int2& pt)
{
	word room = map[pt(w)].room;
	if(room == (word)-1)
		return nullptr;
	return &rooms[room];
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
				Room& room = rooms[room_index];
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
			} while(tries2 > 0);
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
			} while(tries2 > 0);
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
			} while(tries2 > 0);
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
			} while(tries2 > 0);
			break;
		}

		++tries;
		rot = (GameDirection)((rot + 1) % 4);
	} while(tries <= 3);

	return false;
}

//=================================================================================================
void InsideLocationLevel::SaveLevel(GameWriter& f, bool local)
{
	f << w;
	f << h;
	f.Write(map, sizeof(Tile)*w*h);

	// units
	f << units.size();
	for(Unit* unit : units)
		unit->Save(f, local);

	// chests
	f << chests.size();
	for(Chest* chest : chests)
		chest->Save(f, local);

	// objects
	f << objects.size();
	for(Object* object : objects)
		object->Save(f);

	// doors
	f << doors.size();
	for(Door* door : doors)
		door->Save(f, local);

	// ground items
	f << items.size();
	for(GroundItem* item : items)
		item->Save(f);

	// usable objects
	f << usables.size();
	for(Usable* usable : usables)
		usable->Save(f, local);

	// bloods
	f << bloods.size();
	for(Blood& blood : bloods)
		blood.Save(f);

	// lights
	f << lights.size();
	for(Light& light : lights)
		light.Save(f);

	// rooms
	f << rooms.size();
	for(Room& room : rooms)
		room.Save(f);

	// room groups
	f << groups.size();
	for(RoomGroup& group : groups)
	{
		f << group.rooms;
		f << group.target;
	}

	// traps
	f << traps.size();
	for(Trap* trap : traps)
		trap->Save(f, local);

	f << staircase_up;
	f << staircase_down;
	f << staircase_up_dir;
	f << staircase_down_dir;
	f << staircase_down_in_wall;
}

//=================================================================================================
void InsideLocationLevel::LoadLevel(GameReader& f, bool local)
{
	f >> w;
	f >> h;
	map = new Tile[w*h];
	f.Read(map, sizeof(Tile)*w*h);

	// units
	units.resize(f.Read<uint>());
	for(Unit*& unit : units)
	{
		unit = new Unit;
		Unit::AddRefid(unit);
		unit->Load(f, local);
	}

	// chests
	chests.resize(f.Read<uint>());
	for(Chest*& chest : chests)
	{
		chest = new Chest;
		chest->Load(f, local);
	}

	// objects
	objects.resize(f.Read<uint>());
	for(Object*& object : objects)
	{
		object = new Object;
		object->Load(f);
	}

	// doors
	doors.resize(f.Read<uint>());
	for(Door*& door : doors)
	{
		door = new Door;
		door->Load(f, local);
	}

	// ground items
	items.resize(f.Read<int>());
	for(GroundItem*& item : items)
	{
		item = new GroundItem;
		item->Load(f);
	}

	// usable objects
	usables.resize(f.Read<uint>());
	for(Usable*& usable : usables)
	{
		usable = new Usable;
		Usable::AddRefid(usable);
		usable->Load(f, local);
	}

	// bloods
	bloods.resize(f.Read<uint>());
	for(Blood& blood : bloods)
		blood.Load(f);

	// lights
	lights.resize(f.Read<uint>());
	for(Light& light : lights)
		light.Load(f);

	// rooms
	rooms.resize(f.Read<uint>());
	for(Room& room : rooms)
		room.Load(f);

	// room groups
	if(LOAD_VERSION >= V_0_8)
	{
		groups.resize(f.Read<uint>());
		for(RoomGroup& group : groups)
		{
			f >> group.rooms;
			f >> group.target;
		}
	}
	else
	{
		groups.resize(rooms.size());
		for(int i = 0; i < (int)rooms.size(); ++i)
		{
			groups[i].rooms.push_back(i);
			groups[i].target = rooms[i].target;
		}
	}

	// traps
	traps.resize(f.Read<uint>());
	for(Trap*& trap : traps)
	{
		trap = new Trap;
		trap->Load(f, local);
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
		Room* gora = GetNearestRoom(Vec3(2.f*staircase_up.x + 1, 0, 2.f*staircase_up.y + 1));
		Room* dol = GetNearestRoom(Vec3(2.f*staircase_down.x + 1, 0, 2.f*staircase_down.y + 1));
		int best_dist, dist;
		Room* best = nullptr;

		for(vector<Room>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it)
		{
			if(it->IsCorridor() || (no_target && it->target != RoomTarget::None))
				continue;
			dist = Int2::Distance(it->pos, gora->pos) + Int2::Distance(it->pos, dol->pos);
			if(!best || dist > best_dist)
			{
				best_dist = dist;
				best = &*it;
			}
		}

		return *best;
	}
	else
	{
		Room* gora = GetNearestRoom(Vec3(2.f*staircase_up.x + 1, 0, 2.f*staircase_up.y + 1));
		int best_dist, dist;
		Room* best = nullptr;

		for(vector<Room>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it)
		{
			if(it->IsCorridor() || (no_target && it->target != RoomTarget::None))
				continue;
			dist = Int2::Distance(it->pos, gora->pos);
			if(!best || dist > best_dist)
			{
				best_dist = dist;
				best = &*it;
			}
		}

		return *best;
	}
}

//=================================================================================================
void InsideLocationLevel::BuildRefidTables()
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		(*it)->refid = (int)Unit::refid_table.size();
		Unit::refid_table.push_back(*it);
	}

	for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
	{
		(*it)->refid = (int)Usable::refid_table.size();
		Usable::refid_table.push_back(*it);
	}
}

//=================================================================================================
int InsideLocationLevel::FindRoomId(RoomTarget target)
{
	int index = 0;
	for(vector<Room>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it, ++index)
	{
		if(it->target == target)
			return index;
	}

	return -1;
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

	return map[pt.x - 1 + pt.y*w].IsWall() ||
		map[pt.x + 1 + pt.y*w].IsWall() ||
		map[pt.x + (pt.y - 1)*w].IsWall() ||
		map[pt.x + (pt.y + 1)*w].IsWall();
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
		if(IS_SET(kierunek, 1 << i))
		{
			dir = i;
			return true;
		}
		i = (i + 1) % 4;
	}

	return true;
}

//=================================================================================================
bool InsideLocationLevel::FindUnit(Unit* unit)
{
	assert(unit);

	for(Unit* u : units)
	{
		if(u == unit)
			return true;
	}

	return false;
}

//=================================================================================================
Unit* InsideLocationLevel::FindUnit(UnitData* data)
{
	assert(data);

	for(Unit* u : units)
	{
		if(u->data == data)
			return u;
	}

	return nullptr;
}

//=================================================================================================
Chest* InsideLocationLevel::FindChestWithItem(const Item* item, int* index)
{
	assert(item);

	for(Chest* chest : chests)
	{
		int idx = chest->FindItem(item);
		if(idx != -1)
		{
			if(index)
				*index = idx;
			return chest;
		}
	}

	return nullptr;
}

//=================================================================================================
Chest* InsideLocationLevel::FindChestWithQuestItem(int quest_refid, int* index)
{
	for(Chest* chest : chests)
	{
		int idx = chest->FindQuestItem(quest_refid);
		if(idx != -1)
		{
			if(index)
				*index = idx;
			return chest;
		}
	}

	return nullptr;
}

//=================================================================================================
Room& InsideLocationLevel::GetRoom(RoomTarget target, bool down_stairs)
{
	if(target == RoomTarget::None)
		return GetFarRoom(down_stairs, true);
	else
	{
		int id = FindRoomId(target);
		if(id == -1)
		{
			assert(0);
			id = 0;
		}

		return rooms[id];
	}
}
