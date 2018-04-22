#include "Pch.h"
#include "GameCore.h"
#include "DungeonGenerator.h"

//-----------------------------------------------------------------------------
#define F(_x,_y) mapa[(_x)+(_y)*opcje->w]
#define H(_x,_y) F(_x,_y).type
#define HR(_x,_y) F(_x,_y).room

//-----------------------------------------------------------------------------
enum GenerateError
{
	ZLE_DANE = 1,
	BRAK_MIEJSCA = 2
};

//-----------------------------------------------------------------------------
const DungeonGenerator::DIR opposite_dir[5] = {
	DungeonGenerator::GORA,
	DungeonGenerator::PRAWO,
	DungeonGenerator::DOL,
	DungeonGenerator::LEWO,
	DungeonGenerator::BRAK
};

//-----------------------------------------------------------------------------
const Int2 dir_shift[5] = {
	Int2(0,-1), // DÓ£
	Int2(-1,0), // LEWO
	Int2(0,1), // GÓRA
	Int2(1,0), // PRAWO
	Int2(0,0) // BRAK
};

//=================================================================================================
bool DungeonGenerator::Generate(OpcjeMapy& _opcje, bool recreate)
{
	// sprawdŸ opcje
	assert(_opcje.w && _opcje.h && _opcje.room_size.x >= 2 && _opcje.room_size.y >= _opcje.room_size.x
		&& InRange(_opcje.korytarz_szansa, 0, 100) && _opcje.rooms);
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! walidacja nowych opcji
	assert(_opcje.korytarz_szansa == 0 || (_opcje.corridor_size.x >= 1 && _opcje.corridor_size.y >= _opcje.corridor_size.x));

	if(!recreate)
		mapa = new Pole[_opcje.w * _opcje.h];
	opcje = &_opcje;
	free.clear();

	_opcje.blad = 0;
	_opcje.mapa = mapa;

	GenerateInternal();

	if(_opcje.stop)
		return (_opcje.blad == 0);

	if(_opcje.devmode)
		Draw();

	return (_opcje.blad == 0);
}

//=================================================================================================
bool DungeonGenerator::ContinueGenerate()
{
	// po³¹cz pokoje
	if(opcje->polacz_pokoj > 0)
		JoinRooms();

	// generowanie schodów
	if(opcje->schody_dol != OpcjeMapy::BRAK || opcje->schody_gora != OpcjeMapy::BRAK)
		GenerateStairs();

	// generuj flagi pól
	SetFlags();

	if(opcje->devmode)
		Draw();

	return (opcje->blad == 0);
}

//=================================================================================================
void DungeonGenerator::GenerateInternal()
{
	FIXME;
	Srand(0);

	// ustaw wzór obszaru (np wype³nij obszar nieu¿ytkiem, na oko³o blokada)
	SetPattern();

	// jeœli s¹ jakieœ pocz¹tkowe pokoje to je dodaj
	if(!opcje->rooms->empty())
	{
		int index = 0;
		for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it, ++index)
			AddRoom(*it, index);
	}
	else
	{
		// stwórz pocz¹tkowy pokój
		int w = opcje->room_size.Random(),
			h = opcje->room_size.Random();
		AddRoom(Room::INVALID_ROOM, (opcje->w - w) / 2, (opcje->h - h) / 2, w, h, ADD_ROOM);
	}

	// generuj
	while(!free.empty())
	{
		int idx = Rand() % free.size();
		int id = free[idx];
		if(idx != free.size() - 1)
			std::iter_swap(free.begin() + idx, free.end() - 1);
		free.pop_back();

		// sprawdŸ czy to miejsce jest dobre
		DIR dir = IsFreeWay(id);
		if(dir == BRAK)
			continue;

		Int2 pt(id % opcje->w, id / opcje->w);
		Int2 parent_pt = pt + dir_shift[opposite_dir[dir]];
		word parent_room = mapa[parent_pt(opcje->w)].room;
		assert(parent_room != Room::INVALID_ROOM);
		int ramp = CanCreateRamp(parent_room);
		
		if(ramp != 0)
		{
			CreateRamp(parent_room, pt, dir, ramp == 1);
			continue;
		}

		// pokój czy korytarz
		if(Rand() % 100 < opcje->korytarz_szansa)
			CreateCorridor(parent_room, pt, dir);
		else
			CreateRoom(id, parent_room, pt, dir);
	}
	
	// po³¹cz korytarze
	if(opcje->polacz_korytarz > 0)
		JoinCorridors();
	MarkCorridors();

	// losowe dziury
	if(opcje->kraty_szansa > 0)
		CreateHoles();

	if(opcje->stop)
		return;

	// po³¹cz pokoje
	if(opcje->polacz_pokoj > 0)
		JoinRooms();

	// generowanie schodów
	if(opcje->schody_dol != OpcjeMapy::BRAK || opcje->schody_gora != OpcjeMapy::BRAK)
		GenerateStairs();

	// generuj flagi pól
	SetFlags();
}

