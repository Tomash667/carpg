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
				mapa[id].type = DRZWI;
				AddRoom(parent_room, pt.x, pt.y, w, h, ADD_ROOM);
			}
		}
	}

	// szukaj po³¹czeñ pomiêdzy pokojami/korytarzami
	int index = 0;
	for(Room& room : *opcje->rooms)
	{
		for(int x = 1; x < room.size.x - 1; ++x)
		{
			SearchForConnection(room.pos.x + x, room.pos.y, index);
			SearchForConnection(room.pos.x + x, room.pos.y + room.size.y - 1, index);
		}
		for(int y = 1; y < room.size.y - 1; ++y)
		{
			SearchForConnection(room.pos.x, room.pos.y + y, index);
			SearchForConnection(room.pos.x + room.size.x - 1, room.pos.y + y, index);
		}
		++index;
	}

	// po³¹cz korytarze
	if(opcje->polacz_korytarz > 0)
		JoinCorridors();
	else
		MarkCorridors();

	// losowe dziury
	if(opcje->kraty_szansa > 0)
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
	assert(pole == NIEUZYTE || pole == SCIANA || pole == BLOKADA || pole == BLOKADA_SCIANA || pole == DRZWI);

	if(pole == NIEUZYTE)
		pole = SCIANA;
	else if(pole == BLOKADA)
		pole = BLOKADA_SCIANA;
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
		HR(x + i, y - 1) = (word)id;
		SetWall(H(x + i, y + h));
		HR(x + i, y + h) = (word)id;
	}
	for(int i = 0; i < h; ++i)
	{
		SetWall(H(x - 1, y + i));
		HR(x - 1, y + i) = (word)id;
		SetWall(H(x + w, y + i));
		HR(x + w, y + i) = (word)id;
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
Room* DungeonGenerator::CreateCorridor(int parent_room, Int2& start_pt, DIR dir)
{
	int dl = opcje->corridor_size.Random();
	int w, h;

	Int2 pt(start_pt);
	Int2 size_shift;

	if(dir == LEWO)
	{
		pt.x -= dl;
		w = dl;
		h = 1;
		size_shift = Int2(1, 0);
	}
	else if(dir == PRAWO)
	{
		pt.x += 1;
		w = dl;
		h = 1;
		size_shift = Int2(-1, 0);
	}
	else if(dir == GORA)
	{
		pt.y += 1;
		w = 1;
		h = dl;
		size_shift = Int2(-1, 0);
	}
	else
	{
		pt.y -= dl;
		w = 1;
		h = dl;
		size_shift = Int2(1, 0);
	}

	if(CanCreateRoom(pt.x, pt.y, w, h))
	{
		Room* room = AddRoom(parent_room, pt.x, pt.y, w, h, ADD_CORRIDOR);
		room->size += size_shift;
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
		if(p != NIEUZYTE && p != SCIANA && p != BLOKADA && p != BLOKADA_SCIANA)
			return false;
		p = H(x + i, y + h - 1);
		if(p != NIEUZYTE && p != SCIANA && p != BLOKADA && p != BLOKADA_SCIANA)
			return false;
	}

	for(int i = 0; i < h; ++i)
	{
		POLE p = H(x, y + i);
		if(p != NIEUZYTE && p != SCIANA && p != BLOKADA && p != BLOKADA_SCIANA)
			return false;
		p = H(x + w - 1, y + i);
		if(p != NIEUZYTE && p != SCIANA && p != BLOKADA && p != BLOKADA_SCIANA)
			return false;
	}

	return true;
}

//=================================================================================================
void DungeonGenerator::SearchForConnection(int x, int y, int id)
{
	if(H(x, y) != DRZWI)
		return;

	Room& r = opcje->rooms->at(id);

	if(x > 0 && H(x - 1, y) == PUSTE)
	{
		int to_id = HR(x - 1, y);
		if(to_id != id)
			SearchForConnection(r, to_id);
	}

	if(x<int(opcje->w - 1) && H(x + 1, y) == PUSTE)
	{
		int to_id = HR(x + 1, y);
		if(to_id != id)
			SearchForConnection(r, to_id);
	}

	if(y > 0 && H(x, y - 1) == PUSTE)
	{
		int to_id = HR(x, y - 1);
		if(to_id != id)
			SearchForConnection(r, to_id);
	}

	if(y<int(opcje->h - 1) && H(x, y + 1) == PUSTE)
	{
		int to_id = HR(x, y + 1);
		if(to_id != id)
			SearchForConnection(r, to_id);
	}
}

//=================================================================================================
void DungeonGenerator::SearchForConnection(Room& room, int id)
{
	Room& room2 = opcje->rooms->at(id);
	if(room.level != room2.level)
		return;

	bool added = false;
	for(vector<int>::iterator it = room.connected.begin(), end = room.connected.end(); it != end; ++it)
	{
		if(*it == id)
		{
			added = true;
			break;
		}
	}

	if(!added)
		room.connected.push_back(id);
}

//=================================================================================================
void DungeonGenerator::JoinCorridors()
{
	int index = 0;
	for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it, ++index)
	{
		if(!it->IsCorridorOrRamp())
			continue;

		if(it->IsCorridor())
		{
			Room& r = *it;

			for(vector<int>::iterator it2 = r.connected.begin(), end2 = r.connected.end(); it2 != end2; ++it2)
			{
				Room& r2 = opcje->rooms->at(*it2);

				if(!r2.IsCorridor() || Rand() % 100 >= opcje->polacz_korytarz)
					continue;

				int x1 = max(r.pos.x, r2.pos.x),
					x2 = min(r.pos.x + r.size.x, r2.pos.x + r2.size.x),
					y1 = max(r.pos.y, r2.pos.y),
					y2 = min(r.pos.y + r.size.y, r2.pos.y + r2.size.y);

				assert(x1 < x2 && y1 < y2);

				for(int y = y1; y < y2; ++y)
				{
					for(int x = x1; x < x2; ++x)
					{
						Pole& po = mapa[x + y * opcje->w];
						if(po.type == DRZWI)
						{
							assert(po.room == index || po.room == *it2);
							po.type = PUSTE;
							goto usunieto_drzwi;
						}
					}
				}

				// brak po³¹czenia albo zosta³o ju¿ usuniête

			usunieto_drzwi:
				continue;
			}
		}

		// oznacz niski sufit
		for(int y = 0; y < it->size.y; ++y)
		{
			for(int x = 0; x < it->size.x; ++x)
			{
				Pole& p = mapa[x + it->pos.x + (y + it->pos.y)*opcje->w];
				if(p.type == PUSTE || p.type == DRZWI || p.type == KRATKA_PODLOGA)
					p.flags = Pole::F_NISKI_SUFIT;
			}
		}
	}
}

