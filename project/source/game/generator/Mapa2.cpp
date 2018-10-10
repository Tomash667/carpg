#include "Pch.h"
#include "GameCore.h"
#include "Mapa2.h"
#include "BitStreamFunc.h"
#include "SaveState.h"
#include "GameCommon.h"

const float Room::HEIGHT = 4.f;
const float Room::HEIGHT_LOW = 3.f;

//-----------------------------------------------------------------------------
// Kod b³êdu
#define ZLE_DANE 1
#define BRAK_MIEJSCA 2

//-----------------------------------------------------------------------------
namespace Mapa
{
	//-----------------------------------------------------------------------------
	// Kierunek
	enum DIR
	{
		DOL,
		LEWO,
		GORA,
		PRAWO,
		BRAK
	};

	//-----------------------------------------------------------------------------
	// Odwrotnoœæ kierunku
	const DIR odwrotnosc[5] = {
		GORA,
		PRAWO,
		DOL,
		LEWO,
		BRAK
	};

	//-----------------------------------------------------------------------------
	// Przesuniêcie dla tego kierunku
	const Int2 kierunek[5] = {
		Int2(0,-1), // DÓ£
		Int2(-1,0), // LEWO
		Int2(0,1), // GÓRA
		Int2(1,0), // PRAWO
		Int2(0,0) // BRAK
	};

	Tile* mapa;
	OpcjeMapy* opcje;
	vector<int> wolne;

	//-----------------------------------------------------------------------------
	// Opcje dodawania pokoju
	enum DODAJ
	{
		NIE_DODAWAJ,
		DODAJ_POKOJ,
		DODAJ_KORYTARZ
	};

	bool czy_sciana_laczaca(int x, int y, int id1, int id2);
	void dodaj_pokoj(int x, int y, int w, int h, DODAJ dodaj, int id = -1);
	void dodaj_pokoj(const Room& pokoj, int id = -1) { dodaj_pokoj(pokoj.pos.x, pokoj.pos.y, pokoj.size.x, pokoj.size.y, NIE_DODAWAJ, id); }
	void generuj();
	void oznacz_korytarze();
	void polacz_korytarze();
	void polacz_pokoje();
	bool sprawdz_pokoj(int x, int y, int w, int h);
	void stworz_korytarz(Int2& pt, DIR dir);
	void szukaj_polaczenia(int x, int y, int id);
	void ustaw_sciane(TILE_TYPE& pole);
	void ustaw_wzor();
	DIR wolna_droga(int id);

#define H(_x,_y) mapa[(_x)+(_y)*opcje->w].type
#define HR(_x,_y) mapa[(_x)+(_y)*opcje->w].room

	bool wolne_pole(TILE_TYPE p)
	{
		return (p == EMPTY || p == BARS || p == BARS_FLOOR || p == BARS_CEILING);
	}

	//=================================================================================================
	// Sprawdza czy dana œciana ³¹czy dwa pokoje (NS)
	//=================================================================================================
	bool czy_sciana_laczaca(int x, int y, int id1, int id2)
	{
		if(mapa[x - 1 + y*opcje->w].room == id1 && wolne_pole(mapa[x - 1 + y*opcje->w].type))
		{
			if(mapa[x + 1 + y*opcje->w].room == id2 && wolne_pole(mapa[x + 1 + y*opcje->w].type))
				return true;
		}
		else if(mapa[x - 1 + y*opcje->w].room == id2 && wolne_pole(mapa[x - 1 + y*opcje->w].type))
		{
			if(mapa[x + 1 + y*opcje->w].room == id1 && wolne_pole(mapa[x + 1 + y*opcje->w].type))
				return true;
		}
		if(mapa[x + (y - 1)*opcje->w].room == id1 && wolne_pole(mapa[x + (y - 1)*opcje->w].type))
		{
			if(mapa[x + (y + 1)*opcje->w].room == id2 && wolne_pole(mapa[x + (y + 1)*opcje->w].type))
				return true;
		}
		else if(mapa[x + (y - 1)*opcje->w].room == id2 && wolne_pole(mapa[x + (y - 1)*opcje->w].type))
		{
			if(mapa[x + (y + 1)*opcje->w].room == id1 && wolne_pole(mapa[x + (y + 1)*opcje->w].type))
				return true;
		}
		return false;
	}