//=================================================================================================
void DungeonGenerator::SetPattern()
{
	Pole def;
	def.type = NIEUZYTE;
	def.flags = 0;
	def.room = Room::INVALID_ROOM;
	std::fill(mapa, mapa + (opcje->w * opcje->h), def);
	
	if(opcje->ksztalt == OpcjeMapy::PROSTOKAT)
	{
		for(int x = 0; x < opcje->w; ++x)
		{
			H(x, 0) = BLOKADA;
			H(x, opcje->h - 1) = BLOKADA;
		}

		for(int y = 0; y < opcje->h; ++y)
		{
			H(0, y) = BLOKADA;
			H(opcje->w - 1, y) = BLOKADA;
		}
	}
	else if(opcje->ksztalt == OpcjeMapy::OKRAG)
	{
		int w = (opcje->w - 3) / 2,
			h = (opcje->h - 3) / 2;

		for(int y = 0; y < opcje->h; ++y)
		{
			for(int x = 0; x < opcje->w; ++x)
			{
				if(Distance(float(x - 1) / w, float(y - 1) / h, 1.f, 1.f) > 1.f)
					mapa[x + y * opcje->w].type = BLOKADA;
			}
		}
	}
	else
	{
		assert(0);
	}
}

//=================================================================================================
void SetWall(POLE& pole)
{
	assert(pole == NIEUZYTE || pole == SCIANA || pole == DRZWI);
	if(pole == NIEUZYTE)
		pole = SCIANA;
}

//=================================================================================================
Room* DungeonGenerator::AddRoom(int parent_room, int x, int y, int w, int h, ADD add, int _id)
{
	assert(x >= 0 && y >= 0 && w >= 1 && h >= 1 && x + w < int(opcje->w) && y + h < int(opcje->h));

	int id;
	if(add == ADD_NO)
		id = _id;
	else
		id = (int)opcje->rooms->size();

	// mark tiles as empty
	for(int yy = y; yy < y + h; ++yy)
	{
		for(int xx = x; xx < x + w; ++xx)
		{
			Pole& p = F(xx, yy);
			assert(p.type == NIEUZYTE);
			p.type = PUSTE;
			p.room = (word)id;
		}
	}

	// mark tiles as walls
	for(int i = -1; i <= w; ++i)
	{
		SetWall(H(x + i, y - 1));
		SetWall(H(x + i, y + h));
	}
	for(int i = 0; i < h; ++i)
	{
		SetWall(H(x - 1, y + i));
		SetWall(H(x + w, y + i));
	}

	// check if there can be room/corridor behind wall
	if(add != ADD_RAMP)
	{
		for(int i = 0; i < w; ++i)
		{
			if(IsFreeWay(x + i + (y - 1) * opcje->w) != BRAK)
				free.push_back(x + i + (y - 1) * opcje->w);
			if(IsFreeWay(x + i + (y + h)*opcje->w) != BRAK)
				free.push_back(x + i + (y + h)*opcje->w);
		}
		for(int i = 0; i < h; ++i)
		{
			if(IsFreeWay(x - 1 + (y + i)*opcje->w) != BRAK)
				free.push_back(x - 1 + (y + i)*opcje->w);
			if(IsFreeWay(x + w + (y + i)*opcje->w) != BRAK)
				free.push_back(x + w + (y + i)*opcje->w);
		}
	}

	if(add != ADD_NO)
	{
		Room& room = Add1(*opcje->rooms);
		room.pos.x = x;
		room.pos.y = y;
		room.size.x = w;
		room.size.y = h;

		switch(add)
		{
		default:
		case ADD_ROOM:
			room.target = RoomTarget::None;
			break;
		case ADD_CORRIDOR:
			room.target = RoomTarget::Corridor;
			break;
		case ADD_RAMP:
			room.target = RoomTarget::Ramp;
			break;
		}

		if(parent_room != Room::INVALID_ROOM)
		{
			Room& r = opcje->rooms->at(parent_room);
			room.level = r.level;
			room.counter = r.counter + 1;
			room.y = r.y;
			room.connected.push_back(parent_room);
			r.connected.push_back(id);
		}
		else
		{
			room.level = 0;
			room.counter = 0;
			room.y = 0.f;
		}

		return &room;
	}

	return nullptr;
}

