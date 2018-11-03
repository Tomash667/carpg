/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Base - Modu� podstawowy
 * Dokumentacja: Patrz plik doc/Base.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

// G��wne includy
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

// Niechciane includy
#include <limits> // :(
#include <cmath> // :(

// To jest na wypadek w��czania gdzie� <windows.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// String chc� mie� jak typ wbudowany
using std::string;

// Wy��cz g�upie warningi Visual C++
// ' var ' : conversion from 'size_t' to ' type ', possible loss of data
#pragma warning(disable: 4267)
// 'function': was declared deprecated
#pragma warning(disable: 4996)

// Postawowe typy danych
typedef unsigned __int32 uint;
typedef __int8 int1;
typedef unsigned __int8 uint1;
typedef __int16 int2;
typedef unsigned __int16 uint2;
typedef __int32 int4;
typedef unsigned __int32 uint4;
typedef __int64 int8;
typedef unsigned __int64 uint8;

// Wektor string�w
typedef std::vector<string> STRING_VECTOR;

// Je�li wska�nik niezerowy, zwalnia go i zeruje
#define SAFE_DELETE(x) { delete (x); (x) = 0; }
#define SAFE_DELARR(x) { delete [] (x); (x) = 0; }
#define SAFE_RELEASE(x) { if (x) { (x)->Release(); (x) = 0; } }

// Uniwersalny, brakuj�cy w C++ operator dos�ownego rzutowania (reintepretacji)
template <typename destT, typename srcT>
destT &absolute_cast(srcT &v)
{
	return reinterpret_cast<destT&>(v);
}
template <typename destT, typename srcT>
const destT &absolute_cast(const srcT &v)
{
	return reinterpret_cast<const destT&>(v);
}


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Og�lne

// Zwraca true, je�li podana liczba jest pot�g� dw�jki.
// T musi by� liczb� ca�kowit� bez znaku - uint1, uint2, uint4, uint8, albo
// liczb� ze znakiem ale na pewno dodatni�.
template <typename T>
inline bool IsPow2(T x)
{
	return (x & (x-1)) == 0;
}

// Zwraca logarytm dw�jkowy z podanej liczby ca�kowitej bez znaku, tzn numer
// najstarszego niezerowego bitu. Innymi s�owy, o ile bit�w przesun�� w lewo
// jedynk� �eby otrzyma� najstarsz� jedynk� podanej liczby.
// - Warto�� 0 zwraca dla liczb 0 i 1.
uint4 log2u(uint4 x);
// Zwraca mask� bitow� z ustawionymi na jedynk� n najm�odszymi bitami.
// n musi by� z z zakresu 0..32.
uint4 GetBitMask(uint4 n);

// Alokuje now� tablic� dynamiczn� 2D
template <typename T>
T **new_2d(size_t cx, size_t cy)
{
	T **a = new T*[cx];
	for (size_t x = 0; x < cx; x++)
		a[x] = new T[cy];
	return a;
}

// Zwalnia tablic� dynamiczn� 2D
template <typename T>
void delete_2d(T **a, size_t cx)
{
	for (size_t x = 0; x < cx; x++)
		delete[] a[x];
	delete[] a;
}

// Kopiuje string do char* ��cznie ze znakami '\0' (czego nie zapewnia strcpy)
// - Na ko�cu do��cza '\0'.
// - Oczywi�cie dest musi by� dostatecznie pojemne.
inline void strcpy0(char* dest, const string &src)
{
	for (size_t i = 0; i < src.length(); i++)
		dest[i] = src[i];
	dest[src.length()] = '\0';
}

// Wzorzec projektowy Singleton - klasa bazowa
template <typename T>
class Singleton
{
public:
	static T & GetInstance()
	{
		static T Instance;
		return Instance;
	}
};

// Zeruje pami��
void ZeroMem(void *Data, size_t NumBytes);

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Inteligentne wska�niki

// Polityka zwalaniania, kt�ra robi: delete p;
class DeletePolicy
{
public:
	template <typename T>
	static void Destroy(T *p)
	{
		// Sztuczka inspirowana Boost (checked_delete.hpp)
		typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
		(void) sizeof(type_must_be_complete);
		delete p;
	}
};
// Polityka zwalaniania, kt�ra robi: delete [] p;
class DeleteArrayPolicy
{
public:
	template <typename T>
	static void Destroy(T *p)
	{
		// Sztuczka inspirowana Boost (checked_delete.hpp)
		typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
		(void) sizeof(type_must_be_complete);
		delete [] p;
	}
};

// Wska�nik z wy��cznym prawem w�asno�ci.
// - Niekopiowalny.
// - W destruktorze zwalnia.
template <typename T, typename PolicyT = DeletePolicy>
class scoped_ptr
{
    template<typename Y, typename PolicyY> friend class scoped_ptr;

private:
	T *m_Ptr;

	scoped_ptr(const scoped_ptr &);
	scoped_ptr & operator = (const scoped_ptr &);

public:
	typedef T value_type;
	typedef T *ptr_type;

	explicit scoped_ptr(T *p = NULL) : m_Ptr(p) { }
	~scoped_ptr() { PolicyT::template Destroy<T>(m_Ptr); }
	
	T & operator * () const { assert(m_Ptr != NULL); return *m_Ptr; }
	T * operator -> () const { assert(m_Ptr != NULL); return m_Ptr; }
	T & operator [] (uint i) const { return m_Ptr[i]; }

	inline friend bool operator == (const scoped_ptr &lhs, const T *rhs) { return lhs.m_Ptr == rhs; }
	inline friend bool operator == (const T *lhs, const scoped_ptr &rhs) { return lhs == rhs.m_Ptr; }
	inline friend bool operator != (const scoped_ptr &lhs, const T *rhs) { return lhs.m_Ptr != rhs; }
	inline friend bool operator != (const T *lhs, const scoped_ptr &rhs) { return lhs != rhs.m_Ptr; }

	T * get() const { return m_Ptr; }
	void swap(scoped_ptr<T, PolicyT> &b) { T *tmp = b.m_Ptr; b.m_Ptr = m_Ptr; m_Ptr = tmp; }
	void reset(T *p = NULL) { assert(p == NULL || p != m_Ptr); scoped_ptr<T, PolicyT>(p).swap(*this); }
};

// Wska�nik ze zliczaniem referencji
// - Kopiowalny.
// - Zwalnia, kiedy zwolniony zostanie ostatni wskazuj�cy na obiekt shared_ptr.
template <typename T, typename PolicyT = DeletePolicy>
class shared_ptr
{
    template<typename Y, typename PolicyY> friend class shared_ptr;

private:
	T *m_Ptr;
	unsigned *m_Counter;

public:
	typedef T value_type;
	typedef T *ptr_type;

	explicit shared_ptr(T *p = NULL) : m_Ptr(p), m_Counter(new unsigned(1)) { }
	~shared_ptr() { if (--(*m_Counter) == 0) { delete m_Counter; PolicyT::template Destroy<T>(m_Ptr); } }
	
	shared_ptr(const shared_ptr &p) : m_Ptr(p.m_Ptr), m_Counter(p.m_Counter) { (*m_Counter)++; }
	shared_ptr & operator = (const shared_ptr &p) { reset<T>(p); return *this; }
	template <typename U, typename PolicyU> explicit shared_ptr(const shared_ptr<U, PolicyU> &p) : m_Ptr(p.m_Ptr), m_Counter(p.m_Counter) { (*m_Counter)++; }
	template <typename U, typename PolicyU> shared_ptr & operator = (const shared_ptr<U, PolicyU> &p) { reset<U, PolicyU>(p); return *this; }