	//=================================================================================================
	// Dodaje pokój/korytarz (NS)
	//=================================================================================================
	void dodaj_pokoj(int x, int y, int w, int h, DODAJ dodaj, int _id)
	{
		assert(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < int(opcje->w) && y + h < int(opcje->h));

		int id;
		if(dodaj == NIE_DODAWAJ)
			id = _id;
		else
			id = (int)opcje->rooms->size();

		for(int yy = y + 1; yy < y + h - 1; ++yy)
		{
			for(int xx = x + 1; xx < x + w - 1; ++xx)
			{
				assert(H(xx, yy) == UNUSED);
				H(xx, yy) = EMPTY;
				HR(xx, yy) = (word)id;
			}
		}

		for(int i = 0; i < w; ++i)
		{
			ustaw_sciane(H(x + i, y));
			HR(x + i, y) = (word)id;
			ustaw_sciane(H(x + i, y + h - 1));
			HR(x + i, y + h - 1) = (word)id;
		}

		for(int i = 0; i < h; ++i)
		{
			ustaw_sciane(H(x, y + i));
			HR(x, y + i) = (word)id;
			ustaw_sciane(H(x + w - 1, y + i));
			HR(x + w - 1, y + i) = (word)id;
		}

		for(int i = 1; i < w - 1; ++i)
		{
			if(wolna_droga(x + i + y*opcje->w) != BRAK)
				wolne.push_back(x + i + y*opcje->w);
			if(wolna_droga(x + i + (y + h - 1)*opcje->w) != BRAK)
				wolne.push_back(x + i + (y + h - 1)*opcje->w);
		}
		for(int i = 1; i < h - 1; ++i)
		{
			if(wolna_droga(x + (y + i)*opcje->w) != BRAK)
				wolne.push_back(x + (y + i)*opcje->w);
			if(wolna_droga(x + w - 1 + (y + i)*opcje->w) != BRAK)
				wolne.push_back(x + w - 1 + (y + i)*opcje->w);
		}

		if(dodaj != NIE_DODAWAJ)
		{
			Room& room = Add1(*opcje->rooms);
			room.pos.x = x;
			room.pos.y = y;
			room.size.x = w;
			room.size.y = h;
			room.target = (dodaj == DODAJ_KORYTARZ ? RoomTarget::Corridor : RoomTarget::None);
		}
	}

	//=================================================================================================
	// Generuje podziemia
	//=================================================================================================
	void generuj()
	{
		// ustaw wzór obszaru (np wype³nij obszar nieu¿ytkiem, na oko³o blokada)
		ustaw_wzor();

		// jeœli s¹ jakieœ pocz¹tkowe pokoje to je dodaj
		if(!opcje->rooms->empty())
		{
			int index = 0;
			for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it, ++index)
				dodaj_pokoj(*it, index);
		}
		else
		{
			// stwórz pocz¹tkowy pokój
			int w = opcje->rozmiar_pokoj.Random(),
				h = opcje->rozmiar_pokoj.Random();
			dodaj_pokoj((opcje->w - w) / 2, (opcje->h - h) / 2, w, h, DODAJ_POKOJ);
		}

		// generuj
		while(!wolne.empty())
		{
			int idx = Rand() % wolne.size();
			int id = wolne[idx];
			if(idx != wolne.size() - 1)
				std::iter_swap(wolne.begin() + idx, wolne.end() - 1);
			wolne.pop_back();

			// sprawdŸ czy to miejsce jest dobre
			DIR dir = wolna_droga(id);
			if(dir == BRAK)
				continue;

			Int2 pt(id%opcje->w, id / opcje->w);

			// pokój czy korytarz
			if(Rand() % 100 < opcje->korytarz_szansa)
				stworz_korytarz(pt, dir);
			else
			{
				int w = opcje->rozmiar_pokoj.Random(),
					h = opcje->rozmiar_pokoj.Random();

				if(dir == LEWO)
				{
					pt.x -= w - 1;
					pt.y -= h / 2;
				}
				else if(dir == PRAWO)
					pt.y -= h / 2;
				else if(dir == GORA)
					pt.x -= w / 2;
				else
				{
					pt.x -= w / 2;
					pt.y -= h - 1;
				}

				if(sprawdz_pokoj(pt.x, pt.y, w, h))
				{
					mapa[id].type = DOORS;
					dodaj_pokoj(pt.x, pt.y, w, h, DODAJ_POKOJ);
				}
			}

			//rysuj();
			//_getch();
		}

		// szukaj po³¹czeñ pomiêdzy pokojami/korytarzami
		int index = 0;
		for(Room& room : *opcje->rooms)
		{
			for(int x = 1; x < room.size.x - 1; ++x)
			{
				szukaj_polaczenia(room.pos.x + x, room.pos.y, index);
				szukaj_polaczenia(room.pos.x + x, room.pos.y + room.size.y - 1, index);
			}
			for(int y = 1; y < room.size.y - 1; ++y)
			{
				szukaj_polaczenia(room.pos.x, room.pos.y + y, index);
				szukaj_polaczenia(room.pos.x + room.size.x - 1, room.pos.y + y, index);
			}
			++index;
		}

		// po³¹cz korytarze
		if(opcje->polacz_korytarz > 0)
			polacz_korytarze();
		else
			oznacz_korytarze();

		// losowe dziury
		if(opcje->kraty_szansa > 0)
		{
			for(int y = 1; y < opcje->w - 1; ++y)
			{
				for(int x = 1; x < opcje->h - 1; ++x)
				{
					Tile& p = mapa[x + y*opcje->w];
					if(p.type == EMPTY && Rand() % 100 < opcje->kraty_szansa)
					{
						if(!IS_SET(p.flags, Tile::F_LOW_CEILING))
						{
							int j = Rand() % 3;
							if(j == 0)
								p.type = BARS_FLOOR;
							else if(j == 1)
								p.type = BARS_CEILING;
							else
								p.type = BARS;
						}
						else if(Rand() % 3 == 0)
							p.type = BARS_FLOOR;
					}
				}
			}
		}

		if(opcje->stop)
			return;