//=================================================================================================
// Sprawdza czy w danym punkcie mo¿na dodaæ wejœcie do korytarza/pokoju
//=================================================================================================
DungeonGenerator::DIR DungeonGenerator::IsFreeWay(int id)
{
	int x = id % opcje->w,
		y = id / opcje->w;
	if(x == 0 || y == 0 || x == opcje->w - 1 || y == opcje->h - 1)
		return BRAK;

	DIR jest = BRAK;
	int ile = 0;

	if(mapa[id - 1].type == PUSTE)
	{
		++ile;
		jest = LEWO;
	}
	if(mapa[id + 1].type == PUSTE)
	{
		++ile;
		jest = PRAWO;
	}
	if(mapa[id + opcje->w].type == PUSTE)
	{
		++ile;
		jest = GORA;
	}
	if(mapa[id - opcje->w].type == PUSTE)
	{
		++ile;
		jest = DOL;
	}

	if(ile != 1)
		return BRAK;

	const Int2& od = dir_shift[opposite_dir[jest]];
	if(mapa[id + od.x + od.y*opcje->w].type != NIEUZYTE)
		return BRAK;

	return opposite_dir[jest];
}

//=================================================================================================
// 0 no; 1 up; -1 down
int DungeonGenerator::CanCreateRamp(int room_index)
{
	return 0;

	Room& room = opcje->rooms->at(room_index);
	int chance = (room.counter - opcje->ramp_chance_min);
	if(chance <= 0)
		return 0;
	chance *= opcje->ramp_chance;
	if(Rand() % 100 >= chance)
		return 0;
	bool go_up = opcje->ramp_go_up;
	if(Rand() % 100 < opcje->ramp_chance_other)
		go_up = !go_up;
	if(opcje->ramp_go_up)
	{
		if(room.level == 0 && !go_up)
			go_up = true;
	}
	else
	{
		if(room.level == 0 && go_up)
			go_up = false;
	}
	room.counter -= opcje->ramp_decrease_chance;
	return go_up ? 1 : -1;
}

//=================================================================================================
Room* DungeonGenerator::CreateRamp(int parent_room, const Int2& start_pt, DIR dir, bool up)
{
	int length = opcje->ramp_length.Random();
	int ychange = length - 2;
	int w, h;
	Room& r = opcje->rooms->at(parent_room);
	int new_level = r.level + ychange / 2;
	float y1 = r.y,
		y2 = 0.5f * new_level;

	Int2 pt(start_pt);
	Int2 end_pt;
	float yb[4];

	switch(dir)
	{
	case LEWO:
		end_pt = Int2(pt.x - length, pt.y);
		pt.x -= length - 1;
		pt.y -= 1;
		w = length;
		h = 3;
		yb[Room::TOP_LEFT] = y2;
		yb[Room::TOP_RIGHT] = y1;
		yb[Room::BOTTOM_LEFT] = y2;
		yb[Room::BOTTOM_RIGHT] = y1;
		break;
	case PRAWO:
		end_pt = pt;
		pt.y -= 1;
		w = length;
		h = 3;
		yb[Room::TOP_LEFT] = y1;
		yb[Room::TOP_RIGHT] = y2;
		yb[Room::BOTTOM_LEFT] = y1;
		yb[Room::BOTTOM_RIGHT] = y2;
		break;
	case GORA:
		end_pt = pt;
		pt.x -= 1;
		w = 3;
		h = length;
		yb[Room::TOP_LEFT] = y2;
		yb[Room::TOP_RIGHT] = y2;
		yb[Room::BOTTOM_LEFT] = y1;
		yb[Room::BOTTOM_RIGHT] = y1;
		break;
	case DOL:
		end_pt = Int2(pt.x, pt.y - length);
		pt.y -= length - 1;
		pt.x -= 1;
		w = 3;
		h = length;
		yb[Room::TOP_LEFT] = y1;
		yb[Room::TOP_RIGHT] = y1;
		yb[Room::BOTTOM_LEFT] = y2;
		yb[Room::BOTTOM_RIGHT] = y2;
		break;
	}

	if(CanCreateRoom(pt.x, pt.y, w, h))
	{
		H(start_pt.x, start_pt.y) = DRZWI;
		Room* room = AddRoom(parent_room, pt.x, pt.y, w, h, ADD_RAMP);
		room->level = new_level;
		memcpy(room->yb, yb, sizeof(yb));

		if(IsFreeWay(pt.x - 1 + pt.y * opcje->w) != BRAK)
			free.push_back(pt.x - 1 + pt.y * opcje->w);
		if(IsFreeWay(pt.x + 1 + pt.y * opcje->w) != BRAK)
			free.push_back(pt.x + 1 + pt.y * opcje->w);
		if(IsFreeWay(pt.x + (pt.y - 1) * opcje->w) != BRAK)
			free.push_back(pt.x + (pt.y - 1) * opcje->w);
		if(IsFreeWay(pt.x + (pt.y + 1) * opcje->w) != BRAK)
			free.push_back(pt.x + (pt.y + 1) * opcje->w);

		return room;
	}

	return nullptr;
}

