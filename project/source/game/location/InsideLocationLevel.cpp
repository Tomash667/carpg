// dane poziomu lokacji
#include "Pch.h"
#include "Core.h"
#include "InsideLocationLevel.h"
#include "Game.h"
#include "SaveState.h"

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
bool InsideLocationLevel::GetRandomNearWallTile(const Room& room, Int2& _tile, int& _rot, bool nocol)
{
	_rot = Rand() % 4;

	int tries = 0;

	do
	{
		int tries2 = 10;

		switch(_rot)
		{
		case 2:
			// górna œciana, obj \/
			do
			{
				_tile.x = Random(room.pos.x + 1, room.pos.x + room.size.x - 2);
				_tile.y = room.pos.y + 1;

				if(czy_blokuje2(map[_tile.x + (_tile.y - 1)*w]) && !czy_blokuje21(map[_tile.x + _tile.y*w]) && (nocol || !czy_blokuje21(map[_tile.x + (_tile.y + 1)*w])))
					return true;

				--tries2;
			} while(tries2 > 0);
			break;
		case 1:
			// prawa œciana, obj <
			do
			{
				_tile.x = room.pos.x + room.size.x - 2;
				_tile.y = Random(room.pos.y + 1, room.pos.y + room.size.y - 2);

				if(czy_blokuje2(map[_tile.x + 1 + _tile.y*w]) && !czy_blokuje21(map[_tile.x + _tile.y*w]) && (nocol || !czy_blokuje21(map[_tile.x - 1 + _tile.y*w])))
					return true;

				--tries2;
			} while(tries2 > 0);
			break;
		case 0:
			// dolna œciana, obj /|
			do
			{
				_tile.x = Random(room.pos.x + 1, room.pos.x + room.size.x - 2);
				_tile.y = room.pos.y + room.size.y - 2;

				if(czy_blokuje2(map[_tile.x + (_tile.y + 1)*w]) && !czy_blokuje21(map[_tile.x + _tile.y*w]) && (nocol || !czy_blokuje21(map[_tile.x + (_tile.y - 1)*w])))
					return true;

				--tries2;
			} while(tries2 > 0);
			break;
		case 3:
			// lewa œciana, obj >
			do
			{
				_tile.x = room.pos.x + 1;
				_tile.y = Random(room.pos.y + 1, room.pos.y + room.size.y - 2);

				if(czy_blokuje2(map[_tile.x - 1 + _tile.y*w]) && !czy_blokuje21(map[_tile.x + _tile.y*w]) && (nocol || !czy_blokuje21(map[_tile.x + 1 + _tile.y*w])))
					return true;

				--tries2;
			} while(tries2 > 0);
			break;
		}

		++tries;
		_rot = (_rot + 1) % 4;
	} while(tries <= 3);

	return false;
}

//=================================================================================================
void InsideLocationLevel::SaveLevel(HANDLE file, bool local)
{
	WriteFile(file, &w, sizeof(w), &tmp, nullptr);
	WriteFile(file, &h, sizeof(h), &tmp, nullptr);
	WriteFile(file, map, sizeof(Pole)*w*h, &tmp, nullptr);

	uint ile;

	// jednostki
	ile = units.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		(*it)->Save(file, local);

	// skrzynie
	ile = chests.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
		(*it)->Save(file, local);

	// obiekty
	ile = objects.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Object*>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		(*it)->Save(file);

	// drzwi
	ile = doors.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
		(*it)->Save(file, local);

	// przedmioty
	ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		(*it)->Save(file);

	// u¿ywalne
	ile = usables.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
		(*it)->Save(file, local);

	// krew
	FileWriter f(file);
	ile = bloods.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Save(f);

	// œwiat³a
	f << lights.size();
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Save(f);

	// pokoje
	ile = rooms.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Room>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it)
		it->Save(file);

	// pu³apki
	ile = traps.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Trap*>::iterator it = traps.begin(), end = traps.end(); it != end; ++it)
		(*it)->Save(file, local);

	WriteFile(file, &staircase_up, sizeof(staircase_up), &tmp, nullptr);
	WriteFile(file, &staircase_down, sizeof(staircase_down), &tmp, nullptr);
	WriteFile(file, &staircase_up_dir, sizeof(staircase_up_dir), &tmp, nullptr);
	WriteFile(file, &staircase_down_dir, sizeof(staircase_down_dir), &tmp, nullptr);
	WriteFile(file, &staircase_down_in_wall, sizeof(staircase_down_in_wall), &tmp, nullptr);
}

//=================================================================================================
void InsideLocationLevel::LoadLevel(HANDLE file, bool local)
{
	ReadFile(file, &w, sizeof(w), &tmp, nullptr);
	ReadFile(file, &h, sizeof(h), &tmp, nullptr);
	map = new Pole[w*h];
	ReadFile(file, map, sizeof(Pole)*w*h, &tmp, nullptr);

	// jednostki
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	units.resize(ile);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		*it = new Unit;
		Unit::AddRefid(*it);
		(*it)->Load(file, local);
	}

	// skrzynie
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	chests.resize(ile);
	for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
	{
		*it = new Chest;
		(*it)->Load(file, local);
	}

	// obiekty
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	objects.resize(ile);
	for(vector<Object*>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		*it = new Object;
		(*it)->Load(file);
	}

	// drzwi
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	doors.resize(ile);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
	{
		*it = new Door;
		(*it)->Load(file, local);
	}

	// przedmioty
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	items.resize(ile);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		*it = new GroundItem;
		(*it)->Load(file);
	}

	// u¿ywalne
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	usables.resize(ile);
	for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
	{
		*it = new Usable;
		Usable::AddRefid(*it);
		(*it)->Load(file, local);
	}

	// krew
	FileReader f(file);
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	bloods.resize(ile);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Load(f);

	// œwiat³a
	f >> ile;
	lights.resize(ile);
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Load(f);

	// pokoje
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	rooms.resize(ile);
	for(vector<Room>::iterator it = rooms.begin(), end = rooms.end(); it != end; ++it)
		it->Load(file);

	// pu³apki
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	traps.resize(ile);
	for(vector<Trap*>::iterator it = traps.begin(), end = traps.end(); it != end; ++it)
	{
		*it = new Trap;
		(*it)->Load(file, local);
	}

	ReadFile(file, &staircase_up, sizeof(staircase_up), &tmp, nullptr);
	ReadFile(file, &staircase_down, sizeof(staircase_down), &tmp, nullptr);
	ReadFile(file, &staircase_up_dir, sizeof(staircase_up_dir), &tmp, nullptr);
	ReadFile(file, &staircase_down_dir, sizeof(staircase_down_dir), &tmp, nullptr);
	ReadFile(file, &staircase_down_in_wall, sizeof(staircase_down_in_wall), &tmp, nullptr);

	// konwersja krzese³ w sto³ki
	if(LOAD_VERSION < V_0_2_12)
	{
		auto chair = BaseUsable::Get("chair"),
			stool = BaseUsable::Get("stool");
		for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
		{
			Usable& u = **it;
			if(u.base == chair)
				u.base = stool;
		}
	}

	// konwersja ³awy w obrócon¹ ³awê i ustawienie wariantu
	if(LOAD_VERSION < V_0_2_20)
	{
		auto bench = BaseUsable::Get("bench"),
			bench_dir = BaseUsable::Get("bench_dir");
		for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
		{
			Usable& u = **it;
			if(u.base == bench)
			{
				u.base = bench_dir;
				u.variant = Rand() % 2;
			}
		}
	}
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
void InsideLocationLevel::BuildRefidTable()
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