	T & operator * () const { assert(m_Ptr != NULL); return *m_Ptr; }
	T * operator -> () const { assert(m_Ptr != NULL); return m_Ptr; }
	T & operator [] (uint i) const { return m_Ptr[i]; }

	inline friend bool operator == (const shared_ptr &lhs, const T *rhs) { return lhs.m_Ptr == rhs; }
	inline friend bool operator == (const T *lhs, const shared_ptr &rhs) { return lhs == rhs.m_Ptr; }
	inline friend bool operator != (const shared_ptr &lhs, const T *rhs) { return lhs.m_Ptr != rhs; }
	inline friend bool operator != (const T *lhs, const shared_ptr &rhs) { return lhs != rhs.m_Ptr; }
	template <typename U, typename PolicyU> bool operator == (const shared_ptr<U, PolicyU> &rhs) const { return m_Ptr == rhs.m_Ptr; }
	template <typename U, typename PolicyU> bool operator != (const shared_ptr<U, PolicyU> &rhs) const { return m_Ptr != rhs.m_Ptr; }

	T * get() const { return m_Ptr; }
	void swap(shared_ptr<T, PolicyT> &b) { T *tmp = b.m_Ptr; b.m_Ptr = m_Ptr; m_Ptr = tmp; unsigned *tmpc = b.m_Counter; b.m_Counter = m_Counter; m_Counter = tmpc; }
	void reset(T *p = NULL) { if (p == m_Ptr) return; shared_ptr<T, PolicyT>(p).swap(*this); }
	template <typename U, typename PolicyU> void reset(const shared_ptr<U, PolicyU> &p) { shared_ptr<U, PolicyU>(p).swap(*this); }
	bool unique() const { return *m_Counter == 1; }
};

template <typename T, typename PolicyT> void swap(scoped_ptr<T, PolicyT> &a, scoped_ptr<T, PolicyT> &b) { a.swap(b); }
template <typename T, typename PolicyT> void swap(shared_ptr<T, PolicyT> &a, shared_ptr<T, PolicyT> &b) { a.swap(b); }

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// �a�cuchy

// Rodzaje znak�w ko�ca wiersza
enum EOLMODE
{
	EOL_NONE, // Podczas konwersji pozostawienie bez zmian
	EOL_CRLF, // 13 i 10 (Windows)
	EOL_LF,   // 10      (Unix)
	EOL_CR	  // 13      (Mac)
};

// Standardowy �a�cuch ko�ca wiersza zale�ny od systemu
extern const char * const EOL;

// Strona kodowa polskich znakow
enum CHARSET
{
	CHARSET_NONE = 0, // Brak polskich liter
	CHARSET_WINDOWS,  // Strona kodowa Windows-1250 (u�ywana w GUI Windows)
	CHARSET_ISO,      // Strona kodowa ISO-8859-2 (Latin-2) (u�ywana w Linuksie)
	CHARSET_IBM,      // Strona kodowa IBM (CP852) (u�ywana na konsoli Windows)
	CHARSET_UTF8,     // Strona kodowa UTF-8
};

// Zwraca true, jesli znak jest alfanumeryczny (litera lub cyfra) wg ustawie� systemu
bool CharIsAlphaNumeric(char ch);
// Zwraca true, jesli znak jest alfanumeryczny wg sta�ych regu� - cyfra, litera
// lub polska litera w kodowaniu Windows-1250
bool CharIsAlphaNumeric_f(char ch);