//=================================================================================================
Room* DungeonGenerator::CreateRoom(int id, int parent_room, Int2& pt, DIR dir)
{
	int w = opcje->room_size.Random(),
		h = opcje->room_size.Random();

	if(dir == LEWO)
	{
		pt.x -= w;
		pt.y -= h / 2;
	}
	else if(dir == PRAWO)
	{
		pt.x += 1;
		pt.y -= h / 2;
	}
	else if(dir == GORA)
	{
		pt.x -= w / 2;
		pt.y += 1;
	}
	else
	{
		pt.x -= w / 2;
		pt.y -= h;
	}

	if(CanCreateRoom(pt.x, pt.y, w, h))
	{
		// create 1 tile room for doors
		int door_room_id = opcje->rooms->size();
		Room& door_room = Add1(opcje->rooms);
		Room& p = opcje->rooms->at(parent_room);
		door_room.target = (p.target == RoomTarget::Corridor ? RoomTarget::CorridorDoors : RoomTarget::Doors);
		door_room.connected.push_back(parent_room);
		p.connected.push_back(door_room_id);
		door_room.pos.x = id % opcje->w;
		door_room.pos.y = id / opcje->w;
		door_room.size = Int2(1, 1);
		door_room.level = p.level;
		door_room.counter = p.counter;
		door_room.y = p.y;
		mapa[id].type = DRZWI;
		mapa[id].room = door_room_id;

		return AddRoom(door_room_id, pt.x, pt.y, w, h, ADD_ROOM);
	}
	else
		return nullptr;
}

//=================================================================================================
Room* DungeonGenerator::CreateCorridor(int parent_room, Int2& start_pt, DIR dir)
{
	int dl = opcje->corridor_size.Random();
	int w, h;
	Int2 pt(start_pt);

	if(dir == LEWO)
	{
		pt.x -= dl;
		w = dl;
		h = 1;
	}
	else if(dir == PRAWO)
	{
		pt.x += 1;
		w = dl;
		h = 1;
	}
	else if(dir == GORA)
	{
		pt.y += 1;
		w = 1;
		h = dl;
	}
	else
	{
		pt.y -= dl;
		w = 1;
		h = dl;
	}

	if(CanCreateRoom(pt.x, pt.y, w, h))
	{
		Room* room = AddRoom(parent_room, pt.x, pt.y, w, h, ADD_CORRIDOR);
		switch(dir)
		{
		case LEWO:
			++room->size.x;
			break;
		case PRAWO:
			--room->pos.x;
			++room->size.x;
			break;
		case DOL:
			++room->size.y;
			break;
		case GORA:
			--room->pos.y;
			++room->size.y;
			break;
		}
		Pole& p = F(start_pt.x, start_pt.y);
		p.type = DRZWI;
		p.room = word(opcje->rooms->size() - 1);
		return room;
	}

	return nullptr;
}

//=================================================================================================
bool DungeonGenerator::CanCreateRoom(int x, int y, int w, int h)
{
	--x;
	--y;
	w += 2;
	h += 2;

	if(!(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < int(opcje->w) && y + h < int(opcje->h)))
		return false;

	for(int yy = y + 1; yy < y + h - 1; ++yy)
	{
		for(int xx = x + 1; xx < x + w - 1; ++xx)
		{
			if(H(xx, yy) != NIEUZYTE)
				return false;
		}
	}

	for(int i = 0; i < w; ++i)
	{
		POLE p = H(x + i, y);
		if(p != NIEUZYTE && p != SCIANA)
			return false;
		p = H(x + i, y + h - 1);
		if(p != NIEUZYTE && p != SCIANA)
			return false;
	}

	for(int i = 0; i < h; ++i)
	{
		POLE p = H(x, y + i);
		if(p != NIEUZYTE && p != SCIANA)
			return false;
		p = H(x + w - 1, y + i);
		if(p != NIEUZYTE && p != SCIANA)
			return false;
	}

	return true;
}

