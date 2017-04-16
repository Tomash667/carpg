#pragma once

//-----------------------------------------------------------------------------
#include "Light.h"

/* BUDOWA MAPY

 h = 5+- - - - - +
     4|# # # # # |
     3|# # # # # |
     2|# # # # # |
     1|# # # # # |
  +  0|# # # # # |
  Y   +- - - - - +
  -    0 1 2 3 4 5 = w
     -X+

Do zrobienia w przysz�o�ci:
+ okre�lony uk�ad mapy, pocz�tkowe niekt�re pola s� zablokowane np. ko�o, krzy�
+ tajne drzwi tam gdzie nie ma drzwi na ko�cu korytarza
+ tajna �ciana do zniszczenia na ko�cu korytarza
+ usuwanie niekt�rych mur�w ��cz�cych pomieszczenia
+ okr�g�e pomieszczenia
+ drzwi ukryte za np gobelinem
+ szansa na kontynuowanie korytarza
+ usuwanie �lepych uliczek

*/

//-----------------------------------------------------------------------------
// Rodzaj pola mapy
enum POLE : byte
{
	NIEUZYTE,
	PUSTE,
	SCHODY_GORA,
	SCHODY_DOL,
	DRZWI,
	OTWOR_NA_DRZWI,
	KRATKA_PODLOGA,
	KRATKA_SUFIT,
	KRATKA,
	SCIANA,
	BLOKADA,
	BLOKADA_SCIANA,
	ZAJETE
};

//-----------------------------------------------------------------------------
// Struktura opisuj�ca pole na mapie
struct Pole
{
	//-----------------------------------------------------------------------------
	// Flagi pola
	enum FLAGI
	{
		F_PODLOGA        = 0x1,
		F_SUFIT          = 0x2,
		F_NISKI_SUFIT    = 0x4,
		F_KRATKA_PODLOGA = 0x8,
		F_KRATKA_SUFIT   = 0x10,

		// unused 0x20 0x40 0x80

		F_SCIANA_LEWA  = 0x100,
		F_SCIANA_PRAWA = 0x200,
		F_SCIANA_PRZOD = 0x400,
		F_SCIANA_TYL   = 0x800,

		F_PODSUFIT_LEWA  = 0x1000,
		F_PODSUFIT_PRAWA = 0x2000,
		F_PODSUFIT_PRZOD = 0x4000,
		F_PODSUFIT_TYL   = 0x8000,

		F_GORA_LEWA  = 0x10000,
		F_GORA_PRAWA = 0x20000,
		F_GORA_PRZOD = 0x40000,
		F_GORA_TYL   = 0x80000,

		F_DZIURA_LEWA  = 0x100000,
		F_DZIURA_PRAWA = 0x200000,
		F_DZIURA_PRZOD = 0x400000,
		F_DZIURA_TYL   = 0x800000, // 1<<21

		F_SPECJALNE = 1<<29, // p�ki co u�ywane do oznaczenia drzwi do wi�zienia
		F_DRUGA_TEKSTURA = 1<<30,
		F_ODKRYTE = 1<<31
	};

	int flags;
	word room;
	POLE type;
	// jeszcze jest 1-2 bajty miejsca na co� :o (jak pok�j b�dzie byte)

	// DDDDGGGGRRRRSSSS000KKNSP
	// ####    ####    ########
	//     ####    ####
	// �ciany >> 8
	// podsufit >> 12
	// g�ra >> 16
	// d� >> 20

	bool IsWall() const
	{
		return type == SCIANA || type == BLOKADA_SCIANA;
	}
};

//-----------------------------------------------------------------------------
// Czy pole blokuje ruch
inline bool czy_blokuje2(POLE p)
{
	return p >= SCIANA || p == NIEUZYTE;
}
inline bool czy_blokuje2(const Pole& p)
{
	return czy_blokuje2(p.type);
}
inline bool czy_blokuje21(POLE p)
{
	// czy to pole blokuje ruch albo nie powinno na nim by� obiekt�w bo zablokuj� droge
	return !(p == PUSTE || p == KRATKA || p == KRATKA_PODLOGA || p == KRATKA_SUFIT);
}
inline bool czy_blokuje21(const Pole& p)
{
	return czy_blokuje21(p.type);
}

//-----------------------------------------------------------------------------
// Room target
enum class RoomTarget
{
	None,
	Corridor,
	StairsUp,
	StairsDown,
	Treasury,
	Portal,
	Prison,
	Throne,
	PortalCreate
};

