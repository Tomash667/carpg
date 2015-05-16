#pragma once

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

Do zrobienia w przysz³oœci:
+ okreœlony uk³ad mapy, pocz¹tkowe niektóre pola s¹ zablokowane np. ko³o, krzy¿
+ tajne drzwi tam gdzie nie ma drzwi na koñcu korytarza
+ tajna œciana do zniszczenia na koñcu korytarza
+ usuwanie niektórych murów ³¹cz¹cych pomieszczenia
+ okr¹g³e pomieszczenia
+ drzwi ukryte za np gobelinem
+ szansa na kontynuowanie korytarza
+ usuwanie œlepych uliczek

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
// Struktura opisuj¹ca pole na mapie
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
		F_BLOKADA		 = 0x20, // nieu¿ywane ?

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

		F_SPECJALNE = 1<<29, // póki co u¿ywane do oznaczenia drzwi do wiêzienia
		F_DRUGA_TEKSTURA = 1<<30,
		F_ODKRYTE = 1<<31
	};

	int flagi;
	word pokoj;
	POLE co;
	// jeszcze jest 1-2 bajty miejsca na coœ :o (jak pokój bêdzie byte)

	// DDDDGGGGRRRRSSSS000KKNSP
	// ####    ####    ########
	//     ####    ####
	// œciany >> 8
	// podsufit >> 12
	// góra >> 16
	// dó³ >> 20

	inline bool IsWall() const
	{
		return co == SCIANA || co == BLOKADA_SCIANA;
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
	return czy_blokuje2(p.co);
}
inline bool czy_blokuje21(POLE p)
{
	// czy to pole blokuje ruch albo nie powinno na nim byæ obiektów bo zablokujê droge
	return !(p == PUSTE || p == KRATKA || p == KRATKA_PODLOGA || p == KRATKA_SUFIT);
}
inline bool czy_blokuje21(const Pole& p)
{
	return czy_blokuje21(p.co);
}

//-----------------------------------------------------------------------------
// Œwiat³o w podziemiach
struct Light
{
	VEC3 pos, color;
	//int pokoj;
	float range;
	// tymczasowe
	VEC3 t_pos, t_color;

	void Save(File& f);
	void Load(File& f);
};

//-----------------------------------------------------------------------------
// Cel pomieszczenia
#define POKOJ_CEL_BRAK 0
#define POKOJ_CEL_SCHODY_GORA 1
#define POKOJ_CEL_SCHODY_DOL 2
#define POKOJ_CEL_SKARBIEC 3
#define POKOJ_CEL_PORTAL 4
#define POKOJ_CEL_WIEZIENIE 5
#define POKOJ_CEL_TRON 6
#define POKOJ_CEL_PORTAL_STWORZ 7

//-----------------------------------------------------------------------------
// Struktura opisuj¹ca pomieszczenie w podziemiach
struct Pokoj
{
	INT2 pos, size;
	vector<int> polaczone;
	//vector<Light> lights;
	int cel; // mo¿na by po³¹czyæ cel i korytarz (korytarz by by³ jednym z celi)
	bool korytarz;

	inline VEC3 Srodek() const
	{
		return VEC3(float(pos.x*2+size.x),0,float(pos.y*2+size.y));
	}
	inline INT2 Srodek2() const
	{
		return pos + size/2;
	}
	inline bool IsInside(float _x, float _z) const
	{
		return (_x >= 2.f*pos.x && _z >= 2.f*pos.y && _x <= 2.f*(pos.x+size.x) && _z <= 2.f*(pos.y+size.y));
	}
	inline bool IsInside(const VEC3& _pos) const
	{
		return IsInside(_pos.x, _pos.z);
	}
	inline float Distance(const VEC3& _pos) const
	{
		if(IsInside(_pos))
			return 0.f;

		return distance2d(_pos, Srodek());
	}
	inline float Distance(const Pokoj& _room) const
	{
		return distance2d(Srodek(), _room.Srodek());
	}
	inline VEC3 GetRandomPos() const
	{
		return VEC3(random(2.f*(pos.x+1), 2.f*(pos.x+size.x-1)), 0, random(2.f*(pos.y+1), 2.f*(pos.y+size.y-1)));
	}
	inline VEC3 GetRandomPos(float _margin) const
	{
		return VEC3(
			random(2.f*(pos.x+1) + _margin, 2.f*(pos.x+size.x-1) - _margin),
			0,
			random(2.f*(pos.y+1) + _margin, 2.f*(pos.y+size.y-1) - _margin));
	}
	inline VEC3 GetCenter() const
	{
		return VEC3(2.f*pos.x+float(size.x), 0, 2.f*pos.y+float(size.y));
	}

	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
// Opcje mapy
struct OpcjeMapy
{
	//-----------------------------------------------------------------------------
	// Kszta³t mapy podziemi
	enum KSZTALT_MAPY
	{
		PROSTOKAT,
		OKRAG
	};

	//-----------------------------------------------------------------------------
	// Gdzie stworzyæ schody?
	enum GDZIE_SCHODY
	{
		BRAK,
		LOSOWO,
		NAJDALEJ, // schody s¹ jak najdalej od zerowego pokoju
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
	vector<Pokoj>* pokoje;
	Pokoj* schody_gora_pokoj, *schody_dol_pokoj;
	int schody_gora_kierunek, schody_dol_kierunek;
	INT2 schody_gora_pozycja, schody_dol_pozycja;

	// output
	int blad;
	bool schody_dol_w_scianie;

	// tylko nowe zmienne s¹ zerowane dla kompatybilnoœci ze starym kodem
	OpcjeMapy() : stop(false)
	{

	}
};

//-----------------------------------------------------------------------------
bool generuj_mape2(OpcjeMapy& opcje, bool recreate=false);
bool kontynuuj_generowanie_mapy(OpcjeMapy& opcje);
bool generuj_schody(OpcjeMapy& opcje);
void rysuj_mape_konsola(Pole* mapa, uint w, uint h);
// zwraca pole które ³¹czy dwa pomieszczenia, zak³ada ¿e po³¹czenie istnieje!, operuje na danych u¿ytych w generuj_mape2
INT2 pole_laczace(int pokoj1, int pokoj2);
void ustaw_flagi(Pole* mapa, uint wh);

//-----------------------------------------------------------------------------
void generate_labirynth(Pole*& mapa, const INT2& size, const INT2& room_size, INT2& stairs, int& stairs_dir, INT2& room_pos, int kratki_szansa);

//-----------------------------------------------------------------------------
void generate_cave(Pole*& mapa, int size, INT2& stairs, int& stairs_dir, vector<INT2>& holes, IBOX2D* ext);
void regenerate_cave_flags(Pole* mapa, int size);
void free_cave_data();