//=================================================================================================
// Chance to remove doors between corridors
void DungeonGenerator::JoinCorridors()
{
	int index = 0;
	for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it, ++index)
	{
		if(it->target != RoomTarget::Corridor)
			continue;

		Room& r = *it;

		for(int index2 : r.connected)
		{
			if(index2 > index) // prevents double check
				continue;

			Room& r2 = opcje->rooms->at(index2);

			if(r2.target != RoomTarget::Corridor || Rand() % 100 > opcje->polacz_korytarz)
				continue;

			int x1 = max(r.pos.x - 1, r2.pos.x - 1),
				x2 = min(r.pos.x + r.size.x + 1, r2.pos.x + r2.size.x + 1),
				y1 = max(r.pos.y - 1, r2.pos.y - 1),
				y2 = min(r.pos.y + r.size.y + 1, r2.pos.y + r2.size.y + 1);

			assert(x1 < x2 && y1 < y2);

			bool removed = false;
			for(int y = y1; y < y2 && !removed; ++y)
			{
				for(int x = x1; x < x2; ++x)
				{
					Pole& po = mapa[x + y * opcje->w];
					if(po.type == DRZWI && (po.room == index || po.room == index2))
					{
						po.type = PUSTE;
						removed = true;
						break;
					}
				}
			}
		}
	}
}

//=================================================================================================
void DungeonGenerator::MarkCorridors()
{
	for(Room& room : *opcje->rooms)
	{
		if(room.IsRoom() || room.IsRamp() || room.target == RoomTarget::Doors)
			continue;

		for(int y = 0; y < room.size.y; ++y)
		{
			for(int x = 0; x < room.size.x; ++x)
			{
				Pole& p = mapa[x + room.pos.x + (y + room.pos.y)*opcje->w];
				if(p.type == PUSTE || p.type == DRZWI || p.type == KRATKA_PODLOGA)
					p.flags = Pole::F_NISKI_SUFIT;
			}
		}
	}
}

//=================================================================================================
void DungeonGenerator::CreateHoles()
{
	for(int y = 1; y < opcje->w - 1; ++y)
	{
		for(int x = 1; x < opcje->h - 1; ++x)
		{
			Pole& p = mapa[x + y * opcje->w];
			if(p.type == PUSTE && Rand() % 100 < opcje->kraty_szansa && !opcje->rooms->at(p.room).IsRamp())
			{
				if(!IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				{
					int j = Rand() % 3;
					if(j == 0)
						p.type = KRATKA_PODLOGA;
					else if(j == 1)
						p.type = KRATKA_SUFIT;
					else
						p.type = KRATKA;
				}
				else if(Rand() % 3 == 0)
					p.type = KRATKA_PODLOGA;
			}
		}
	}
}

//=================================================================================================
void DungeonGenerator::JoinRooms()
{
	int index = 0;
	for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it, ++index)
	{
		Room& r = *it;
		if(!r.CanJoinRoom())
			continue;

		for(int door_index : r.connected)
		{
			if(door_index > index) // prevents double check
				continue;

			Room& r_door = opcje->rooms->at(door_index);
			if(r_door.target != RoomTarget::Doors)
				continue;

			int index2;
			if(r_door.connected[0] == index)
				index2 = r_door.connected[1];
			else
				index2 = r_door.connected[0];
			Room& r2 = opcje->rooms->at(index2);
			if(!r2.CanJoinRoom() || Rand() % 100 > opcje->polacz_pokoj)
				continue;

			// znajdŸ wspólny obszar
			int x1 = max(r.pos.x - 1, r2.pos.x - 1),
				x2 = min(r.pos.x + r.size.x + 1, r2.pos.x + r2.size.x + 1),
				y1 = max(r.pos.y - 1, r2.pos.y - 1),
				y2 = min(r.pos.y + r.size.y + 1, r2.pos.y + r2.size.y + 1);

			if(x1 == 0)
				++x1;
			if(y1 == 0)
				++y1;
			if(x2 == opcje->w - 1)
				--x2;
			if(y2 == opcje->h - 1)
				--y2;
			
			assert(x1 < x2 && y1 < y2);

			for(int y = y1; y < y2; ++y)
			{
				for(int x = x1; x < x2; ++x)
				{
					if(IsConnectingWall(x, y, index, index2))
					{
						Pole& po = mapa[x + y * opcje->w];
						if(po.type == SCIANA || po.type == DRZWI)
						{
							po.type = PUSTE;
							po.room = door_index;
							if(x < r_door.pos.x)
							{
								r_door.size.x += (r_door.pos.x - x);
								r_door.pos.x = x;
							}
							else if(x >= r_door.pos.x + r_door.size.x)
								++r_door.size.x;
							if(y < r_door.pos.y)
							{
								r_door.size.y += (r_door.pos.y - y);
								r_door.pos.y = y;
							}
							else if(y >= r_door.pos.y + r_door.size.y)
								++r_door.size.y;
						}
					}
				}
			}
		}
	}
}

