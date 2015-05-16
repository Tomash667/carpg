// dane poziomu lokacji
#include "Pch.h"
#include "Base.h"
#include "InsideLocationLevel.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
InsideLocationLevel::~InsideLocationLevel()
{
	delete[] mapa;
	DeleteElements(units);
	DeleteElements(chests);
	DeleteElements(doors);
	DeleteElements(useables);
	DeleteElements(items);
	DeleteElements(traps);
}

//=================================================================================================
Pokoj* InsideLocationLevel::GetNearestRoom(const VEC3& _pos)
{
	if(pokoje.empty())
		return NULL;

	float dist, best_dist = 1000.f;
	Pokoj* best_room = NULL;

	for(vector<Pokoj>::iterator it = pokoje.begin(), end = pokoje.end(); it != end; ++it)
	{
		dist = it->Distance(_pos);
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
Pokoj* InsideLocationLevel::FindEscapeRoom(const VEC3& _my_pos, const VEC3& _enemy_pos)
{
	Pokoj* my_room = GetNearestRoom(_my_pos),
		* enemy_room = GetNearestRoom(_enemy_pos);

	if(!my_room)
		return NULL;

	int id;
	if(enemy_room)
		id = GetRoomId(enemy_room);
	else
		id = -1;

	Pokoj* best_room = NULL;
	float best_dist = 0.f, dist;
	VEC3 mid;

	for(vector<int>::iterator it = my_room->polaczone.begin(), end = my_room->polaczone.end(); it != end; ++it)
	{
		if(*it == id)
			continue;

		mid = pokoje[*it].Srodek();

		dist = distance(_my_pos, mid) - distance(_enemy_pos, mid);
		if(dist < best_dist)
		{
			best_dist = dist;
			best_room = &pokoje[*it];
		}
	}

	return best_room;
}

//=================================================================================================
Pokoj* InsideLocationLevel::GetRoom(const INT2& pt)
{
	word pokoj = mapa[pt.x+pt.y*w].pokoj;
	if(pokoj == (word)-1)
		return NULL;
	return &pokoje[pokoj];
}

//=================================================================================================
bool InsideLocationLevel::GetRandomNearWallTile(const Pokoj& _pokoj, INT2& _tile, int& _rot, bool nocol)
{
	_rot = rand2()%4;

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
				_tile.x = random(_pokoj.pos.x+1, _pokoj.pos.x+_pokoj.size.x-2);
				_tile.y = _pokoj.pos.y+1;

				if(czy_blokuje2(mapa[_tile.x+(_tile.y-1)*w]) && !czy_blokuje21(mapa[_tile.x+_tile.y*w]) && (nocol || !czy_blokuje21(mapa[_tile.x+(_tile.y+1)*w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		case 1:
			// prawa œciana, obj <
			do 
			{
				_tile.x = _pokoj.pos.x+_pokoj.size.x-2;
				_tile.y = random(_pokoj.pos.y+1, _pokoj.pos.y+_pokoj.size.y-2);

				if(czy_blokuje2(mapa[_tile.x+1+_tile.y*w]) && !czy_blokuje21(mapa[_tile.x+_tile.y*w]) && (nocol || !czy_blokuje21(mapa[_tile.x-1+_tile.y*w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		case 0:
			// dolna œciana, obj /|
			do 
			{
				_tile.x = random(_pokoj.pos.x+1, _pokoj.pos.x+_pokoj.size.x-2);
				_tile.y = _pokoj.pos.y+_pokoj.size.y-2;

				if(czy_blokuje2(mapa[_tile.x+(_tile.y+1)*w]) && !czy_blokuje21(mapa[_tile.x+_tile.y*w]) && (nocol || !czy_blokuje21(mapa[_tile.x+(_tile.y-1)*w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		case 3:
			// lewa œciana, obj >
			do 
			{
				_tile.x = _pokoj.pos.x+1;
				_tile.y = random(_pokoj.pos.y+1, _pokoj.pos.y+_pokoj.size.y-2);

				if(czy_blokuje2(mapa[_tile.x-1+_tile.y*w]) && !czy_blokuje21(mapa[_tile.x+_tile.y*w]) && (nocol || !czy_blokuje21(mapa[_tile.x+1+_tile.y*w])))
					return true;

				--tries2;
			}
			while(tries2 > 0);
			break;
		}

		++tries;
		_rot = (_rot+1)%4;
	}
	while(tries <= 3);

	return false;
}

//=================================================================================================
void InsideLocationLevel::SaveLevel(HANDLE file, bool local)
{
	WriteFile(file, &w, sizeof(w), &tmp, NULL);
	WriteFile(file, &h, sizeof(h), &tmp, NULL);
	WriteFile(file, mapa, sizeof(Pole)*w*h, &tmp, NULL);

	uint ile;

	// jednostki
	ile = units.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		(*it)->Save(file, local);

	// skrzynie
	ile = chests.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
		(*it)->Save(file, local);

	// obiekty
	ile = objects.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		it->Save(file);

	// drzwi
	ile = doors.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
		(*it)->Save(file, local);

	// przedmioty
	ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		(*it)->Save(file);

	// u¿ywalne
	ile = useables.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		(*it)->Save(file, local);

	// krew
	File f(file);
	ile = bloods.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Save(f);

	// œwiat³a
	f << lights.size();
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Save(f);

	// pokoje
	ile = pokoje.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Pokoj>::iterator it = pokoje.begin(), end = pokoje.end(); it != end; ++it)
		it->Save(file);

	// pu³apki
	ile = traps.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Trap*>::iterator it = traps.begin(), end = traps.end(); it != end; ++it)
		(*it)->Save(file, local);

	WriteFile(file, &schody_gora, sizeof(schody_gora), &tmp, NULL);
	WriteFile(file, &schody_dol, sizeof(schody_dol), &tmp, NULL);
	WriteFile(file, &schody_gora_dir, sizeof(schody_gora_dir), &tmp, NULL);
	WriteFile(file, &schody_dol_dir, sizeof(schody_dol_dir), &tmp, NULL);
	WriteFile(file, &schody_dol_w_scianie, sizeof(schody_dol_w_scianie), &tmp, NULL);
}

//=================================================================================================
void InsideLocationLevel::LoadLevel(HANDLE file, bool local)
{
	ReadFile(file, &w, sizeof(w), &tmp, NULL);
	ReadFile(file, &h, sizeof(h), &tmp, NULL);
	mapa = new Pole[w*h];
	ReadFile(file, mapa, sizeof(Pole)*w*h, &tmp, NULL);

	if(LOAD_VERSION == V_0_2)
	{
		for(int i=0; i<w*h; ++i)
		{
			if(mapa[i].co >= KRATKA_PODLOGA)
				mapa[i].co = (POLE)(mapa[i].co+1);
		}
	}

	// jednostki
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	units.resize(ile);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		*it = new Unit;
		Unit::AddRefid(*it);
		(*it)->Load(file, local);
	}

	// skrzynie
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	chests.resize(ile);
	for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
	{
		*it = new Chest;
		(*it)->Load(file, local);
	}

	// obiekty
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	objects.resize(ile);
	int index = 0;
	static vector<int> objs_need_update;
	for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it, ++index)
	{
		if(!it->Load(file))
			objs_need_update.push_back(index);
	}

	// drzwi
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	doors.resize(ile);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
	{
		*it = new Door;
		(*it)->Load(file, local);
	}

	// przedmioty
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	items.resize(ile);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		*it = new GroundItem;
		(*it)->Load(file);
	}

	// u¿ywalne
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	useables.resize(ile);
	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
	{
		*it = new Useable;
		Useable::AddRefid(*it);
		(*it)->Load(file, local);
	}

	// krew
	File f(file);
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	bloods.resize(ile);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Load(f);

	// œwiat³a
	f >> ile;
	lights.resize(ile);
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Load(f);

	// pokoje
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	pokoje.resize(ile);
	for(vector<Pokoj>::iterator it = pokoje.begin(), end = pokoje.end(); it != end; ++it)
		it->Load(file);

	// pu³apki
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	traps.resize(ile);
	for(vector<Trap*>::iterator it = traps.begin(), end = traps.end(); it != end; ++it)
	{
		*it = new Trap;
		(*it)->Load(file, local);
	}

	ReadFile(file, &schody_gora, sizeof(schody_gora), &tmp, NULL);
	ReadFile(file, &schody_dol, sizeof(schody_dol), &tmp, NULL);
	ReadFile(file, &schody_gora_dir, sizeof(schody_gora_dir), &tmp, NULL);
	ReadFile(file, &schody_dol_dir, sizeof(schody_dol_dir), &tmp, NULL);
	ReadFile(file, &schody_dol_w_scianie, sizeof(schody_dol_w_scianie), &tmp, NULL);

	// aktualizuj obiekty
	if(!objs_need_update.empty())
	{
		for(vector<int>::reverse_iterator it = objs_need_update.rbegin(), end = objs_need_update.rend(); it != end; ++it)
		{
			Object& o = objects[*it];
			Useable* u = new Useable;
			u->pos = o.pos;
			u->rot = o.rot.y;
			u->user = NULL;
			u->netid = Game::Get().useable_netid_counter++;
			if(IS_SET(o.base->flagi, OBJ_ZYLA_ZELAZA))
				u->type = U_ZYLA_ZELAZA;
			else
				u->type = U_ZYLA_ZLOTA;
			useables.push_back(u);

			objects.erase(objects.begin()+*it);
		}

		objs_need_update.clear();
	}

	// konwersja krzese³ w sto³ki
	if(LOAD_VERSION < V_0_2_12)
	{
		for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		{
			Useable& u = **it;
			if(u.type == U_KRZESLO)
				u.type = U_STOLEK;
		}
	}

	// konwersja ³awy w obrócon¹ ³awê i ustawienie wariantu
	if(LOAD_VERSION < V_0_2_20)
	{
		for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		{
			Useable& u = **it;
			if(u.type == U_LAWA)
			{
				u.type = U_LAWA_DIR;
				u.variant = rand2()%2;
			}
		}
	}
}

//=================================================================================================
Pokoj& InsideLocationLevel::GetFarRoom(bool have_down_stairs)
{
	if(have_down_stairs)
	{
		Pokoj* gora = GetNearestRoom(VEC3(2.f*schody_gora.x+1,0,2.f*schody_gora.y+1));
		Pokoj* dol = GetNearestRoom(VEC3(2.f*schody_dol.x+1,0,2.f*schody_dol.y+1));
		int best_dist, dist;
		Pokoj* best = NULL;

		for(vector<Pokoj>::iterator it = pokoje.begin(), end = pokoje.end(); it != end; ++it)
		{
			if(it->korytarz)
				continue;
			dist = distance(it->pos, gora->pos) + distance(it->pos, dol->pos);
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
		Pokoj* gora = GetNearestRoom(VEC3(2.f*schody_gora.x+1,0,2.f*schody_gora.y+1));
		int best_dist, dist;
		Pokoj* best = NULL;

		for(vector<Pokoj>::iterator it = pokoje.begin(), end = pokoje.end(); it != end; ++it)
		{
			if(it->korytarz)
				continue;
			dist = distance(it->pos, gora->pos);
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

	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
	{
		(*it)->refid = (int)Useable::refid_table.size();
		Useable::refid_table.push_back(*it);
	}
}

//=================================================================================================
int InsideLocationLevel::FindRoomId(int cel)
{
	int index = 0;
	for(vector<Pokoj>::iterator it = pokoje.begin(), end = pokoje.end(); it != end; ++it, ++index)
	{
		if(it->cel == cel)
			return index;
	}

	return -1;
}

//=================================================================================================
bool InsideLocationLevel::IsTileNearWall(const INT2& pt) const
{
	assert(pt.x > 0 && pt.y > 0 && pt.x < w-1 && pt.y < h-1);

	return mapa[pt.x-1+pt.y*w].IsWall() ||
		   mapa[pt.x+1+pt.y*w].IsWall() ||
		   mapa[pt.x+(pt.y-1)*w].IsWall() ||
		   mapa[pt.x+(pt.y+1)*w].IsWall();
}

//=================================================================================================
bool InsideLocationLevel::IsTileNearWall(const INT2& pt, int& dir) const
{
	assert(pt.x > 0 && pt.y > 0 && pt.x < w-1 && pt.y < h-1);

	int kierunek = 0;

	if(mapa[pt.x-1+pt.y*w].IsWall())
		kierunek |= (1<<GDIR_LEFT);
	if(mapa[pt.x+1+pt.y*w].IsWall())
		kierunek |= (1<<GDIR_RIGHT);
	if(mapa[pt.x+(pt.y-1)*w].IsWall())
		kierunek |= (1<<GDIR_DOWN);
	if(mapa[pt.x+(pt.y+1)*w].IsWall())
		kierunek |= (1<<GDIR_UP);

	if(kierunek == 0)
		return false;

	int i = rand2()%4;
	while(true)
	{
		if(IS_SET(kierunek, 1<<i))
		{
			dir = i;
			return true;
		}
		i = (++i)%4;
	}

	return true;
}