		// po³¹cz pokoje
		if(opcje->polacz_pokoj > 0)
			polacz_pokoje();

		// generowanie schodów
		if(opcje->schody_dol != OpcjeMapy::BRAK || opcje->schody_gora != OpcjeMapy::BRAK)
			generuj_schody(*opcje);

		// generuj flagi pól
		Tile::SetupFlags(opcje->mapa, Int2(opcje->w, opcje->h));
	}

	//=================================================================================================
	// Oznacza pola na mapie niskim sufitem tam gdzie jest korytarz
	//=================================================================================================
	void oznacz_korytarze()
	{
		for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it)
		{
			if(!it->IsCorridor())
				continue;

			for(int y = 0; y < it->size.y; ++y)
			{
				for(int x = 0; x < it->size.x; ++x)
				{
					Tile& p = mapa[x + it->pos.x + (y + it->pos.y)*opcje->w];
					if(p.type == EMPTY || p.type == DOORS || p.type == BARS_FLOOR)
						p.flags = Tile::F_LOW_CEILING;
				}
			}
		}
	}

	//=================================================================================================
	// £¹czy korytarze ze sob¹
	//=================================================================================================
	void polacz_korytarze()
	{
		int index = 0;
		for(vector<Room>::iterator it = opcje->rooms->begin(), end = opcje->rooms->end(); it != end; ++it, ++index)
		{
			if(!it->IsCorridor())
				continue;

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
						Tile& po = mapa[x + y*opcje->w];
						if(po.type == DOORS)
						{
							assert(po.room == index || po.room == *it2);
							po.type = EMPTY;
							goto usunieto_drzwi;
						}
					}
				}

				// brak po³¹czenia albo zosta³o ju¿ usuniête

			usunieto_drzwi:
				continue;
			}

			// oznacz niski sufit
			for(int y = 0; y < it->size.y; ++y)
			{
				for(int x = 0; x < it->size.x; ++x)
				{
					Tile& p = mapa[x + it->pos.x + (y + it->pos.y)*opcje->w];
					if(p.type == EMPTY || p.type == DOORS || p.type == BARS_FLOOR)
						p.flags = Tile::F_LOW_CEILING;
				}
			}
		}
	}

	//=================================================================================================
	// Usuwa róg z zaznaczenia przy ³¹czeniu pokoi
	//=================================================================================================
	void usun_rog(int& x1, int& x2, int& y1, int& y2, int x, int y)
	{
		if(x1 == x2)
		{
			// | y1
			// |
			// |
			// | y2
			// x1/x2
			if(x1 == x)
			{
				if(y1 == y)
					++y1;
				else if(y2 == y)
					--y2;
			}
		}
		else
		{
			// --------- y1/y2
			// x1      x2
			if(y1 == y)
			{
				if(x1 == x)
					++x1;
				else if(x2 == x)
					--x2;
			}
		}
	}

	//=================================================================================================
	// £¹czy pokoje ze sob¹
	//=================================================================================================
	void polacz_pokoje()
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

				// usuñ rogi
				/*usun_rog(x1, x2, y1, y2, p.pos.x, p.pos.y);
				usun_rog(x1, x2, y1, y2, p.pos.x+p.size.x, p.pos.y);
				usun_rog(x1, x2, y1, y2, p.pos.x, p.pos.y+p.size.y);
				usun_rog(x1, x2, y1, y2, p.pos.x+p.size.x, p.pos.y+p.size.y);
				usun_rog(x1, x2, y1, y2, p2.pos.x, p2.pos.y);
				usun_rog(x1, x2, y1, y2, p2.pos.x+p2.size.x, p2.pos.y);
				usun_rog(x1, x2, y1, y2, p2.pos.x, p2.pos.y+p2.size.y);
				usun_rog(x1, x2, y1, y2, p2.pos.x+p2.size.x, p2.pos.y+p2.size.y);*/

				assert(x1 < x2 && y1 < y2);

				for(int y = y1; y < y2; ++y)
				{
					for(int x = x1; x < x2; ++x)
					{
						if(czy_sciana_laczaca(x, y, index, *it2))
						{
							Tile& po = mapa[x + y*opcje->w];
							if(po.type == WALL || po.type == DOORS)
								po.type = EMPTY;
						}
					}
				}
			}
		}
	}

	//=================================================================================================
	// Sprawdza czy w tym miejscu mo¿na dodaæ korytarz
	//=================================================================================================
	bool sprawdz_pokoj(int x, int y, int w, int h)
	{
		if(!(x >= 0 && y >= 0 && w >= 3 && h >= 3 && x + w < int(opcje->w) && y + h < int(opcje->h)))
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
	// Tworzy korytarz
	//=================================================================================================
	void stworz_korytarz(Int2& _pt, DIR dir)
	{
		int dl = opcje->rozmiar_korytarz.Random();
		int w, h;

		Int2 pt(_pt);

		if(dir == LEWO)
		{
			pt.x -= dl - 1;
			pt.y -= 1;
			w = dl;
			h = 3;
		}
		else if(dir == PRAWO)
		{
			pt.y -= 1;
			w = dl;
			h = 3;
		}
		else if(dir == GORA)
		{
			pt.x -= 1;
			w = 3;
			h = dl;
		}
		else
		{
			pt.y -= dl - 1;
			pt.x -= 1;
			w = 3;
			h = dl;
		}

		if(sprawdz_pokoj(pt.x, pt.y, w, h))
		{
			H(_pt.x, _pt.y) = DOORS;
			dodaj_pokoj(pt.x, pt.y, w, h, DODAJ_KORYTARZ);
		}
	}

	//=================================================================================================
	// Szuka po³¹czañ pomiêdzy pomieszczeniami
	//=================================================================================================
	void szukaj_polaczenia(int x, int y, int id)
	{
		Room& r = opcje->rooms->at(id);

		if(H(x, y) == DOORS)
		{
			if(x > 0 && H(x - 1, y) == EMPTY)
			{
				int to_id = HR(x - 1, y);
				if(to_id != id)
				{
					bool jest = false;
					for(vector<int>::iterator it = r.connected.begin(), end = r.connected.end(); it != end; ++it)
					{
						if(*it == to_id)
						{
							jest = true;
							break;
						}
					}
					if(!jest)
					{
						r.connected.push_back(to_id);
						return;
					}
				}
			}

			if(x<int(opcje->w - 1) && H(x + 1, y) == EMPTY)
			{
				int to_id = HR(x + 1, y);
				if(to_id != id)
				{
					bool jest = false;
					for(vector<int>::iterator it = r.connected.begin(), end = r.connected.end(); it != end; ++it)
					{
						if(*it == to_id)
						{
							jest = true;
							break;
						}
					}
					if(!jest)
					{
						r.connected.push_back(to_id);
						return;
					}
				}
			}

			if(y > 0 && H(x, y - 1) == EMPTY)
			{
				int to_id = HR(x, y - 1);
				if(to_id != id)
				{
					bool jest = false;
					for(vector<int>::iterator it = r.connected.begin(), end = r.connected.end(); it != end; ++it)
					{
						if(*it == to_id)
						{
							jest = true;
							break;
						}
					}
					if(!jest)
					{
						r.connected.push_back(to_id);
						return;
					}
				}
			}

			if(y<int(opcje->h - 1) && H(x, y + 1) == EMPTY)
			{
				int to_id = HR(x, y + 1);
				if(to_id != id)
				{
					bool jest = false;
					for(vector<int>::iterator it = r.connected.begin(), end = r.connected.end(); it != end; ++it)
					{
						if(*it == to_id)
						{
							jest = true;
							break;
						}
					}
					if(!jest)
					{
						r.connected.push_back(to_id);
						return;
					}
				}
			}
		}
	}

	void ustaw_flagi()
	{
		for(int y = 0; y < opcje->h; ++y)
		{
			for(int x = 0; x < opcje->w; ++x)
			{
				Tile& p = mapa[x + y*opcje->w];
				if(p.type != EMPTY && p.type != DOORS && p.type != BARS && p.type != BARS_FLOOR && p.type != BARS_CEILING && p.type != STAIRS_DOWN)
					continue;

				// pod³oga
				if(p.type == BARS || p.type == BARS_FLOOR)
				{
					p.flags |= Tile::F_BARS_FLOOR;

					if(!OR2_EQ(mapa[x - 1 + y*opcje->w].type, BARS, BARS_FLOOR))
						p.flags |= Tile::F_HOLE_RIGHT;
					if(!OR2_EQ(mapa[x + 1 + y*opcje->w].type, BARS, BARS_FLOOR))
						p.flags |= Tile::F_HOLE_LEFT;
					if(!OR2_EQ(mapa[x + (y - 1)*opcje->w].type, BARS, BARS_FLOOR))
						p.flags |= Tile::F_HOLE_BACK;
					if(!OR2_EQ(mapa[x + (y + 1)*opcje->w].type, BARS, BARS_FLOOR))
						p.flags |= Tile::F_HOLE_FRONT;
				}
				else if(p.type != STAIRS_DOWN)
					p.flags |= Tile::F_FLOOR;

				if(p.type == BARS || p.type == BARS_CEILING)
					assert(!IS_SET(p.flags, Tile::F_LOW_CEILING));

				if(!IS_SET(p.flags, Tile::F_LOW_CEILING))
				{
					if(IS_SET(mapa[x - 1 + y*opcje->w].flags, Tile::F_LOW_CEILING))
						p.flags |= Tile::F_CEIL_RIGHT;
					if(IS_SET(mapa[x + 1 + y*opcje->w].flags, Tile::F_LOW_CEILING))
						p.flags |= Tile::F_CEIL_LEFT;
					if(IS_SET(mapa[x + (y - 1)*opcje->w].flags, Tile::F_LOW_CEILING))
						p.flags |= Tile::F_CEIL_BACK;
					if(IS_SET(mapa[x + (y + 1)*opcje->w].flags, Tile::F_LOW_CEILING))
						p.flags |= Tile::F_CEIL_FRONT;

					// dziura w suficie
					if(p.type == BARS || p.type == BARS_CEILING)
					{
						p.flags |= Tile::F_BARS_CEILING;

						if(!OR2_EQ(mapa[x - 1 + y*opcje->w].type, BARS, BARS_CEILING))
							p.flags |= Tile::F_UP_RIGHT;
						if(!OR2_EQ(mapa[x + 1 + y*opcje->w].type, BARS, BARS_CEILING))
							p.flags |= Tile::F_UP_LEFT;
						if(!OR2_EQ(mapa[x + (y - 1)*opcje->w].type, BARS, BARS_CEILING))
							p.flags |= Tile::F_UP_BACK;
						if(!OR2_EQ(mapa[x + (y + 1)*opcje->w].type, BARS, BARS_CEILING))
							p.flags |= Tile::F_UP_FRONT;
					}
					else
					{
						// normalny sufit w tym miejscu
						p.flags |= Tile::F_CEILING;
					}
				}

				// œciany
				if(OR2_EQ(mapa[x - 1 + y*opcje->w].type, WALL, BLOCKADE_WALL))
					p.flags |= Tile::F_WALL_RIGHT;
				if(OR2_EQ(mapa[x + 1 + y*opcje->w].type, WALL, BLOCKADE_WALL))
					p.flags |= Tile::F_WALL_LEFT;
				if(OR2_EQ(mapa[x + (y - 1)*opcje->w].type, WALL, BLOCKADE_WALL))
					p.flags |= Tile::F_WALL_BACK;
				if(OR2_EQ(mapa[x + (y + 1)*opcje->w].type, WALL, BLOCKADE_WALL))
					p.flags |= Tile::F_WALL_FRONT;
			}
		}
	}

	//=================================================================================================
	// Ustawia œcianê na danym polu
	//=================================================================================================
	void ustaw_sciane(TILE_TYPE& pole)
	{
		assert(pole == UNUSED || pole == WALL || pole == BLOCKADE || pole == BLOCKADE_WALL || pole == DOORS);

		if(pole == UNUSED)
			pole = WALL;
		else if(pole == BLOCKADE)
			pole = BLOCKADE_WALL;
	}

	//=================================================================================================
	// Ustawia wzór podziemi
	//=================================================================================================
	void ustaw_wzor()
	{
		memset(mapa, 0, sizeof(Tile)*opcje->w*opcje->h);

		if(opcje->ksztalt == OpcjeMapy::PROSTOKAT)
		{
			for(int x = 0; x < opcje->w; ++x)
			{
				H(x, 0) = BLOCKADE;
				H(x, opcje->h - 1) = BLOCKADE;
			}

			for(int y = 0; y < opcje->h; ++y)
			{
				H(0, y) = BLOCKADE;
				H(opcje->w - 1, y) = BLOCKADE;
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
						mapa[x + y*opcje->w].type = BLOCKADE;
				}
			}
		}
		else
		{
			assert(0);
		}
	}

	//=================================================================================================
	// Sprawdza czy w danym punkcie mo¿na dodaæ wejœcie do korytarza/pokoju
	//=================================================================================================
	DIR wolna_droga(int id)
	{
		int x = id%opcje->w,
			y = id / opcje->w;
		if(x == 0 || y == 0 || x == opcje->w - 1 || y == opcje->h - 1)
			return BRAK;

		DIR jest = BRAK;
		int count = 0;

		if(mapa[id - 1].type == EMPTY)
		{
			++count;
			jest = LEWO;
		}
		if(mapa[id + 1].type == EMPTY)
		{
			++count;
			jest = PRAWO;
		}
		if(mapa[id + opcje->w].type == EMPTY)
		{
			++count;
			jest = GORA;
		}
		if(mapa[id - opcje->w].type == EMPTY)
		{
			++count;
			jest = DOL;
		}

		if(count != 1)
			return BRAK;

		const Int2& od = kierunek[odwrotnosc[jest]];
		if(mapa[id + od.x + od.y*opcje->w].type != UNUSED)
			return BRAK;

		return odwrotnosc[jest];
	}