//=================================================================================================
void DungeonGenerator::MarkCorridors()
{
	for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it)
	{
		if(!it->IsCorridorOrRamp())
			continue;

		for(int y = 0; y < it->size.y; ++y)
		{
			for(int x = 0; x < it->size.x; ++x)
			{
				Pole& p = mapa[x + it->pos.x + (y + it->pos.y)*opcje->w];
				if(p.type == PUSTE || p.type == DRZWI || p.type == KRATKA_PODLOGA)
					p.flags = Pole::F_NISKI_SUFIT;
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
		if(!it->CanJoinRoom())
			continue;

		Room& r = *it;

		for(vector<int>::iterator it2 = r.connected.begin(), end2 = r.connected.end(); it2 != end2; ++it2)
		{
			Room& r2 = opcje->rooms->at(*it2);

			if(!r2.CanJoinRoom() || Rand() % 100 >= opcje->polacz_pokoj)
				continue;

			// znajdŸ wspólny obszar
			int x1 = max(r.pos.x, r2.pos.x),
				x2 = min(r.pos.x + r.size.x, r2.pos.x + r2.size.x),
				y1 = max(r.pos.y, r2.pos.y),
				y2 = min(r.pos.y + r.size.y, r2.pos.y + r2.size.y);

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
					if(IsConnectingWall(x, y, index, *it2))
					{
						Pole& po = mapa[x + y * opcje->w];
						if(po.type == SCIANA || po.type == DRZWI)
							po.type = PUSTE;
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
// zwraca pole oznaczone ?
// ###########
// #    #    #
// # p1 ? p2 #
// #    #    #
// ###########
// jeœli jest kilka takich pól to zwraca pierwsze
Int2 DungeonGenerator::GetConnectingTile(int room1, int room2)
{
	assert(room1 >= 0 && room2 >= 0 && max(room1, room2) < (int)opcje->rooms->size());

	Room& r1 = opcje->rooms->at(room1);
	Room& r2 = opcje->rooms->at(room2);

	// sprawdŸ czy istnieje po³¹czenie
#ifdef _DEBUG
	bool ok = false;
	for(vector<int>::iterator it = r1.connected.begin(), end = r1.connected.end(); it != end; ++it)
	{
		if(*it == room2)
		{
			ok = true;
			break;
		}
	}
	assert(ok && "Rooms aren't connected!");
#endif

	// znajdŸ wspólne pola
	int x1 = max(r1.pos.x, r2.pos.x),
		x2 = min(r1.pos.x + r1.size.x, r2.pos.x + r2.size.x),
		y1 = max(r1.pos.y, r2.pos.y),
		y2 = min(r1.pos.y + r1.size.y, r2.pos.y + r2.size.y);

	assert(x1 < x2 && y1 < y2);

	for(int y = y1; y < y2; ++y)
	{
		for(int x = x1; x < x2; ++x)
		{
			Pole& po = mapa[x + y * opcje->w];
			if(po.type == PUSTE || po.type == DRZWI)
				return Int2(x, y);
		}
	}

	assert(0 && "Brak pola ³¹cz¹cego!");
	return Int2(-1, -1);
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

	for(int y = max(1, room.pos.y); y < min(opcje->h - 1, room.size.y + room.pos.y); ++y)
	{
		for(int x = max(1, room.pos.x); x < min(opcje->w - 1, room.size.x + room.pos.x); ++x)
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
			else if((p.type == SCIANA || p.type == BLOKADA_SCIANA) && (x > 0) && (x<int(opcje->w - 1)) && (y > 0) && (y<int(opcje->h - 1)))
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
			else if(p == BLOKADA)
				p = BLOKADA_SCIANA;
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