// Zwraca true, jesli znak jest litera
bool CharIsAlpha(char ch);
// Zwraca true, jesli znak jest cyfr� dziesietn�
inline bool CharIsDigit(char ch) { return (ch >= '0' && ch <= '9'); }
// Zwraca true, je�li znak jest cyfr� szesnastkow�
// - Akceptuje ma�e i du�e litery.
inline bool CharIsHexDigit(char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'); }
// Zwraca true, jesli znak jest mala litera
// - Nie obs�uguje stron kodowych wielobajtowych (UTF-8).
bool CharIsLower(char ch);
bool CharIsLower(char ch, CHARSET Charset);
// Zwraca true, jesli znak jest duza litera
// - Nie obs�uguje stron kodowych wielobajtowych (UTF-8).
bool CharIsUpper(char ch);
bool CharIsUpper(char ch, CHARSET Charset);
// Zwraca true, jesli podany znak jest odstepem, czyli bialym znakiem
// Czyli jednym ze znakow:
// - 0x09 (9)  "\t" - tabulacja
// - 0x0A (10) "\n" - znak konca wiersza
// - 0x0D (13) "\r" - znak konca wiersza
// -           "\v" - tabulacja pionowa
// - 0x20 (32) " "  - spacja
inline bool CharIsWhitespace(char ch)
{
	return (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t' || ch == '\v');
}

// Odwraca �a�cuch w miejscu
void ReverseString(string *s);
// Obcina bia�e znaki z pocz�tku i ko�ca �a�cucha w miejscu
void Trim(string *s);
// Obcina bia�e znaki z pocz�tku i ko�ca �a�cucha
void Trim(string *Out, const string &s);

// UWAGA! Z nieznanych przyczyn UpperCase i LowerCase �le konwertuje w stronie
// kodowej IBM. Lepiej najpierw zmieni� wielko�� liter w innej stronie a potem
// przekonwertowa� na IBM - wtedy dzia�a. Kiedy� mo�e naprawi� ten b�ad :/

// Je�li to litera, konwertuje na ma��
// - Nie obs�uguje wielobajtowych stron kodowych (UTF-8).
char CharToLower(char ch);
char CharToLower(char ch, CHARSET Charset);
// Je�li to litera, konwertuje na du��
// - Nie obs�uguje wielobajtowych stron kodowych (UTF-8).
char CharToUpper(char ch);
char CharToUpper(char ch, CHARSET Charset);
// Konwertuje �a�cuch na ma�e litery
// - Nie obs�uguje wielobajtowych stron kodowych (UTF-8).
void LowerCase(string *s);
void LowerCase(string *s, CHARSET Charset);
inline void LowerCase(string *Out, const string &s) { *Out = s; LowerCase(Out); }
inline void LowerCase(string *Out, const string &s, CHARSET Charset) { *Out = s; LowerCase(Out, Charset); }
// Konwertuje �a�cuch na du�e
// - Nie obs�uguje wielobajtowych stron kodowych (UTF-8).
void UpperCase(string *s);
void UpperCase(string *s, CHARSET Charset);
inline void UpperCase(string *Out, const string &s) { *Out = s; UpperCase(Out); }
inline void UpperCase(string *Out, const string &s, CHARSET Charset) { *Out = s; UpperCase(Out, Charset); }

// Zmienia znak specjalny kodowania Windows-1250 na odpowiednik normalny
// Jesli podany znak jest jednym ze znakow specjalnych Windows-1250,
// zwraca przez a_s jego normalny odpowiednik (mo�na poda� wska�nik 0) i zwraca true.
// Je�li to nie jest taki znak, zwraca false.
bool Charset_WindowsSpecialChar(char a_c, string *a_s);
// Zmienia znak z jednej strony kodowej na druga lub pozostawia bez zmian
// Nie obsluguje stron kodowych wielobajtowych, tzn. UTF-8
// (wynik jest wtedy niezdefiniowany).
char Charset_Convert_Char(char a_c, CHARSET a_Charset1, CHARSET a_Charset2);
// Konwertuje lancuch na podana strone kodowa
// Out nie moze byc tym samym lancuchem, co s.
void Charset_Convert(string *out, const string &s, CHARSET Charset1, CHARSET Charset2);

// Szyforowanie/deszyfrowanie algorytmem ROT13
void Rot13(string *InOut);
inline void Rot13(string *Out, const string &In) { *Out = In; Rot13(Out); }

// Rodzaj znaku ko�ca wiersza na odpowiadaj�cy mu znak (lub znaki)
void EolModeToStr(string *Out, EOLMODE EolMode);

// Zamienia w �a�cuchu wszystkie wyst�pienia jedego pod�a�cucha na drugi
// Result musi by� oczywi�cie, jak w ka�dej takiej funkcji, innym �a�cuchem ni� s.
void Replace(string *result, const string &s, const string &s1, const string &s2);
// Zmienia w �a�cuchu wszystkie wyst�pienia znaku Ch1 na Ch2
void Replace(string *Out, const string &s, char Ch1, char Ch2);
// Zmienia w �a�cuchu wszystkie wyst�pienia znaku Ch1 na Ch2 w miejscu
void Replace(string *InOut, char Ch1, char Ch2);

// Zmienia wszelkiego rodzaju znaki ko�ca wiersza w poddanym tek�cie na takie same - wg podanego trybu
void ReplaceEOL(string *result, const string &s, EOLMODE EOLMode);

// Zamienia wszystkie odst�py na pojedyncze spacje
void NormalizeWhitespace(string *result, const string &s);

// Zwraca �a�cuch powt�rzony podan� liczb� razy
void DupeString(string *Out, const string &s, size_t count);

// Zwraca pod�a�cuch z prawej strony
void RightStr(string *Out, const string &s, size_t Length);

// Zwraca liczb� wyst�pie� pod�a�cucha
size_t SubstringCount(const string &str, const string &substr);

// Zwraca true, je�li podane �a�cuchy s� r�wne bez rozr�niania wielko�ci liter
bool StrEqualI(const string &s1, const string &s2);
// Zwraca true, je�li podfragmenty podanych �a�cuch�w s� identyczne
bool SubStrEqual(const string &s1, size_t off1, const string &s2, size_t off2, size_t length);
// Zwraca true, je�li podfragmenty podanych �a�cuch�w s� identyczne nie rozr�niaj�c wielko�ci liter
bool SubStrEqualI(const string &s1, size_t off1, const string &s2, size_t off2, size_t length);

// Czy �a�cuch zawiera znaki ko�ca wiersza?
bool ContainsEOL(const string &s);

// Zwraca true, jesli podany lancuch zaczyna sie od podanego podlancucha
bool StrBegins(const string &s, const string &sub, size_t Begin = 0);
// Zwraca true, je�li podany �a�cuch ko�czy si� podanym pod�a�cuchem
bool StrEnds(const string &s, const string &Sub);

// Kolejne wywo�ania rozdzielaj� �a�cuch na kolejne elementy oddzielone w jego tre�ci okre�lonym pod�a�cuchem
// out - [out]
// index - [in,out]
bool Split(const string &s, const string &delimiter, string *out, size_t *index);
// Dzia�a jak wy�ej, ale �a�cuchem rozdzielaj�cym jest ka�dy dowolny znak z delimiters
bool SplitFirstOf(const string &s, const string &delimiters, string *out, size_t *index);
// Dzieli �a�cuch na cz�ci i zwraca je jako wektor
void Split(STRING_VECTOR *Out, const string &s, const string &Delimiter);
// Kolejne wywo�ania rozdzielaj� �a�cuch na kolejne elementy
// oddzielone w jego tre�ci kt�r�� z sekwencji uznawan� za koniec wiersza
// (czyli na kolejne wiersze)
bool SplitEOL(const string &s, string *out, size_t *index);
// Kolejne wywo�ania rozdzielaj� �a�cuch na elementy
// oddzielone odst�pami uwzgl�dniaj�c ci�gi obi�te w cudzys�owy "" jako ca�o��
bool SplitQuery(const string &s, string *out, size_t *index);
// Sprawdza, czy podany �ancuch jest zgodny z podan� mask� mog�c� zawiera� znaki wieloznaczne:
// '?' zast�puje dowolny jeden znak, '*' zast�puje dowoln� liczb� (tak�e 0) dowolnych znak�w.
bool ValidateWildcard(const string &Mask, const string &S, bool CaseSensitive = true, size_t MaskOff = 0, size_t SOff = 0);
// Zwraca zmiennoprzecinkow� trafno�� wyszukiwania stringa SubStr w stringu Str
// Zaawansowany algorytm, wymy�lony przeze mnie dawno temu. Bierze pod uwag�
// rzeczy takie jak: # ile razy pod�a�cuch wyst�puje # czy pasuje wielko�� liter
// # d�ugo�� �a�cucha i pod�a�cucha # czy to ca�e s�owo # czy to ca�y �a�cuch.
float FineSearch(const string &SubStr, const string &Str);
// Odleg�o�� edycyjna mi�dzy dwoma �a�cuchami.
// Z�o�ono��: O( s1.length * s2.length )
// Im mniejsza, tym �a�cuchy bardziej podobne. Je�li identyczne, zwraca 0.
size_t LevenshteinDistance(const string &s1, const string &s2);
// Odleg�o�� edycyjna mi�dzy dwoma �a�cuchami bez uwzgl�dniania wielko�ci znak�w
// Z�o�ono��: O( s1.length * s2.length )
// Im mniejsza, tym �a�cuchy bardziej podobne. Je�li identyczne, zwraca 0.
size_t LevenshteinDistanceI(const string &s1, const string &s2);

// Sortowanie naturalne, czyli takie w kt�rym np. "abc2" jest przed "abc120"
class StringNaturalCompare
{
private:
	bool m_CaseInsensitive;

	int CompareNumbers(const string &s1, size_t i1, const string &s2, size_t i2);

public:
	StringNaturalCompare(bool CaseInsensitive) : m_CaseInsensitive(CaseInsensitive) { }
	int Compare(const string &s1, const string &s2);
};

// Funktory do sortowania �a�cuch�w
// Przyk�ad u�ycia:
//   std::vector<string> v;
//   std::sort(v.begin(), v.end(), StringNaturalLess());

class StringNaturalLess : public StringNaturalCompare
{
public:
	StringNaturalLess(bool CaseInsensitive = true) : StringNaturalCompare(CaseInsensitive) { }
	bool operator () (const string &s1, const string &s2) { return (Compare(s1, s2) < 0); }
};

class StringNaturalGreater : public StringNaturalCompare
{
public:
	StringNaturalGreater(bool CaseInsensitive = true) : StringNaturalCompare(CaseInsensitive) { }
	bool operator () (const string &s1, const string &s2) { return (Compare(s1, s2) > 0); }
};

// Predykaty do por�wnywania �a�cuch�w bez rozr�niania wielko�ci liter.
class StrLessCi
{
public:
	bool operator () (const string &s1, const string &s2)
	{
		string u1 = s1, u2 = s2;
		UpperCase(&u1);
		UpperCase(&u2);
		return u1 < u2;
	}
};
class StrGreaterCi
{
public:
	bool operator () (const string &s1, const string &s2)
	{
		string u1 = s1, u2 = s2;
		UpperCase(&u1);
		UpperCase(&u2);
		return u1 > u2;
	}
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// �cie�ki do plik�w

const char DIR_SEP = '\\';

// Zwraca true, je�li �cie�ka jest bezwzgl�dna
bool PathIsAbsolute(const string &s);
// Dodaje do sciezki konczacy '/' lub '\', jesli go nie ma - na miejscu
void IncludeTrailingPathDelimiter(string *InOutPath);
// Dodaje do sciezki konczacy '/' lub '\', jesli go nie ma - do nowego stringa
void IncludeTrailingPathDelimiter(string *OutPath, const string &InPath);
// Pozbawia sciezke konczacego '/' lub '\', jesli go ma - na miejscu
void ExcludeTrailingPathDelimiter(string *InOutPath);
// Pozbawia sciezke konczacego '/' lub '\', jesli go ma - do nowego stringa
void ExcludeTrailingPathDelimiter(string *OutPath, const string &InPath);
// Zwraca pocz�tek �cie�ki, w postaci takiej jak "C:\", "\\komputer\udzia�\" czy "/" albo "\"
// Je�li �cie�ka jest wzgl�dna, zwraca �a�cuch pusty.
void ExtractPathPrefix(string *OutPrefix, const string &s);
// Zwraca �cie�k� do pliku bez nazwy pliku
void ExtractFilePath(string *OutPath, const string &s);
// Zwraca sam� nazw� pliku bez �cie�ki
void ExtractFileName(string *OutFileName, const string &s);
// Zwraca rozszerzenie pliku wraz z kropk�
void ExtractFileExt(string *OutExt, const string &s);
// Zmienia w nazwie pliku rozszerzenie na nowe
// Jesli Ext = "", usuwa rozszerzenie.
// Nowe rozszerzenie musi zawiera� rozpoczynaj�c� kropk� (tzn. nie musi, ale wypada�oby :)
void ChangeFileExt(string *Out, const string &FileName, const string &Ext);
// Pozbawia sciezke artefaktow typu ".\" czy "..\"
// Odpowiednio ja oczywiscie przetwarza tak, ze wyjscie jest logicznie rownowazne wejsciu.
void NormalizePath(string *OutPath, const string &s);
// Przetwarza sciezke wzgledna na bezwzgledna wzgledem okreslonej sciezki
// Na przyk�ad Base="C:\Kat", Path="PodKat\Plik.txt", Wynik="C:\Kat\PodKat\Plik.txt".
void RelativeToAbsolutePath(string *Out, const string &Base, const string &Path);
// Przetwarza sciezke bezwzgledna na wzgledna wzgledem okreslonej sciezki
// Je�li si� nie da, wychodzi r�wnie� bezwzgl�dna, r�wna Target.
// Na przyk�ad Base="C:\Kat", Tareget="C:\Kat\Podkat\Plik.txt", Wynik="PodKat\Plik.txt".
void AbsoluteToRelativePath(string *Out, const string &Base, const string &Target);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KONWERSJE

extern const char _DIGITS_L[];
extern const char _DIGITS_U[];

// Zwraca liczbowy odpowiednik cyfry szesnastkowej.
// Je�li b��d, zwraca 0xFF.
// Akceptuje zar�wno ma�e, jak i du�e litery.
inline uint1 HexDigitToNumber(char Ch)
{
	if (Ch >= '0' && Ch <= '9')
		return (uint1)(Ch - '0');
	else if (Ch >= 'A' && Ch <= 'F')
		return (uint1)(Ch - ('A' - 10));
	else if (Ch >= 'a' && Ch <= 'f')
		return (uint1)(Ch - ('a' - 10));
	else
		return 0xFF;
}

// Konwersja liczby ca�kowitej na �a�cuch
// Base musi by� z zakresu 2..36
template <typename T>
void UintToStr(string *Out, T x, uint4 Base = 10, bool UpperCase = true)
{
	if (x == 0)
	{
		Out->assign("0");
		return;
	}

	Out->clear();
	Out->reserve(8);

	// Podstawa to pot�ga dw�jki - b�dzie pro�ciej, bo przesuwanie bitowe zamiast dzielenia
	if (IsPow2<uint4>(Base))
	{
		// O ile bit�w przesuwa�?
		uint4 Bits = log2u(Base);
		// Maska bitowa do &-owania
		uint4 BitMask = GetBitMask(Bits);
		// Jazda!
		if (UpperCase)
		{
			while (x != 0)
			{
				*Out += _DIGITS_U[x & BitMask];
				x = x >> Bits;
			}
		}
		else
		{
			while (x != 0)
			{
				*Out += _DIGITS_L[x & BitMask];
				x = x >> Bits;
			}
		}
	}
	// Podstawa to nie pot�ga dw�jki
	else
	{
		if (UpperCase)
		{
			while (x != 0)
			{
				*Out += _DIGITS_U[x % Base];
				x /= Base;
			}
		}
		else
		{
			while (x != 0)
			{
				*Out += _DIGITS_L[x % Base];
				x /= Base;
			}
		}
	}

	ReverseString(Out);
}

template <typename T>
void IntToStr(string *Out, T x, uint4 Base = 10, bool UpperCase = true)
{
	if (x == 0)
	{
		Out->assign("0");
		return;
	}

	Out->clear();
	Out->reserve(8);

	// Liczba dodatnia - identycznie jak w UintToStr
	if (x > 0)
	{
		// Podstawa to pot�ga dw�jki - b�dzie pro�ciej, bo przesuwanie bitowe zamiast dzielenia
		if (IsPow2<uint4>(Base))
		{
			// O ile bit�w przesuwa�?
			uint4 Bits = log2u(Base);
			// Maska bitowa do &-owania
			uint4 BitMask = GetBitMask(Bits);
			// Jazda!
			if (UpperCase)
			{
				while (x != 0)
				{
					*Out += _DIGITS_U[x & BitMask];
					x = x >> Bits;
				}
			}
			else
			{
				while (x != 0)
				{
					*Out += _DIGITS_L[x & BitMask];
					x = x >> Bits;
				}
			}
		}
		// Podstawa to nie pot�ga dw�jki
		else
		{
			if (UpperCase)
			{
				while (x != 0)
				{
					*Out += _DIGITS_U[x % Base];
					x /= Base;
				}
			}
			else
			{
				while (x != 0)
				{
					*Out += _DIGITS_L[x % Base];
					x /= Base;
				}
			}
		}
	}
	// Liczba ujemna - tu b�dzie musia�o by� inaczej
	// Modulo jest UB, wi�c u�yjemy obej�cia.
	else
	{
		T x_div_base;
		if (UpperCase)
		{
			while (x != 0)
			{
				x_div_base = x/(T)Base;
				*Out += _DIGITS_U[- (x - x_div_base*(T)Base)];
				x = x_div_base;
			}
		}
		else
		{
			while (x != 0)
			{
				x_div_base = x/(T)Base;
				*Out += _DIGITS_L[- (x - x_div_base*(T)Base)];
				x = x_div_base;
			}
		}
		*Out += '-';
	}

	ReverseString(Out);
}

// Konwersja �a�cucha na liczb� ca�kowit�
// Zwraca:
//  0 - sukces
// -1 - b��d sk�adni �a�cucha
// -2 - przekroczenie zakresu
// Convert digit in any base ('0'..'9', 'A'..'Z', 'a'..'z') to number, return false on error
// [Wewn�trzna]
template <typename T>
bool _CharToDigit(T *Digit, char ch)
{
	if (ch >= '0' && ch <= '9')
		*Digit = static_cast<T>(ch-'0');
	else if (ch >= 'A' && ch <= 'Z')
		*Digit = static_cast<T>(ch-'A'+10);
	else if (ch >= 'a' && ch <= 'z')
		*Digit = static_cast<T>(ch-'a'+10);
	else
		return false;
	return true;
}

template <typename T>
int StrToUint(T *Number, const string &str, unsigned Base = 10)
{
	T BaseT = static_cast<T>(Base);
	if (str.empty()) return -1;
	*Number = 0;
	T Digit, LastNumber = 0, Limit = std::numeric_limits<T>::max()/BaseT;
	for (size_t i = 0; i < str.length(); ++i) {
		if (!_CharToDigit(&Digit, str[i])) return -1;
		if (Digit >= BaseT)  return -1;
		if (*Number > Limit) return -2;
		*Number = *Number * BaseT + Digit;
		if (*Number < LastNumber) return -2;
		LastNumber = *Number;
	}
	return 0;
}

template <typename T>
int StrToInt(T *Number, const string &str, unsigned Base = 10)
{
	T BaseT = static_cast<T>(Base);
	if (str.empty()) return -1;
	*Number = 0;
	size_t i = 0;
	int Sign = +1;
	if (str[0] == '+') i = 1;
	else if (str[0] == '-') { Sign = -1; i = 1; }
	T Digit, LastNumber = 0, Limit;
	if (Sign > 0) Limit = std::numeric_limits<T>::max()/BaseT;
	else          Limit = std::numeric_limits<T>::min()/BaseT;
	for (; i < str.length(); ++i) {
		if (!_CharToDigit(&Digit, str[i])) return -1;
		if (Digit >= BaseT) return -1;
		if (Sign > 0) { if (*Number > Limit) return -2; }
		else          { if (*Number < Limit) return -2; }
		*Number = *Number * BaseT + Digit * Sign;
		if (Sign > 0) { if (*Number < LastNumber) return -2; }
		else          { if (*Number > LastNumber) return -2; }
		LastNumber = *Number;
	}
	return 0;
}

template <typename T> inline string IntToStrR (T x, int base = 10, bool UpperCase = true) { string r; IntToStr (&r, x, base, UpperCase); return r; }
template <typename T> inline string UintToStrR(T x, int base = 10, bool UpperCase = true) { string r; UintToStr(&r, x, base, UpperCase); return r; }

inline void Size_tToStr(string *Out, size_t x, size_t Base = 10, bool UpperCase = true) { UintToStr(Out, (uint4)x, Base, UpperCase); }
inline string Size_tToStrR(size_t x, size_t Base = 10, bool UpperCase = true) { string R; Size_tToStr(&R, x, Base, UpperCase); return R; }

// Konwersja liczby na �a�cuch o minimalnej podanej d�ugo�ci.
// Zostanie do tej d�ugo�ci uzupe�niony zerami.
template <typename T>
void UintToStr2(string *Out, T x, unsigned Length, int base = 10)
{
	string Tmp;
	UintToStr(&Tmp, x, base);
	if (Tmp.length() >= Length)
		*Out = Tmp;
	else
	{
		Out->clear();
		Out->reserve(std::max(Tmp.length(), Length));
		for (size_t Left = Length - Tmp.length(); Left > 0; Left--)
			*Out += '0';
		*Out += Tmp;
	}
}
template <typename T>
void IntToStr2(string *Out, T x, unsigned Length, int base = 10)
{
	string Tmp;
	IntToStr<T>(&Tmp, x, base);
	if (Tmp.length() >= Length)
		*Out = Tmp;
	else
	{
		Out->clear();
		Out->reserve(std::max(Tmp.length(), Length));
		assert(!Tmp.empty());
		if (Tmp[0] == '-')
		{
			*Out += '-';
			for (size_t Left = Length - Tmp.length(); Left > 0; Left--)
				*Out += '0';
			Out->append(Tmp.begin()+1, Tmp.end());
		}
		else
		{
			for (size_t Left = Length - Tmp.length(); Left > 0; Left--)
				*Out += '0';
			*Out += Tmp;
		}
	}
}

template <typename T> string UintToStr2R(T x, unsigned Length, int Base = 10) { string R; UintToStr2<T>(&R, x, Length, Base); return R; }
template <typename T> string IntToStr2R (T x, unsigned Length, int Base = 10) { string R; IntToStr2<T> (&R, x, Length, Base); return R; }

// Konwertuje znak na �a�cuch, jako �e automatycznie to si� niestety nie odbywa
inline void CharToStr(string *Out, char ch) { Out->clear(); *Out += ch; }
inline string CharToStrR(char ch) { string s; s += ch; return s; }

// Konwersja liczb zmiennoprzecinkowych na �a�cuch
// mode:
// - 'e' : -0.12345e-001
// - 'E' : -0.12345E-001
// - 'f' : -0.12345
// - 'g' : optimum ('e', 'f')
// - 'G' : optimum ('E', 'f')
// precision = 0..20
void DoubleToStr(string *Out, double x, char Mode = 'g', int Precision = 6);
void FloatToStr(string *Out, float x, char Mode = 'g', int Precision = 6);

inline string DoubleToStrR(double x, char Mode = 'g', int Precision = 6) { string R; DoubleToStr(&R, x, Mode, Precision); return R; }
inline string FloatToStrR (float x, char Mode = 'g', int Precision = 6) { string R;  FloatToStr (&R, x, Mode, Precision); return R; }

// Konwersja �a�cucha na liczb� zmiennoprzecinkow�.
// - W przypadku b��du sk�adni zwracaj� warto�� != 0.
// - B��dy zakresu nie s� sprawdzane - wynik niezdefiniowany.
int StrToDouble(double *out, const string &s);
int StrToFloat (float *out,  const string &s);

// Konwertuje warto�� logiczn� na �a�cuch
// mode:
// - '0' : 0 / 1
// - 'f' : false / true
// - 'F' : False / True
// - 'U' : FALSE / TRUE
// - 'g' : f / t
// - 'G' : F / T
void BoolToStr(string *Out, bool x, char mode = 'f');
inline string BoolToStrR(bool x, char mode = 'f') { string R; BoolToStr(&R, x, mode); return R; }
// Konwertuje �a�cuch na warto�� logiczn�
// - Dozwolone wszelkie warto�ci jak w BoolToStr.
// - B��d: zwraca false.
bool StrToBool(bool *result, const string &s);

// Konwertuje wska�nik na �a�cuch 8 znak�w zapisany szesnastkowo
void PtrToStr(string *Out, const void* p);
inline string PtrToStrR(const void *p) { string R; PtrToStr(&R, p); return R; }

// Liczba bajt�w na �a�cuch z rozmiarem np. "1 B", "10.5 kB"
// - Space - czy mi�dzy liczb� a jednostk� dawa� spacj�
// - Precision - ilo�� cyfr po przecinku (mo�e by� te� 0)
// - Jako T uzywac uint1, uint2, uint4, uint8 lub int ale na pewno warto�ci dodaniej.
template <typename T>
void SizeToStr(string *str, T size, bool Space, int Precision)
{
	double size2 = (double)size;
	if (size2 >= 1024.0*1024.0*1024.0*1024.0)
	{
		DoubleToStr( str, size2/(1024.0*1024.0*1024.0*1024.0), 'f', Precision );
		str->append(Space ? " TB" : "TB");
	}
	else if (size2 >= 1024.0*1024.0*1024.0)
	{
		DoubleToStr( str, size2/(1024.0*1024.0*1024.0), 'f', Precision );
		str->append(Space ? " GB" : "GB");
	}
	else if (size2 >= 1024.0*1024.0)
	{
		DoubleToStr( str, size2/(1024.0*1024.0), 'f', Precision );
		str->append(Space ? " MB" : "MB");
	}
	else if (size2 >= 1024.0)
	{
		DoubleToStr( str, size2/1024.0, 'f', Precision );
		str->append(Space ? " KB" : "KB");
	}
	else
	{
		UintToStr( str, size, 'f', Precision );
		str->append(Space ? " B" : "B");
	}
}
template <typename T> string SizeToStrR(T size, bool Space, int Precision) { string r; SizeToStr(size, &r, Space, Precision); return r; }


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// CZAS

class TimeMeasurer
{
private:
	bool m_QPF_Using;
	int8 m_QPF_TicksPerSec;
	int8 m_QPF_Start;
	uint4 m_Start;

public:
	TimeMeasurer();

	// zwraca czas od uruchomienia aplikacji, w sekundach
	double GetTimeD();
	float  GetTimeF();
};

extern TimeMeasurer g_Timer;

// Zatrzymuje bie��cy w�tek na podan� liczb� milisekund
// - Dzia�a z jak��tam dok�adno�ci� - mo�e nie super, ale na pewno wi�ksz� ni�
//   ca�a sekunda.
void Wait(uint4 Miliseconds);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Matematyka

// Maksymalne i minimalne warto�ci liczb r�nych typ�w
const int1 MININT1 = std::numeric_limits<int1>::min();
const int1 MAXINT1 = std::numeric_limits<int1>::max();
const int2 MININT2 = std::numeric_limits<int2>::min();
const int2 MAXINT2 = std::numeric_limits<int2>::max();
const int4 MININT4 = std::numeric_limits<int4>::min();
const int4 MAXINT4 = std::numeric_limits<int4>::max();
const int8 MININT8 = std::numeric_limits<int8>::min();
const int8 MAXINT8 = std::numeric_limits<int8>::max();
const uint1 MAXUINT1 = std::numeric_limits<uint1>::max();
const uint2 MAXUINT2 = std::numeric_limits<uint2>::max();
const uint4 MAXUINT4 = std::numeric_limits<uint4>::max();
const uint8 MAXUINT8 = std::numeric_limits<uint8>::max();
const float MINFLOAT = std::numeric_limits<float>::min();
const float MAXFLOAT = std::numeric_limits<float>::max();

// Liczba zmiennoprzecinkowa bliska zeru
// O dziwo to makro zamienione na funkcj� inline dzia�a wolniej - nigdy nie ufaj
// optymalizacji kompilatora!
#define FLOAT_ALMOST_ZERO(F) ((absolute_cast<uint4>(F) & 0x7f800000L) == 0)

// Sta�e matematyczne
// Bo sta�e z cmath/math.h nie chc� dzia�a� mimo zdefiniowania wcze�niej USE_MATH_CONSTANTS :P
const float E         = 2.71828182845904523536f;  // e
const float LOG2E     = 1.44269504088896340736f;  // log2(e)
const float LOG10E    = 0.434294481903251827651f; // log10(e)
const float LN2       = 0.693147180559945309417f; // ln(2)
const float LN10      = 2.30258509299404568402f;  // ln(10)
const float PI        = 3.14159265358979323846264338327950288419716939937510582f;  // pi
const float PI_2      = 1.57079632679489661923f;  // pi/2
const float PI_4      = 0.785398163397448309616f; // pi/4
const float PI_X_2    = 6.28318530717958647692f;  // 2*pi
const float _1_PI     = 0.318309886183790671538f; // 1/pi
const float _2_PI     = 0.636619772367581343076f; // 2/pi
const float _2_SQRTPI = 1.12837916709551257390f;  // 2/sqrt(pi)
const float SQRT2     = 1.41421356237309504880f;  // sqrt(2)
const float SQRT3     = 1.7320508075688772935274463415059f; // sqrt(3)
const float SQRT1_2   = 0.707106781186547524401f; // 1/sqrt(2)

// Zwraca true, je�li liczba jest niezwyczajna (INF lub NaN)
bool is_finite(float x);
bool is_finite(double x);
// Zwraca true, je�li liczba jest NaN
bool is_nan(float x);
bool is_nan(double x);

// Dodaje dwie liczby z ograniczeniem zakresu zamiast zawini�cia przy przepe�nieniu
// Jako T u�ywa� uint1, uint2, uint4, uint8
// (Autor: Tarlandil)
template <typename T>
T safe_add(T a, T b)
{
	T R = a + b;
	if (R < std::max(a, b)) return std::numeric_limits<T>::max();
	else return R;
}

// Odejmuje dwie liczby z ograniczeniem zakresu zamiast zawini�cia przy przepe�nieniu
// Jako T u�ywa� uint1, uint2, uint4, uint8
// (Autor: Tarlandil)
template <typename T>
T safe_sub(T a, T b)
{
	if (b > a) return T();
	else return a - b;
}

// Mno�y dwie liczby z ograniczeniem zakresu zamiast zawini�cia przy przepe�nieniu
// Jako T u�ywa� uint1, uint2, uint4, uint8
// (Autor: Tarlandil)
template <typename T>
T safe_mul(T a, T b)
{
	if (b == T()) return T();
	T R = a * b;
	if (R / b != a) return std::numeric_limits<T>::max();
	else return R;
}

// Bezpieczny arcus cosinus, ogranicza zakres wej�ciowy do -1...+1 zwracaj�c
// w razie przekrocznia skrajne warto�ci wyniku.
inline float safe_acos(float x)
{
	if (x <= -1.0f) return PI;
	if (x >= 1.0f) return 0.0f;
	return acosf(x);
}

// Zaokr�gla liczb�, zamiast j� obcina�
// Tak matematycznie, czyli do g�ry lub w d� zale�nie od cz�ci u�amkowej.
inline int round(float x)
{
	return static_cast<int>(floorf(x+0.5f));
}
inline int round(double x)
{
	return static_cast<int>(floor(x+0.5));
}

// Dzieli 2 liczby ca�kowite zaokr�glaj�c wynik w g�r�
// Jako typ stosowa� int, uint itp.
// Dzia�a tylko dla liczb dodatnich.
// Uwa�a� na przekroczenie zakresu liczby (x+y).
// (Author: Tarlandil)
template <typename T>
inline T ceil_div(T x, T y)
{
	return (x+y-1) / y;
}

// Zwraca true, je�li liczba le�y w epsilonowym otoczeniu zera
inline bool around(float x, float epsilon)
{
	return (fabsf(x) <= epsilon);
}
inline bool around(double x, double epsilon)
{
	return (fabs(x) <= epsilon);
}

// Zwraca true, je�li liczba le�y w epsilonowym otoczeniu drugiej liczby
inline bool around(float x, float y, float epsilon)
{
	return (fabsf(x-y) <= epsilon);
}
inline bool around(double x, double y, double epsilon)
{
	return (fabs(x-y) <= epsilon);
}

// Zwraca true, je�li dwie liczby zmiennoprzecinkowe s� praktycznie r�wne
// Te funkcje s� fajne bo nie trzeba zna� epsilona, ale za to jest wi�cej liczenia.
// (Autor: Tarlandil)
inline bool float_equal(float x, float y)
{
	float epsilon = (fabsf(x)+fabsf(y)) * 1e-4f;
	//return around(x, y, epsilon);
	return (fabsf(x-y) <= epsilon);
}
inline bool double_equal(double x, double y)
{
	double epsilon = (fabs(x)+fabs(y)) * 1e-13;
	//return around(x, y, epsilon);
	return (fabs(x-y) <= epsilon);
}

// Zwraca najmniejsz� pot�g� dw�jki wi�ksz� lub r�wn� podanej liczbie
// (Autor: Tarlandil)
inline uint4 greater_power_of_2(uint4 x)
{
	// x & (x-1) zeruje najm�odsz� jedynk�, czyli zwraca 0 wtedy i tylko wtedy, gdy x jest pot�g� 2 b�d� 0.
	if (!(x & (x-1))) return x;
	int R[16]={0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
	uint4 r=0;
	if (x&0xFFFF0000) {r|=16;x>>=16;}
	if (x&0xFF00) {r|=8;x>>=8;}
	if (x&0xF0) {r|=4;x>>=4;}
    return 1<<(r+R[x]);
}

// Liczy pot�ge o ca�kowitym wyk�adniku (bardzo szybko!)
// T mo�e by� dowoln� liczb� - int, uint4, float itp.
// (Autor: Tarlandil)
template <typename T>
T powi(T a, uint4 b)
{
	T r = (T)1;
	T p = a;
	while (b)
	{
		if (b & 1)
			r *= p;
		p *= p;
		b >>= 1;
	}
	return r;
}

// Zwraca najwi�ksz� z podanych 3 liczb
template <typename T>
inline T max3(const T &a, const T &b, const T &c)
{
	return std::max(a, std::max(b, c));
}

// Zwraca najmniejsz� z podanych 3 liczb
template <typename T>
inline T min3(const T &a, const T &b, const T &c)
{
	return std::min(a, std::min(b, c));
}

// Zwraca liczb� x ograniczon� do podanego zakresu od a do b
template <typename T>
inline T minmax(const T &a, const T &x, const T &b)
{
	if (x < a) return a; else if (x > b) return b; else return x;
	//return std::min(b, std::max(a, x));
}

// Zwraca cz�� ca�kowit� liczby wraz ze znakiem
// (Autor: Tarlandil)
inline float trunc(float x)
{
	return (x < 0.0f) ? ceilf(x) : floorf(x);
}
inline double trunc(double x)
{
	return (x < 0.0) ? ceil(x) : floor(x);
}

// Zwraca cz�� u�amkow� liczby wraz ze znakiem
// (Autor: Tarlandil)
inline float frac(float x)
{
	return x - trunc(x);
}
inline double frac(double x)
{
	return x - trunc(x);
}

// Interpolacja liniowa 1D
// t = 0..1
inline float Lerp(float x, float y, float t)
{
	return x + t*(y-x);
}
// t jest ograniczane do 0..1
inline float SafeLerp(float x, float y, float t)
{
	return Lerp(x, y, minmax(0.0f, t, 1.0f));
}

// Interpolacja liniowa 2D
// tx tyczy si� pierwszego indeksu, ty drugiego.
inline float Lerp2D(float x11, float x21, float x12, float x22, float tx, float ty)
{
	return Lerp(Lerp(x11, x21, tx), Lerp(x12, x22, tx), ty);
}

// Normalizuje k�t do przedzia�u < 0..2PI )
inline float NormalizeAngle(float angle)
{
	angle /= PI_X_2;
	angle = frac(angle);
	if (angle < 0.0f)
		angle += 1.0f;
	else if (angle >= 1.0f)
		angle -= 1.0f;
	return angle * PI_X_2;
}

// Normalizuje k�t do przedzia�u -PI..+PI
inline float NormalizeAngle2(float Angle)
{
	Angle += PI;
	Angle -= floorf(Angle * (1.0f/PI_X_2)) * PI_X_2;
	Angle -= PI;
	return Angle;
}

// Przelicza k�t ze stopni na radiany
inline float DegToRad(float Angle)
{
	// Angle * PI / 180
	return Angle * 0.017453292519943295769222222222222f;
}
// Przelicza k�t z radian�w na stopnie
inline float RadToDeg(float Angle)
{
	// Angle * 180 / PI
	return Angle * 57.295779513082320876846364344191f;
}

// Oblicza r�nic� k�t�w A-B, gdzie A, B nale�� do: 0..2*PI
// Uwzgl�dnia przekr�canie si� k�t�w, znajduje wi�c mniejsz� r�nic� mi�dzy nimi.
inline float AngleDiff(float A, float B)
{
	float R1 = B - A;
	float R2 = B - PI_X_2 - A;
	float R3 = B + PI_X_2 - A;

	float R1a = fabsf(R1);
	float R2a = fabsf(R2);
	float R3a = fabsf(R3);

	if (R1a < R2a && R1a < R3a)
		return R1;
	else if (R2a < R3a)
		return R2;
	else
		return R3;
}

// Funkcja wyg�adzaj�ca - Critically Damped Smoothing
// To jest wyliczane wg r�wnania r�niczkowego na jak��tam spr�yn�, dok�adnie
// analitycznie, wi�c TimeDelta mo�e by� dowolnie du�e.
// InOutPos   - pozycja przed i po przeliczeniu tego kroku
// InOutVel   - pr�dko�� przed i po przeliczeniu tego kroku
// Dest       - pozycja docelowa
// SmoothTime - wsp�czynnik "bezw�adno�ci" w jednostkach czasu
// TimeDelta  - czas kroku (np. czas od poprzedniej klatki)
// MaxSpeed   - maksymalna dopuszczalna pr�dko�� (maksymalna odleg�o�� na jak� pozycja mo�e nie nad��a�?)
void SmoothCD(float *InOutPos, float Dest, float *InOutVel, float SmoothTime, float TimeDelta);
void SmoothCD(float *InOutPos, float Dest, float *InOutVel, float SmoothTime, float TimeDelta, float MaxSpeed);

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Parser wiersza polece�

// Po szczeg�y patrz dokumentacja - Base.txt.

class CmdLineParser_pimpl;

class CmdLineParser
{
private:
	scoped_ptr<CmdLineParser_pimpl> pimpl;

public:
	enum RESULT
	{
		RESULT_OPT,       // Opcja
		RESULT_PARAMETER, // Go�y parametr bez opcji
		RESULT_END,       // Koniec parametr�w
		RESULT_ERROR,     // B��d sk�adni
	};

	// Wersja dla main(int argc, char **argv)
	CmdLineParser(int argc, char **argv);
	// Wersja dla WinMain(HINSTANCE Instance, HINSTANCE, char *CmdLine, int CmdShow)
	CmdLineParser(const char *CmdLine);

	~CmdLineParser();

	// Rejestruje opcj�
	// Jako Id podawa� liczby wi�ksze od 0.
	// - Rejestruje opcj� jednoznakow�
	void RegisterOpt(uint Id, char Opt, bool Parameter);
	// - Rejestruje opcj� wieloznakow�
	void RegisterOpt(uint Id, const string &Opt, bool Parameter);

	// Wczytuje nast�pn� informacj�, zwraca jej rodzaj
	RESULT ReadNext();

	// Zwraca dane na temat ostatnio wczytanej informacji
	// - Identyfikator opcji. Je�li nie zosta�a wczytana opcja, zwraca 0.
	uint GetOptId();
	// - Tre�� parametru. Je�li nie ma, �a�cuch pusty.
	const string & GetParameter();
};

} // namespace common


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Uniwersalny i rozszerzalny mechanizm konwersji na i z �a�cucha

template <typename T>
struct SthToStr_obj
{
	void operator () (string *Str, const T &Sth)
	{
		// Runtime error
		assert(0 && "SthToStr: Unsupported type.");
		// Compilation error
		// Dzia�a tylko w Visual C++, g++ pokazuje b��d kompilacji nawet kiedy nieu�yte
		int y = UnsupportedTypeInSthToStr;
	}
	static inline bool IsSupported() { return false; }
};

template <typename T>
inline void SthToStr(string *Str, const T &Sth)
{
	SthToStr_obj<T>()(Str, Sth);
}

template <typename T>
struct StrToSth_obj
{
	bool operator () (const string &Str, T *Sth)
	{
		// Runtime error
		assert(0 && "StrToSth: Unsupported type.");
		// Compilation error
		// Dzia�a tylko w Visual C++, g++ pokazuje b��d kompilacji nawet kiedy nieu�yte
		int y = UnsupportedTypeInSthToStr;
		return false;
	}
	static inline bool IsSupported() { return false; }
};

template <typename T>
inline bool StrToSth(T *Sth, const string &Str)
{
	return StrToSth_obj<T>()(Sth, Str);
}


namespace common
{

class Format_pimpl
{
	friend class Format;

private:
	string m_String;
	char m_Sep;
	size_t m_Index;

	Format_pimpl(const string &Fmt, char Sep) : m_String(Fmt), m_Sep(Sep), m_Index(0) { }
};

class Format
{
private:
	shared_ptr<Format_pimpl> pimpl;

public:
	Format(const string &Fmt, char Sep = '#') : pimpl(new Format_pimpl(Fmt, Sep)) { }
	// Dodaje kolejn� informacj� do formatowanego �a�cucha
	// - To taki jakby dziwny konstuktor kopiuj�cy.
	// - U�ywam za ka�dym razem tworzenia osobnego obiektu, bo nie mo�na w g++
	//   modyfikowa� obiektu tymczasowego i nie da�oby si� tego tak wygodnie
	//   u�ywa�.
	// - Do u�ytku wewn�trznego - nie u�ywa�!
	Format(const Format &f, const string &Element);
	Format(const Format &f, const char *Element);

	operator string () const { return pimpl->m_String; }
	string str() const { return pimpl->m_String; }
};

template <typename T>
const Format operator % (const Format &fmt, const T &x)
{
	string s;
	SthToStr<T>(&s, x);
	return Format(fmt, s);
}

inline const Format operator % (const Format &fmt, const char *x)
{
	return Format(fmt, x);
}

} // namespace common


//------------------------------------------------------------------------------
// SthToStr

template <>
struct SthToStr_obj<bool>
{
	void operator () (string *Str, const bool &Sth)
	{
		common::BoolToStr(Str, Sth, 'f');
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<char>
{
	void operator () (string *Str, const char &Sth)
	{
		common::CharToStr(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<int2>
{
	void operator () (string *Str, const int2 &Sth)
	{
		common::IntToStr<int2>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<int4>
{
	void operator () (string *Str, const int4 &Sth)
	{
		common::IntToStr<int4>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};
template <>
struct SthToStr_obj<long>
{
	void operator () (string *Str, const long &Sth)
	{
		common::IntToStr<int4>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<int8>
{
	void operator () (string *Str, const int8 &Sth)
	{
		common::IntToStr<int8>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<uint1>
{
	void operator () (string *Str, const uint1 &Sth)
	{
		common::UintToStr<uint1>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<uint2>
{
	void operator () (string *Str, const uint2 &Sth)
	{
		common::UintToStr<uint2>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<uint4>
{
	void operator () (string *Str, const uint4 &Sth)
	{
		common::UintToStr<uint4>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};
template <>
struct SthToStr_obj<unsigned long>
{
	void operator () (string *Str, const unsigned long &Sth)
	{
		common::UintToStr<uint4>(Str, (uint4)Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<uint8>
{
	void operator () (string *Str, const uint8 &Sth)
	{
		common::UintToStr<uint8>(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<float>
{
	void operator () (string *Str, const float &Sth)
	{
		common::FloatToStr(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<double>
{
	void operator () (string *Str, const double &Sth)
	{
		common::DoubleToStr(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<string>
{
	void operator () (string *Str, const string &Sth)
	{
		*Str = Sth;
	}
	static inline bool IsSupported() { return true; }
};

template <typename T>
struct SthToStr_obj< std::vector<T> >
{
	void operator () (string *Str, const std::vector<T> &Sth)
	{
		Str->clear();
		string Element;
		for (typename std::vector<T>::const_iterator it = Sth.begin(); it != Sth.end(); ++it)
		{
			if (!Str->empty())
				*Str += ',';
			SthToStr<T>(&Element, *it);
			*Str += Element;
		}
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<char *>
{
	void operator () (string *Str, char *Sth)
	{
		*Str = Sth;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<const char *>
{
	void operator () (string *Str, const char *Sth)
	{
		*Str = Sth;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<void *>
{
	void operator () (string *Str, void *Sth)
	{
		common::PtrToStr(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<const void *>
{
	void operator () (string *Str, const void *Sth)
	{
		common::PtrToStr(Str, Sth);
	}
	static inline bool IsSupported() { return true; }
};

//------------------------------------------------------------------------------
// SthToStr

template <>
struct StrToSth_obj<bool>
{
	bool operator () (bool *Sth, const string &Str)
	{
		return common::StrToBool(Sth, Str);
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<char>
{
	bool operator () (char *Sth, const string &Str)
	{
		if (Str.length() != 1) return false;
		*Sth = Str[0];
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<int2>
{
	bool operator () (int2 *Sth, const string &Str)
	{
		return common::StrToInt<int2>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<int4>
{
	bool operator () (int4 *Sth, const string &Str)
	{
		return common::StrToInt<int4>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};
template <>
struct StrToSth_obj<long>
{
	bool operator () (long *Sth, const string &Str)
	{
		return common::StrToInt<int4>((int4*)Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<int8>
{
	bool operator () (int8 *Sth, const string &Str)
	{
		return common::StrToInt<int8>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<uint1>
{
	bool operator () (uint1 *Sth, const string &Str)
	{
		return common::StrToUint<uint1>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<uint2>
{
	bool operator () (uint2 *Sth, const string &Str)
	{
		return common::StrToUint<uint2>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<uint4>
{
	bool operator () (uint4 *Sth, const string &Str)
	{
		return common::StrToUint<uint4>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};
template <>
struct StrToSth_obj<unsigned long>
{
	bool operator () (unsigned long *Sth, const string &Str)
	{
		return common::StrToUint<uint4>((uint4*)Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<uint8>
{
	bool operator () (uint8 *Sth, const string &Str)
	{
		return common::StrToUint<uint8>(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<float>
{
	bool operator () (float *Sth, const string &Str)
	{
		return common::StrToFloat(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<double>
{
	bool operator () (double *Sth, const string &Str)
	{
		return common::StrToDouble(Sth, Str) == 0;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<string>
{
	bool operator () (string *Sth, const string &Str)
	{
		*Sth = Str;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <typename T>
struct StrToSth_obj< std::vector<T> >
{
	bool operator () (std::vector<T> *Sth, const string &Str)
	{
		Sth->clear();
		uint4 Index = 0;
		string Element;
		T Val;
		while (common::Split(Str, ",", &Element, &Index))
		{
			if (!StrToSth<T>(&Val, Element)) return false;
			Sth->push_back(Val);
		}
		return true;
	}
	static inline bool IsSupported() { return true; }
};

//---------------------------------------------------------------------------------
// NEW

typedef const char* cstring;

cstring format(cstring str, ...);