//=================================================================================================
bool IsFreeTile(POLE p)
{
	return (p == PUSTE || p == KRATKA || p == KRATKA_PODLOGA || p == KRATKA_SUFIT);
}

//=================================================================================================
bool DungeonGenerator::IsConnectingWall(int x, int y, int id1, int id2)
{
	if(mapa[x - 1 + y * opcje->w].room == id1 && IsFreeTile(mapa[x - 1 + y * opcje->w].type))
	{
		if(mapa[x + 1 + y * opcje->w].room == id2 && IsFreeTile(mapa[x + 1 + y * opcje->w].type))
			return true;
	}
	else if(mapa[x - 1 + y * opcje->w].room == id2 && IsFreeTile(mapa[x - 1 + y * opcje->w].type))
	{
		if(mapa[x + 1 + y * opcje->w].room == id1 && IsFreeTile(mapa[x + 1 + y * opcje->w].type))
			return true;
	}
	if(mapa[x + (y - 1)*opcje->w].room == id1 && IsFreeTile(mapa[x + (y - 1)*opcje->w].type))
	{
		if(mapa[x + (y + 1)*opcje->w].room == id2 && IsFreeTile(mapa[x + (y + 1)*opcje->w].type))
			return true;
	}
	else if(mapa[x + (y - 1)*opcje->w].room == id2 && IsFreeTile(mapa[x + (y - 1)*opcje->w].type))
	{
		if(mapa[x + (y + 1)*opcje->w].room == id1 && IsFreeTile(mapa[x + (y + 1)*opcje->w].type))
			return true;
	}
	return false;
}

//=================================================================================================
bool DungeonGenerator::GenerateStairs()
{
	assert((opcje->schody_dol != OpcjeMapy::BRAK || opcje->schody_gora != OpcjeMapy::BRAK) && opcje->rooms && opcje->mapa);

	static vector<Room*> rooms;
	for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it)
	{
		if(it->target == RoomTarget::None)
			rooms.push_back(&*it);
	}

	if(opcje->schody_gora != OpcjeMapy::BRAK)
	{
		bool inside_wall;
		if(!AddStairs(rooms, opcje->schody_gora, opcje->schody_gora_pokoj, opcje->schody_gora_pozycja, opcje->schody_gora_kierunek, true, inside_wall))
		{
			rooms.clear();
			return false;
		}
	}

	if(opcje->schody_dol != OpcjeMapy::BRAK)
	{
		if(!AddStairs(rooms, opcje->schody_dol, opcje->schody_dol_pokoj, opcje->schody_dol_pozycja, opcje->schody_dol_kierunek, false, opcje->schody_dol_w_scianie))
		{
			rooms.clear();
			return false;
		}
	}

	rooms.clear();
	return true;
}

//=================================================================================================
bool DungeonGenerator::AddStairs(vector<Room*>& rooms, OpcjeMapy::GDZIE_SCHODY where, Room*& room, Int2& pt, int& dir, bool up, bool& inside_wall)
{
	switch(where)
	{
	case OpcjeMapy::LOSOWO:
		while(!rooms.empty())
		{
			uint id = Rand() % rooms.size();
			Room* r = rooms.at(id);
			if(id != rooms.size() - 1)
				std::iter_swap(rooms.begin() + id, rooms.end() - 1);
			rooms.pop_back();

			if(CreateStairs(*r, pt, dir, (up ? SCHODY_GORA : SCHODY_DOL), inside_wall))
			{
				r->target = (up ? RoomTarget::StairsUp : RoomTarget::StairsDown);
				room = r;
				return true;
			}
		}
		assert(0);
		opcje->blad = BRAK_MIEJSCA;
		return false;
	case OpcjeMapy::NAJDALEJ:
		{
			vector<Int2> p;
			Int2 pos = rooms[0]->CenterTile();
			int index = 1;
			for(vector<Room*>::iterator it = rooms.begin() + 1, end = rooms.end(); it != end; ++it, ++index)
				p.push_back(Int2(index, Int2::Distance(pos, (*it)->CenterTile())));
			std::sort(p.begin(), p.end(), [](Int2& a, Int2& b)
			{
				return a.y < b.y;
			});

			while(!p.empty())
			{
				int p_id = p.back().x;
				p.pop_back();

				if(CreateStairs(*rooms[p_id], pt, dir, (up ? SCHODY_GORA : SCHODY_DOL), inside_wall))
				{
					rooms[p_id]->target = (up ? RoomTarget::StairsUp : RoomTarget::StairsDown);
					room = rooms[p_id];
					return true;
				}
			}

			assert(0);
			opcje->blad = BRAK_MIEJSCA;
			return false;
		}
		break;
	default:
		assert(0);
		opcje->blad = ZLE_DANE;
		return false;
	}
}