#undef H
#undef HR
};

//=================================================================================================
// Generuje mapê podziemi
//=================================================================================================
bool generuj_mape2(OpcjeMapy& _opcje, bool recreate)
{
	// sprawdŸ opcje
	assert(_opcje.w && _opcje.h && _opcje.rozmiar_pokoj.x >= 4 && _opcje.rozmiar_pokoj.y >= _opcje.rozmiar_pokoj.x &&
		InRange(_opcje.korytarz_szansa, 0, 100) && _opcje.rooms);
	assert(_opcje.korytarz_szansa == 0 || (_opcje.rozmiar_korytarz.x >= 3 && _opcje.rozmiar_korytarz.y >= _opcje.rozmiar_korytarz.x));

	if(!recreate)
		Mapa::mapa = new Tile[_opcje.w * _opcje.h];
	Mapa::opcje = &_opcje;
	Mapa::wolne.clear();

	_opcje.blad = 0;
	_opcje.mapa = Mapa::mapa;

	Mapa::generuj();

	if(_opcje.stop)
		return (_opcje.blad == 0);

	if(_opcje.devmode)
		Tile::DebugDraw(_opcje.mapa, Int2(_opcje.w, _opcje.h));

	return (_opcje.blad == 0);
}

bool kontynuuj_generowanie_mapy(OpcjeMapy& _opcje)
{
	// po³¹cz pokoje
	if(_opcje.polacz_pokoj > 0)
		Mapa::polacz_pokoje();

	// generowanie schodów
	if(_opcje.schody_dol != OpcjeMapy::BRAK || _opcje.schody_gora != OpcjeMapy::BRAK)
		generuj_schody(_opcje);

	// generuj flagi pól
	Mapa::ustaw_flagi();

	if(_opcje.devmode)
		Tile::DebugDraw(_opcje.mapa, Int2(_opcje.w, _opcje.h));

	return (_opcje.blad == 0);
}