//-----------------------------------------------------------------------------
// Struktura opisuj�ca pomieszczenie w podziemiach
struct Room
{
	INT2 pos, size;
	vector<int> connected;
	RoomTarget target;

	static const int MIN_SIZE = 19;

	VEC3 Center() const
	{
		return VEC3(float(pos.x*2+size.x),0,float(pos.y*2+size.y));
	}
	INT2 CenterTile() const
	{
		return pos + size/2;
	}
	bool IsInside(float x, float z) const
	{
		return (x >= 2.f*pos.x && z >= 2.f*pos.y && x <= 2.f*(pos.x + size.x) && z <= 2.f*(pos.y + size.y));
	}
	bool IsInside(const VEC3& _pos) const
	{
		return IsInside(_pos.x, _pos.z);
	}
	float Distance(const VEC3& _pos) const
	{
		if(IsInside(_pos))
			return 0.f;
		else
			return distance2d(_pos, Center());
	}
	float Distance(const Room& room) const
	{
		return distance2d(Center(), room.Center());
	}
	VEC3 GetRandomPos() const
	{
		return VEC3(random(2.f*(pos.x+1), 2.f*(pos.x+size.x-1)), 0, random(2.f*(pos.y+1), 2.f*(pos.y+size.y-1)));
	}
	VEC3 GetRandomPos(float margin) const
	{
		return VEC3(
			random(2.f*(pos.x+1) + margin, 2.f*(pos.x+size.x-1) - margin),
			0,
			random(2.f*(pos.y+1) + margin, 2.f*(pos.y+size.y-1) - margin));
	}

	bool IsCorridor() const { return target == RoomTarget::Corridor; }
	bool CanJoinRoom() const { return target == RoomTarget::None || target == RoomTarget::StairsUp || target == RoomTarget::StairsUp; }

	void Save(HANDLE file);
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);
};

//-----------------------------------------------------------------------------
// Opcje mapy
struct OpcjeMapy
{
	//-----------------------------------------------------------------------------
	// Kszta�t mapy podziemi
	enum KSZTALT_MAPY
	{
		PROSTOKAT,
		OKRAG
	};

	//-----------------------------------------------------------------------------
	// Gdzie stworzy� schody?
	enum GDZIE_SCHODY
	{
		BRAK,
		LOSOWO,
		NAJDALEJ, // schody s� jak najdalej od zerowego pokoju
	};

	// input
	int w, h;
	INT2 rozmiar_pokoj, rozmiar_korytarz;
	int korytarz_szansa, polacz_korytarz, polacz_pokoj, kraty_szansa;
	KSZTALT_MAPY ksztalt;
	GDZIE_SCHODY schody_gora, schody_dol;
	bool stop;

	// input/output
	Pole* mapa;
	vector<Room>* rooms;
	Room* schody_gora_pokoj, *schody_dol_pokoj;
	int schody_gora_kierunek, schody_dol_kierunek;
	INT2 schody_gora_pozycja, schody_dol_pozycja;

	// output
	int blad;
	bool schody_dol_w_scianie, devmode;

	// tylko nowe zmienne s� zerowane dla kompatybilno�ci ze starym kodem
	OpcjeMapy() : stop(false)
	{

	}
};

//-----------------------------------------------------------------------------
bool generuj_mape2(OpcjeMapy& opcje, bool recreate=false);
bool kontynuuj_generowanie_mapy(OpcjeMapy& opcje);
bool generuj_schody(OpcjeMapy& opcje);
void rysuj_mape_konsola(Pole* mapa, uint w, uint h);
// zwraca pole kt�re ��czy dwa pomieszczenia, zak�ada �e po��czenie istnieje!, operuje na danych u�ytych w generuj_mape2
INT2 pole_laczace(int pokoj1, int pokoj2);
void ustaw_flagi(Pole* mapa, uint wh);

//-----------------------------------------------------------------------------
void generate_labirynth(Pole*& mapa, const INT2& size, const INT2& room_size, INT2& stairs, int& stairs_dir, INT2& room_pos, int kratki_szansa, bool devmode);

//-----------------------------------------------------------------------------
void generate_cave(Pole*& mapa, int size, INT2& stairs, int& stairs_dir, vector<INT2>& holes, IBOX2D* ext, bool devmode);
void regenerate_cave_flags(Pole* mapa, int size);
void free_cave_data();