//=================================================================================================
// aktualnie najwy¿szy priorytet maj¹ schody w œcianie po œrodku œciany
struct PosDir
{
	Int2 pos;
	int dir, prio;
	bool inside_wall;

	PosDir(int x, int y, int dir, bool inside_wall, const Room& room) : pos(x, y), dir(dir), prio(0), inside_wall(inside_wall)
	{
		if(inside_wall)
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

//=================================================================================================
bool DungeonGenerator::CreateStairs(Room& room, Int2& pt, int& dir, POLE stairs_type, bool& inside_wall)
{
#define B(_xx,_yy) (czy_blokuje2(opcje->mapa[x+_xx+(y+_yy)*opcje->w].type))
#define P(_xx,_yy) (!czy_blokuje21(opcje->mapa[x+_xx+(y+_yy)*opcje->w].type))

	static vector<PosDir> wybor;
	wybor.clear();
	// im wiêkszy priorytet tym lepiej

	for(int y = max(1, room.pos.y - 1); y < min(opcje->h - 1, room.size.y + room.pos.y + 1); ++y)
	{
		for(int x = max(1, room.pos.x - 1); x < min(opcje->w - 1, room.size.x + room.pos.x + 1); ++x)
		{
			Pole& p = opcje->mapa[x + y * opcje->w];
			if(p.type == PUSTE && !opcje->rooms->at(p.room).IsRamp())
			{
				const bool left = (x > 0);
				const bool right = (x<int(opcje->w - 1));
				const bool top = (y<int(opcje->h - 1));
				const bool bottom = (y > 0);

				if(left && top)
				{
					// ##
					// #>
					if(B(-1, 1) && B(0, 1) && B(-1, 0))
						wybor.push_back(PosDir(x, y, BIT(0) | BIT(3), false, room));
				}
				if(right && top)
				{
					// ##
					// >#
					if(B(0, 1) && B(1, 1) && B(1, 0))
						wybor.push_back(PosDir(x, y, BIT(0) | BIT(1), false, room));
				}
				if(left && bottom)
				{
					// #>
					// ##
					if(B(-1, 0) && B(-1, -1) && B(0, -1))
						wybor.push_back(PosDir(x, y, BIT(2) | BIT(3), false, room));
				}
				if(right && bottom)
				{
					// <#
					// ##
					if(B(1, 0) && B(0, -1) && B(1, -1))
						wybor.push_back(PosDir(x, y, BIT(1) | BIT(2), false, room));
				}
				if(left && top && bottom)
				{
					// #_
					// #>
					// #_
					if(B(-1, 1) && P(0, 1) && B(-1, 0) && B(-1, -1) && P(0, -1))
						wybor.push_back(PosDir(x, y, BIT(0) | BIT(2) | BIT(3), false, room));
				}
				if(right && top && bottom)
				{
					// _#
					// <#
					// _#
					if(P(0, 1) && B(1, 1) && B(1, 0) && P(0, -1) && B(1, -1))
						wybor.push_back(PosDir(x, y, BIT(0) | BIT(1) | BIT(2), false, room));
				}
				if(top && left && right)
				{
					// ###
					// _>_
					if(B(-1, 1) && B(0, 1) && B(1, 1) && P(-1, 0) && P(1, 0))
						wybor.push_back(PosDir(x, y, BIT(0) | BIT(1) | BIT(3), false, room));
				}
				if(bottom && left && right)
				{
					// _>_
					// ###
					if(P(-1, 0) && P(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1))
						wybor.push_back(PosDir(x, y, BIT(1) | BIT(2) | BIT(3), false, room));
				}
				if(left && right && top && bottom)
				{
					//  ___
					//  _<_
					//  ___
					if(P(-1, -1) && P(0, -1) && P(1, -1) && P(-1, 0) && P(1, 0) && P(-1, 1) && P(0, 1) && P(1, 1))
						wybor.push_back(PosDir(x, y, BIT(0) | BIT(1) | BIT(2) | BIT(3), false, room));
				}
			}
			else if(p.type == SCIANA && (x > 0) && (x<int(opcje->w - 1)) && (y > 0) && (y<int(opcje->h - 1)))
			{
				// ##
				// #>_
				// ##
				if(B(-1, 1) && B(0, 1) && B(-1, 0) && P(1, 0) && B(-1, -1) && B(0, -1))
					wybor.push_back(PosDir(x, y, BIT(3), true, room));

				//  ##
				// _<#
				//  ##
				if(B(0, 1) && B(1, 1) && P(-1, 0) && B(1, 0) && B(0, -1) && B(1, -1))
					wybor.push_back(PosDir(x, y, BIT(1), true, room));

				// ###
				// #>#
				//  _
				if(B(-1, 1) && B(0, 1) && B(1, 1) && B(-1, 0) && B(1, 0) && P(0, -1))
					wybor.push_back(PosDir(x, y, BIT(0), true, room));

				//  _
				// #<#
				// ###
				if(P(0, 1) && B(-1, 0) && B(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1))
					wybor.push_back(PosDir(x, y, BIT(2), true, room));
			}
		}
	}

	if(wybor.empty())
		return false;

	std::sort(wybor.begin(), wybor.end());

	int best_prio = wybor.front().prio;
	int ile = 0;

	vector<PosDir>::iterator it = wybor.begin(), end = wybor.end();
	while(it != end && it->prio == best_prio)
	{
		++it;
		++ile;
	}

	PosDir& pd = wybor[Rand() % ile];
	pt = pd.pos;
	opcje->mapa[pd.pos.x + pd.pos.y*opcje->w].type = stairs_type;
	inside_wall = pd.inside_wall;

	for(int y = max(0, pd.pos.y - 1); y <= min(int(opcje->h), pd.pos.y + 1); ++y)
	{
		for(int x = max(0, pd.pos.x - 1); x <= min(int(opcje->w), pd.pos.x + 1); ++x)
		{
			POLE& p = opcje->mapa[x + y * opcje->w].type;
			if(p == NIEUZYTE)
				p = SCIANA;
		}
	}

	switch(pd.dir)
	{
	case 0:
		assert(0);
		return false;
	case BIT(0):
		dir = 0;
		break;
	case BIT(1):
		dir = 1;
		break;
	case BIT(2):
		dir = 2;
		break;
	case BIT(3):
		dir = 3;
		break;
	case BIT(0) | BIT(1):
		if(Rand() % 2 == 0)
			dir = 0;
		else
			dir = 1;
		break;
	case BIT(0) | BIT(2):
		if(Rand() % 2 == 0)
			dir = 0;
		else
			dir = 2;
		break;
	case BIT(0) | BIT(3):
		if(Rand() % 2 == 0)
			dir = 0;
		else
			dir = 3;
		break;
	case BIT(1) | BIT(2):
		if(Rand() % 2 == 0)
			dir = 1;
		else
			dir = 2;
		break;
	case BIT(1) | BIT(3):
		if(Rand() % 2 == 0)
			dir = 1;
		else
			dir = 3;
		break;
	case BIT(2) | BIT(3):
		if(Rand() % 2 == 0)
			dir = 2;
		else
			dir = 3;
		break;
	case BIT(0) | BIT(1) | BIT(2):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = 0;
			else if(t == 1)
				dir = 1;
			else
				dir = 2;
		}
		break;
	case BIT(0) | BIT(1) | BIT(3):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = 0;
			else if(t == 1)
				dir = 1;
			else
				dir = 3;
		}
		break;
	case BIT(0) | BIT(2) | BIT(3):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = 0;
			else if(t == 1)
				dir = 2;
			else
				dir = 3;
		}
		break;
	case BIT(1) | BIT(2) | BIT(3):
		{
			int t = Rand() % 3;
			if(t == 0)
				dir = 1;
			else if(t == 1)
				dir = 2;
			else
				dir = 3;
		}
		break;
	case BIT(0) | BIT(1) | BIT(2) | BIT(3):
		dir = Rand() % 4;
		break;
	default:
		assert(0);
		return false;
	}

	return true;

#undef B
#undef P
}