// aktualnie najwy¿szy priorytet maj¹ schody w œcianie po œrodku œciany
struct PosDir
{
	Int2 pos;
	int dir, prio;
	bool w_scianie;

	PosDir(int _x, int _y, int _dir, bool _w_scianie, const Room& room) : pos(_x, _y), dir(_dir), prio(0), w_scianie(_w_scianie)
	{
		if(w_scianie)
			prio += (room.size.x + room.size.y) * 2;
		_x -= room.pos.x;
		_y -= room.pos.y;
		prio -= abs(room.size.x / 2 - _x) + abs(room.size.y / 2 - _y);
	}

	bool operator < (const PosDir& p)
	{
		return prio > p.prio;
	}
};

bool dodaj_schody(OpcjeMapy& _opcje, Room& room, Int2& _pozycja, GameDirection& _kierunek, TILE_TYPE _schody, bool& _w_scianie)
{
#define B(_xx,_yy) (IsBlocking(_opcje.mapa[x+_xx+(y+_yy)*_opcje.w].type))
#define P(_xx,_yy) (!IsBlocking2(_opcje.mapa[x+_xx+(y+_yy)*_opcje.w].type))

	static vector<PosDir> wybor;
	wybor.clear();
	// im wiêkszy priorytet tym lepiej

	for(int y = max(1, room.pos.y); y < min(_opcje.h - 1, room.size.y + room.pos.y); ++y)
	{
		for(int x = max(1, room.pos.x); x < min(_opcje.w - 1, room.size.x + room.pos.x); ++x)
		{
			Tile& p = _opcje.mapa[x + y*_opcje.w];
			if(p.type == EMPTY)
			{
				const bool left = (x > 0);
				const bool right = (x<int(_opcje.w - 1));
				const bool top = (y<int(_opcje.h - 1));
				const bool bottom = (y > 0);

				if(left && top)
				{
					// ##
					// #>
					if(B(-1, 1) && B(0, 1) && B(-1, 0))
						wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN) | BIT(GDIR_RIGHT), false, room));
				}
				if(right && top)
				{
					// ##
					// >#
					if(B(0, 1) && B(1, 1) && B(1, 0))
						wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN) | BIT(GDIR_LEFT), false, room));
				}
				if(left && bottom)
				{
					// #>
					// ##
					if(B(-1, 0) && B(-1, -1) && B(0, -1))
						wybor.push_back(PosDir(x, y, BIT(GDIR_UP) | BIT(GDIR_RIGHT), false, room));
				}
				if(right && bottom)
				{
					// <#
					// ##
					if(B(1, 0) && B(0, -1) && B(1, -1))
						wybor.push_back(PosDir(x, y, BIT(GDIR_LEFT) | BIT(GDIR_UP), false, room));
				}
				if(left && top && bottom)
				{
					// #_
					// #>
					// #_
					if(B(-1, 1) && P(0, 1) && B(-1, 0) && B(-1, -1) && P(0, -1))
						wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN) | BIT(GDIR_UP) | BIT(GDIR_RIGHT), false, room));
				}
				if(right && top && bottom)
				{
					// _#
					// <#
					// _#
					if(P(0, 1) && B(1, 1) && B(1, 0) && P(0, -1) && B(1, -1))
						wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN) | BIT(GDIR_LEFT) | BIT(GDIR_UP), false, room));
				}
				if(top && left && right)
				{
					// ###
					// _>_
					if(B(-1, 1) && B(0, 1) && B(1, 1) && P(-1, 0) && P(1, 0))
						wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN) | BIT(GDIR_LEFT) | BIT(GDIR_RIGHT), false, room));
				}
				if(bottom && left && right)
				{
					// _>_
					// ###
					if(P(-1, 0) && P(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1))
						wybor.push_back(PosDir(x, y, BIT(GDIR_LEFT) | BIT(GDIR_UP) | BIT(GDIR_RIGHT), false, room));
				}
				if(left && right && top && bottom)
				{
					//  ___
					//  _<_
					//  ___
					if(P(-1, -1) && P(0, -1) && P(1, -1) && P(-1, 0) && P(1, 0) && P(-1, 1) && P(0, 1) && P(1, 1))
						wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN) | BIT(GDIR_LEFT) | BIT(GDIR_UP) | BIT(GDIR_RIGHT), false, room));
				}
			}
			else if((p.type == WALL || p.type == BLOCKADE_WALL) && (x > 0) && (x<int(_opcje.w - 1)) && (y > 0) && (y<int(_opcje.h - 1)))
			{
				// ##
				// #>_
				// ##
				if(B(-1, 1) && B(0, 1) && B(-1, 0) && P(1, 0) && B(-1, -1) && B(0, -1))
					wybor.push_back(PosDir(x, y, BIT(GDIR_RIGHT), true, room));

				//  ##
				// _<#
				//  ##
				if(B(0, 1) && B(1, 1) && P(-1, 0) && B(1, 0) && B(0, -1) && B(1, -1))
					wybor.push_back(PosDir(x, y, BIT(GDIR_LEFT), true, room));

				// ###
				// #>#
				//  _
				if(B(-1, 1) && B(0, 1) && B(1, 1) && B(-1, 0) && B(1, 0) && P(0, -1))
					wybor.push_back(PosDir(x, y, BIT(GDIR_DOWN), true, room));

				//  _
				// #<#
				// ###
				if(P(0, 1) && B(-1, 0) && B(1, 0) && B(-1, -1) && B(0, -1) && B(1, -1))
					wybor.push_back(PosDir(x, y, BIT(GDIR_UP), true, room));
			}
		}
	}

	if(wybor.empty())
		return false;

	std::sort(wybor.begin(), wybor.end());

	int best_prio = wybor.front().prio;
	int count = 0;

	vector<PosDir>::iterator it = wybor.begin(), end = wybor.end();
	while(it != end && it->prio == best_prio)
	{
		++it;
		++count;
	}

	PosDir& pd = wybor[Rand() % count];
	_pozycja = pd.pos;
	_opcje.mapa[pd.pos.x + pd.pos.y*_opcje.w].type = _schody;
	_w_scianie = pd.w_scianie;

	for(int y = max(0, pd.pos.y - 1); y <= min(int(_opcje.h), pd.pos.y + 1); ++y)
	{
		for(int x = max(0, pd.pos.x - 1); x <= min(int(_opcje.w), pd.pos.x + 1); ++x)
		{
			TILE_TYPE& p = _opcje.mapa[x + y*_opcje.w].type;
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
	case BIT(GDIR_DOWN):
		_kierunek = GDIR_DOWN;
		break;
	case BIT(GDIR_LEFT):
		_kierunek = GDIR_LEFT;
		break;
	case BIT(GDIR_UP):
		_kierunek = GDIR_UP;
		break;
	case BIT(GDIR_RIGHT):
		_kierunek = GDIR_RIGHT;
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_LEFT):
		if(Rand() % 2 == 0)
			_kierunek = GDIR_DOWN;
		else
			_kierunek = GDIR_LEFT;
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_UP):
		if(Rand() % 2 == 0)
			_kierunek = GDIR_DOWN;
		else
			_kierunek = GDIR_UP;
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_RIGHT):
		if(Rand() % 2 == 0)
			_kierunek = GDIR_DOWN;
		else
			_kierunek = GDIR_RIGHT;
		break;
	case BIT(GDIR_LEFT) | BIT(GDIR_UP):
		if(Rand() % 2 == 0)
			_kierunek = GDIR_LEFT;
		else
			_kierunek = GDIR_UP;
		break;
	case BIT(GDIR_LEFT) | BIT(GDIR_RIGHT):
		if(Rand() % 2 == 0)
			_kierunek = GDIR_LEFT;
		else
			_kierunek = GDIR_RIGHT;
		break;
	case BIT(GDIR_UP) | BIT(GDIR_RIGHT):
		if(Rand() % 2 == 0)
			_kierunek = GDIR_UP;
		else
			_kierunek = GDIR_RIGHT;
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_LEFT) | BIT(GDIR_UP):
		{
			int t = Rand() % 3;
			if(t == 0)
				_kierunek = GDIR_DOWN;
			else if(t == 1)
				_kierunek = GDIR_LEFT;
			else
				_kierunek = GDIR_UP;
		}
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_LEFT) | BIT(GDIR_RIGHT):
		{
			int t = Rand() % 3;
			if(t == 0)
				_kierunek = GDIR_DOWN;
			else if(t == 1)
				_kierunek = GDIR_LEFT;
			else
				_kierunek = GDIR_RIGHT;
		}
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_UP) | BIT(GDIR_RIGHT):
		{
			int t = Rand() % 3;
			if(t == 0)
				_kierunek = GDIR_DOWN;
			else if(t == 1)
				_kierunek = GDIR_UP;
			else
				_kierunek = GDIR_RIGHT;
		}
		break;
	case BIT(GDIR_LEFT) | BIT(GDIR_UP) | BIT(GDIR_RIGHT):
		{
			int t = Rand() % 3;
			if(t == 0)
				_kierunek = GDIR_LEFT;
			else if(t == 1)
				_kierunek = GDIR_UP;
			else
				_kierunek = GDIR_RIGHT;
		}
		break;
	case BIT(GDIR_DOWN) | BIT(GDIR_LEFT) | BIT(GDIR_UP) | BIT(GDIR_RIGHT):
		_kierunek = (GameDirection)(Rand() % 4);
		break;
	default:
		assert(0);
		return false;
	}

	return true;

