/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Error - Klasy wyj¹tków do obs³ugi b³êdów
 * Dokumentacja: Patrz plik doc/Error.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

// Forward declarations
typedef long HRESULT;
#include <windows.h>

namespace common
{

// Klasa bazowa b³êdów
class Error
{
private:
	std::vector<string> m_Msgs;

protected:
	// Use this constructor if you need to compute something before
	// creating first error message and can't call constructor of this
	// base class on the constructor initialization list
	Error() { }

public:
	// Push a message on the stack
	// Use __FILE__ for file and __LINE__ for line
	void Push(const string &msg, const string &file = "", int line = 0);
	// Zwraca pe³ny opis b³êdu w jednym ³añcuchu
	// Indent - ³añcuch od którego ma siê zaczynaæ ka¿dy komunikat (wciêcie).
	// EOL - ³añcuch oddzielaj¹cy komunikaty (koniec wiersza). Nie do³¹czany na koñcu.
	void GetMessage_(string *Msg, const string &Indent = "", const string &EOL = "\r\n") const;

	// General error creation
	Error(const string &msg, const string &file = "", int line = 0)
	{
		Push(msg, file, line);
	}
};

// Tworzy b³¹d Windows API na podstawie GetLastError
class Win32Error : public Error
{
public:
	Win32Error(const string &msg = "", const string &file = "", int line = 0);
};

} // namespace common

// Use to push a message on the call stack
#define ERR_ADD_FUNC(exception) { (exception).Push(__FUNCSIG__, __FILE__, __LINE__); }
// Guard a function
#define ERR_TRY        { try {
#define ERR_CATCH(msg) } catch(Error &e) { e.Push((msg), __FILE__, __LINE__); throw; } }
#define ERR_CATCH_FUNC } catch(Error &e) { ERR_ADD_FUNC(e); throw; } }
