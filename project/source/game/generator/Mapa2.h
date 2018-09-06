#pragma once

//-----------------------------------------------------------------------------
#include "Light.h"
#include "Tile.h"
#include "Room.h"

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
	Int2 rozmiar_pokoj, rozmiar_korytarz;
	int korytarz_szansa, polacz_korytarz, polacz_pokoj, kraty_szansa;
	KSZTALT_MAPY ksztalt;
	GDZIE_SCHODY schody_gora, schody_dol;
	bool stop;

	// input/output
	Pole* mapa;
	vector<Room>* rooms;
	Room* schody_gora_pokoj, *schody_dol_pokoj;
	int schody_gora_kierunek, schody_dol_kierunek;
	Int2 schody_gora_pozycja, schody_dol_pozycja;

	// output
	int blad;
	bool schody_dol_w_scianie, devmode;

	// tylko nowe zmienne s¹ zerowane dla kompatybilnoœci ze starym kodem
	OpcjeMapy() : stop(false)
	{
	}
};

//-----------------------------------------------------------------------------
bool generuj_mape2(OpcjeMapy& opcje, bool recreate = false);
bool kontynuuj_generowanie_mapy(OpcjeMapy& opcje);
bool generuj_schody(OpcjeMapy& opcje);
void rysuj_mape_konsola(Pole* mapa, uint w, uint h);
// zwraca pole które ³¹czy dwa pomieszczenia, zak³ada ¿e po³¹czenie istnieje!, operuje na danych u¿ytych w generuj_mape2
Int2 pole_laczace(int pokoj1, int pokoj2);
void ustaw_flagi(Pole* mapa, uint wh);