#undef B
#undef P
}

bool SortujPokoje(Int2& a, Int2& b)
{
	return a.y < b.y;
}

bool generuj_schody2(OpcjeMapy& _opcje, vector<Room*>& rooms, OpcjeMapy::GDZIE_SCHODY _gdzie, Room*& room, Int2& _pozycja, GameDirection& _kierunek, bool _gora, bool& _w_scianie)
{
	switch(_gdzie)
	{
	case OpcjeMapy::LOSOWO:
		while(!rooms.empty())
		{
			uint id = Rand() % rooms.size();
			Room* r = rooms.at(id);
			if(id != rooms.size() - 1)
				std::iter_swap(rooms.begin() + id, rooms.end() - 1);
			rooms.pop_back();

			if(dodaj_schody(_opcje, *r, _pozycja, _kierunek, (_gora ? STAIRS_UP : STAIRS_DOWN), _w_scianie))
			{
				r->target = (_gora ? RoomTarget::StairsUp : RoomTarget::StairsDown);
				room = r;
				return true;
			}
		}
		assert(0);
		_opcje.blad = BRAK_MIEJSCA;
		return false;
	case OpcjeMapy::NAJDALEJ:
		{
			vector<Int2> p;
			Int2 pos = rooms[0]->CenterTile();
			int index = 1;
			for(vector<Room*>::iterator it = rooms.begin() + 1, end = rooms.end(); it != end; ++it, ++index)
				p.push_back(Int2(index, Int2::Distance(pos, (*it)->CenterTile())));
			std::sort(p.begin(), p.end(), SortujPokoje);

			while(!p.empty())
			{
				int p_id = p.back().x;
				p.pop_back();

				if(dodaj_schody(_opcje, *rooms[p_id], _pozycja, _kierunek, (_gora ? STAIRS_UP : STAIRS_DOWN), _w_scianie))
				{
					rooms[p_id]->target = (_gora ? RoomTarget::StairsUp : RoomTarget::StairsDown);
					room = rooms[p_id];
					return true;
				}
			}

			assert(0);
			_opcje.blad = BRAK_MIEJSCA;
			return false;
		}
		break;
		/*case OpcjeMapy::KIERUNEK:
		case OpcjeMapy::POKOJ:
		case OpcjeMapy::PIERWSZY:
		case OpcjeMapy::NAJDALEJ:
			if(_gora)
			{
				assert(0);
				_opcje.blad = ZLE_DANE;
				return false;
			}
			else
			{
			}
			break;
		case OpcjeMapy::POZYCJA:*/
	default:
		assert(0);
		_opcje.blad = ZLE_DANE;
		return false;
	}
}

