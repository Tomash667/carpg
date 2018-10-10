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
	Int2 rozmiar_pokoj, rozmiar_korytarz;
	int korytarz_szansa, polacz_korytarz, polacz_pokoj, kraty_szansa;
	KSZTALT_MAPY ksztalt;
	GDZIE_SCHODY schody_gora, schody_dol;
	bool stop;

	// input/output
	Tile* mapa;
	vector<Room>* rooms;
	Room* schody_gora_pokoj, *schody_dol_pokoj;
	GameDirection schody_gora_kierunek, schody_dol_kierunek;
	Int2 schody_gora_pozycja, schody_dol_pozycja;

	// output
	int blad;
	bool schody_dol_w_scianie, devmode;

	// tylko nowe zmienne s� zerowane dla kompatybilno�ci ze starym kodem
	OpcjeMapy() : stop(false)
	{
	}
};

//-----------------------------------------------------------------------------
bool generuj_mape2(OpcjeMapy& opcje, bool recreate = false);
bool kontynuuj_generowanie_mapy(OpcjeMapy& opcje);
bool generuj_schody(OpcjeMapy& opcje);
// zwraca pole kt�re ��czy dwa pomieszczenia, zak�ada �e po��czenie istnieje!, operuje na danych u�ytych w generuj_mape2
Int2 pole_laczace(int pokoj1, int pokoj2);