//=================================================================================================
// Generuje schody
//=================================================================================================
bool generuj_schody(OpcjeMapy& _opcje)
{
	assert((_opcje.schody_dol != OpcjeMapy::BRAK || _opcje.schody_gora != OpcjeMapy::BRAK) && _opcje.rooms && _opcje.mapa);

	static vector<Room*> rooms;
	for(vector<Room>::iterator it = _opcje.rooms->begin(), end = _opcje.rooms->end(); it != end; ++it)
	{
		if(it->target == RoomTarget::None)
			rooms.push_back(&*it);
	}

	if(_opcje.schody_gora != OpcjeMapy::BRAK)
	{
		bool w_scianie;
		if(!generuj_schody2(_opcje, rooms, _opcje.schody_gora, _opcje.schody_gora_pokoj, _opcje.schody_gora_pozycja, _opcje.schody_gora_kierunek, true, w_scianie))
		{
			rooms.clear();
			return false;
		}
	}

	if(_opcje.schody_dol != OpcjeMapy::BRAK)
	{
		if(!generuj_schody2(_opcje, rooms, _opcje.schody_dol, _opcje.schody_dol_pokoj, _opcje.schody_dol_pozycja, _opcje.schody_dol_kierunek, false, _opcje.schody_dol_w_scianie))
		{
			rooms.clear();
			return false;
		}
	}

	rooms.clear();
	return true;
}

inline bool IsInside(const Int2& pt, const Int2& start, const Int2& size)
{
	return (pt.x >= start.x && pt.y >= start.y && pt.x < start.x + size.x && pt.y < start.y + size.y);
}

// zwraca pole oznaczone ?
// ###########
// #    #    #
// # p1 ? p2 #
// #    #    #
// ###########
// jeœli jest kilka takich pól to zwraca pierwsze
Int2 pole_laczace(int pokoj1, int pokoj2)
{
	assert(pokoj1 >= 0 && pokoj2 >= 0 && max(pokoj1, pokoj2) < (int)Mapa::opcje->rooms->size());

	Room& r1 = Mapa::opcje->rooms->at(pokoj1);
	Room& r2 = Mapa::opcje->rooms->at(pokoj2);

	// sprawdŸ czy istnieje po³¹czenie
#ifdef _DEBUG
	for(vector<int>::iterator it = r1.connected.begin(), end = r1.connected.end(); it != end; ++it)
	{
		if(*it == pokoj2)
			goto ok;
	}
	assert(0 && "Pokoj1 i pokoj2 nie s¹ po³¹czone!");
ok:
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
			Tile& po = Mapa::mapa[x + y*Mapa::opcje->w];
			if(po.type == EMPTY || po.type == DOORS)
				return Int2(x, y);
		}
	}

	assert(0 && "Brak pola ³¹cz¹cego!");
	return Int2(-1, -1);
}
