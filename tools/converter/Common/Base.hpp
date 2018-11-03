/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Base - Modu³ podstawowy
 * Dokumentacja: Patrz plik doc/Base.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

// G³ówne includy
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

// Niechciane includy
#include <limits> // :(
#include <cmath> // :(

// To jest na wypadek w³¹czania gdzieœ <windows.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// String chcê mieæ jak typ wbudowany
using std::string;

// Wy³¹cz g³upie warningi Visual C++
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

// Wektor stringów
typedef std::vector<string> STRING_VECTOR;

// Jeœli wskaŸnik niezerowy, zwalnia go i zeruje
#define SAFE_DELETE(x) { delete (x); (x) = 0; }
#define SAFE_DELARR(x) { delete [] (x); (x) = 0; }
#define SAFE_RELEASE(x) { if (x) { (x)->Release(); (x) = 0; } }

// Uniwersalny, brakuj¹cy w C++ operator dos³ownego rzutowania (reintepretacji)
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
// Ogólne

// Zwraca true, jeœli podana liczba jest potêg¹ dwójki.
// T musi byæ liczb¹ ca³kowit¹ bez znaku - uint1, uint2, uint4, uint8, albo
// liczb¹ ze znakiem ale na pewno dodatni¹.
template <typename T>
inline bool IsPow2(T x)
{
	return (x & (x-1)) == 0;
}

// Zwraca logarytm dwójkowy z podanej liczby ca³kowitej bez znaku, tzn numer
// najstarszego niezerowego bitu. Innymi s³owy, o ile bitów przesun¹æ w lewo
// jedynkê ¿eby otrzymaæ najstarsz¹ jedynkê podanej liczby.
// - Wartoœæ 0 zwraca dla liczb 0 i 1.
uint4 log2u(uint4 x);
// Zwraca maskê bitow¹ z ustawionymi na jedynkê n najm³odszymi bitami.
// n musi byæ z z zakresu 0..32.
uint4 GetBitMask(uint4 n);

// Alokuje now¹ tablicê dynamiczn¹ 2D
template <typename T>
T **new_2d(size_t cx, size_t cy)
{
	T **a = new T*[cx];
	for (size_t x = 0; x < cx; x++)
		a[x] = new T[cy];
	return a;
}

// Zwalnia tablicê dynamiczn¹ 2D
template <typename T>
void delete_2d(T **a, size_t cx)
{
	for (size_t x = 0; x < cx; x++)
		delete[] a[x];
	delete[] a;
}

// Kopiuje string do char* ³¹cznie ze znakami '\0' (czego nie zapewnia strcpy)
// - Na koñcu do³¹cza '\0'.
// - Oczywiœcie dest musi byæ dostatecznie pojemne.
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

// Zeruje pamiêæ
void ZeroMem(void *Data, size_t NumBytes);

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Inteligentne wskaŸniki

// Polityka zwalaniania, która robi: delete p;
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
// Polityka zwalaniania, która robi: delete [] p;
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

// WskaŸnik z wy³¹cznym prawem w³asnoœci.
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

// WskaŸnik ze zliczaniem referencji
// - Kopiowalny.
// - Zwalnia, kiedy zwolniony zostanie ostatni wskazuj¹cy na obiekt shared_ptr.
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
// £añcuchy

// Rodzaje znaków koñca wiersza
enum EOLMODE
{
	EOL_NONE, // Podczas konwersji pozostawienie bez zmian
	EOL_CRLF, // 13 i 10 (Windows)
	EOL_LF,   // 10      (Unix)
	EOL_CR	  // 13      (Mac)
};

// Standardowy ³añcuch koñca wiersza zale¿ny od systemu
extern const char * const EOL;

// Strona kodowa polskich znakow
enum CHARSET
{
	CHARSET_NONE = 0, // Brak polskich liter
	CHARSET_WINDOWS,  // Strona kodowa Windows-1250 (u¿ywana w GUI Windows)
	CHARSET_ISO,      // Strona kodowa ISO-8859-2 (Latin-2) (u¿ywana w Linuksie)
	CHARSET_IBM,      // Strona kodowa IBM (CP852) (u¿ywana na konsoli Windows)
	CHARSET_UTF8,     // Strona kodowa UTF-8
};

// Zwraca true, jesli znak jest alfanumeryczny (litera lub cyfra) wg ustawieñ systemu
bool CharIsAlphaNumeric(char ch);
// Zwraca true, jesli znak jest alfanumeryczny wg sta³ych regu³ - cyfra, litera
// lub polska litera w kodowaniu Windows-1250
bool CharIsAlphaNumeric_f(char ch);

// Zwraca true, jesli znak jest litera
bool CharIsAlpha(char ch);
// Zwraca true, jesli znak jest cyfr¹ dziesietn¹
inline bool CharIsDigit(char ch) { return (ch >= '0' && ch <= '9'); }
// Zwraca true, jeœli znak jest cyfr¹ szesnastkow¹
// - Akceptuje ma³e i du¿e litery.
inline bool CharIsHexDigit(char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'); }
// Zwraca true, jesli znak jest mala litera
// - Nie obs³uguje stron kodowych wielobajtowych (UTF-8).
bool CharIsLower(char ch);
bool CharIsLower(char ch, CHARSET Charset);
// Zwraca true, jesli znak jest duza litera
// - Nie obs³uguje stron kodowych wielobajtowych (UTF-8).
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

// Odwraca ³añcuch w miejscu
void ReverseString(string *s);
// Obcina bia³e znaki z pocz¹tku i koñca ³añcucha w miejscu
void Trim(string *s);
// Obcina bia³e znaki z pocz¹tku i koñca ³añcucha
void Trim(string *Out, const string &s);

// UWAGA! Z nieznanych przyczyn UpperCase i LowerCase Ÿle konwertuje w stronie
// kodowej IBM. Lepiej najpierw zmieniæ wielkoœæ liter w innej stronie a potem
// przekonwertowaæ na IBM - wtedy dzia³a. Kiedyœ mo¿e naprawiê ten b³ad :/

// Jeœli to litera, konwertuje na ma³¹
// - Nie obs³uguje wielobajtowych stron kodowych (UTF-8).
char CharToLower(char ch);
char CharToLower(char ch, CHARSET Charset);
// Jeœli to litera, konwertuje na du¿¹
// - Nie obs³uguje wielobajtowych stron kodowych (UTF-8).
char CharToUpper(char ch);
char CharToUpper(char ch, CHARSET Charset);
// Konwertuje ³añcuch na ma³e litery
// - Nie obs³uguje wielobajtowych stron kodowych (UTF-8).
void LowerCase(string *s);
void LowerCase(string *s, CHARSET Charset);
inline void LowerCase(string *Out, const string &s) { *Out = s; LowerCase(Out); }
inline void LowerCase(string *Out, const string &s, CHARSET Charset) { *Out = s; LowerCase(Out, Charset); }
// Konwertuje ³añcuch na du¿e
// - Nie obs³uguje wielobajtowych stron kodowych (UTF-8).
void UpperCase(string *s);
void UpperCase(string *s, CHARSET Charset);
inline void UpperCase(string *Out, const string &s) { *Out = s; UpperCase(Out); }
inline void UpperCase(string *Out, const string &s, CHARSET Charset) { *Out = s; UpperCase(Out, Charset); }

// Zmienia znak specjalny kodowania Windows-1250 na odpowiednik normalny
// Jesli podany znak jest jednym ze znakow specjalnych Windows-1250,
// zwraca przez a_s jego normalny odpowiednik (mo¿na podaæ wskaŸnik 0) i zwraca true.
// Jeœli to nie jest taki znak, zwraca false.
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

// Rodzaj znaku koñca wiersza na odpowiadaj¹cy mu znak (lub znaki)
void EolModeToStr(string *Out, EOLMODE EolMode);

// Zamienia w ³añcuchu wszystkie wyst¹pienia jedego pod³añcucha na drugi
// Result musi byæ oczywiœcie, jak w ka¿dej takiej funkcji, innym ³añcuchem ni¿ s.
void Replace(string *result, const string &s, const string &s1, const string &s2);
// Zmienia w ³añcuchu wszystkie wyst¹pienia znaku Ch1 na Ch2
void Replace(string *Out, const string &s, char Ch1, char Ch2);
// Zmienia w ³añcuchu wszystkie wyst¹pienia znaku Ch1 na Ch2 w miejscu
void Replace(string *InOut, char Ch1, char Ch2);

// Zmienia wszelkiego rodzaju znaki koñca wiersza w poddanym tekœcie na takie same - wg podanego trybu
void ReplaceEOL(string *result, const string &s, EOLMODE EOLMode);

// Zamienia wszystkie odstêpy na pojedyncze spacje
void NormalizeWhitespace(string *result, const string &s);

// Zwraca ³añcuch powtórzony podan¹ liczbê razy
void DupeString(string *Out, const string &s, size_t count);

// Zwraca pod³añcuch z prawej strony
void RightStr(string *Out, const string &s, size_t Length);

// Zwraca liczbê wyst¹pieñ pod³añcucha
size_t SubstringCount(const string &str, const string &substr);

// Zwraca true, jeœli podane ³añcuchy s¹ równe bez rozró¿niania wielkoœci liter
bool StrEqualI(const string &s1, const string &s2);
// Zwraca true, jeœli podfragmenty podanych ³añcuchów s¹ identyczne
bool SubStrEqual(const string &s1, size_t off1, const string &s2, size_t off2, size_t length);
// Zwraca true, jeœli podfragmenty podanych ³añcuchów s¹ identyczne nie rozró¿niaj¹c wielkoœci liter
bool SubStrEqualI(const string &s1, size_t off1, const string &s2, size_t off2, size_t length);

// Czy ³añcuch zawiera znaki koñca wiersza?
bool ContainsEOL(const string &s);

// Zwraca true, jesli podany lancuch zaczyna sie od podanego podlancucha
bool StrBegins(const string &s, const string &sub, size_t Begin = 0);
// Zwraca true, jeœli podany ³añcuch koñczy siê podanym pod³añcuchem
bool StrEnds(const string &s, const string &Sub);

// Kolejne wywo³ania rozdzielaj¹ ³añcuch na kolejne elementy oddzielone w jego treœci okreœlonym pod³añcuchem
// out - [out]
// index - [in,out]
bool Split(const string &s, const string &delimiter, string *out, size_t *index);
// Dzia³a jak wy¿ej, ale ³añcuchem rozdzielaj¹cym jest ka¿dy dowolny znak z delimiters
bool SplitFirstOf(const string &s, const string &delimiters, string *out, size_t *index);
// Dzieli ³añcuch na czêœci i zwraca je jako wektor
void Split(STRING_VECTOR *Out, const string &s, const string &Delimiter);
// Kolejne wywo³ania rozdzielaj¹ ³añcuch na kolejne elementy
// oddzielone w jego treœci któr¹œ z sekwencji uznawan¹ za koniec wiersza
// (czyli na kolejne wiersze)
bool SplitEOL(const string &s, string *out, size_t *index);
// Kolejne wywo³ania rozdzielaj¹ ³añcuch na elementy
// oddzielone odstêpami uwzglêdniaj¹c ci¹gi obiête w cudzys³owy "" jako ca³oœæ
bool SplitQuery(const string &s, string *out, size_t *index);
// Sprawdza, czy podany ³ancuch jest zgodny z podan¹ mask¹ mog¹c¹ zawieraæ znaki wieloznaczne:
// '?' zastêpuje dowolny jeden znak, '*' zastêpuje dowoln¹ liczbê (tak¿e 0) dowolnych znaków.
bool ValidateWildcard(const string &Mask, const string &S, bool CaseSensitive = true, size_t MaskOff = 0, size_t SOff = 0);
// Zwraca zmiennoprzecinkow¹ trafnoœæ wyszukiwania stringa SubStr w stringu Str
// Zaawansowany algorytm, wymyœlony przeze mnie dawno temu. Bierze pod uwagê
// rzeczy takie jak: # ile razy pod³añcuch wystêpuje # czy pasuje wielkoœæ liter
// # d³ugoœæ ³añcucha i pod³añcucha # czy to ca³e s³owo # czy to ca³y ³añcuch.
float FineSearch(const string &SubStr, const string &Str);
// Odleg³oœæ edycyjna miêdzy dwoma ³añcuchami.
// Z³o¿onoœæ: O( s1.length * s2.length )
// Im mniejsza, tym ³añcuchy bardziej podobne. Jeœli identyczne, zwraca 0.
size_t LevenshteinDistance(const string &s1, const string &s2);
// Odleg³oœæ edycyjna miêdzy dwoma ³añcuchami bez uwzglêdniania wielkoœci znaków
// Z³o¿onoœæ: O( s1.length * s2.length )
// Im mniejsza, tym ³añcuchy bardziej podobne. Jeœli identyczne, zwraca 0.
size_t LevenshteinDistanceI(const string &s1, const string &s2);

// Sortowanie naturalne, czyli takie w którym np. "abc2" jest przed "abc120"
class StringNaturalCompare
{
private:
	bool m_CaseInsensitive;

	int CompareNumbers(const string &s1, size_t i1, const string &s2, size_t i2);

public:
	StringNaturalCompare(bool CaseInsensitive) : m_CaseInsensitive(CaseInsensitive) { }
	int Compare(const string &s1, const string &s2);
};

// Funktory do sortowania ³añcuchów
// Przyk³ad u¿ycia:
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

// Predykaty do porównywania ³añcuchów bez rozró¿niania wielkoœci liter.
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
// Œcie¿ki do plików

const char DIR_SEP = '\\';

// Zwraca true, jeœli œcie¿ka jest bezwzglêdna
bool PathIsAbsolute(const string &s);
// Dodaje do sciezki konczacy '/' lub '\', jesli go nie ma - na miejscu
void IncludeTrailingPathDelimiter(string *InOutPath);
// Dodaje do sciezki konczacy '/' lub '\', jesli go nie ma - do nowego stringa
void IncludeTrailingPathDelimiter(string *OutPath, const string &InPath);
// Pozbawia sciezke konczacego '/' lub '\', jesli go ma - na miejscu
void ExcludeTrailingPathDelimiter(string *InOutPath);
// Pozbawia sciezke konczacego '/' lub '\', jesli go ma - do nowego stringa
void ExcludeTrailingPathDelimiter(string *OutPath, const string &InPath);
// Zwraca pocz¹tek œcie¿ki, w postaci takiej jak "C:\", "\\komputer\udzia³\" czy "/" albo "\"
// Jeœli œcie¿ka jest wzglêdna, zwraca ³añcuch pusty.
void ExtractPathPrefix(string *OutPrefix, const string &s);
// Zwraca œcie¿kê do pliku bez nazwy pliku
void ExtractFilePath(string *OutPath, const string &s);
// Zwraca sam¹ nazwê pliku bez œcie¿ki
void ExtractFileName(string *OutFileName, const string &s);
// Zwraca rozszerzenie pliku wraz z kropk¹
void ExtractFileExt(string *OutExt, const string &s);
// Zmienia w nazwie pliku rozszerzenie na nowe
// Jesli Ext = "", usuwa rozszerzenie.
// Nowe rozszerzenie musi zawieraæ rozpoczynaj¹c¹ kropkê (tzn. nie musi, ale wypada³oby :)
void ChangeFileExt(string *Out, const string &FileName, const string &Ext);
// Pozbawia sciezke artefaktow typu ".\" czy "..\"
// Odpowiednio ja oczywiscie przetwarza tak, ze wyjscie jest logicznie rownowazne wejsciu.
void NormalizePath(string *OutPath, const string &s);
// Przetwarza sciezke wzgledna na bezwzgledna wzgledem okreslonej sciezki
// Na przyk³ad Base="C:\Kat", Path="PodKat\Plik.txt", Wynik="C:\Kat\PodKat\Plik.txt".
void RelativeToAbsolutePath(string *Out, const string &Base, const string &Path);
// Przetwarza sciezke bezwzgledna na wzgledna wzgledem okreslonej sciezki
// Jeœli siê nie da, wychodzi równie¿ bezwzglêdna, równa Target.
// Na przyk³ad Base="C:\Kat", Tareget="C:\Kat\Podkat\Plik.txt", Wynik="PodKat\Plik.txt".
void AbsoluteToRelativePath(string *Out, const string &Base, const string &Target);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KONWERSJE

extern const char _DIGITS_L[];
extern const char _DIGITS_U[];

// Zwraca liczbowy odpowiednik cyfry szesnastkowej.
// Jeœli b³¹d, zwraca 0xFF.
// Akceptuje zarówno ma³e, jak i du¿e litery.
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

// Konwersja liczby ca³kowitej na ³añcuch
// Base musi byæ z zakresu 2..36
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

	// Podstawa to potêga dwójki - bêdzie proœciej, bo przesuwanie bitowe zamiast dzielenia
	if (IsPow2<uint4>(Base))
	{
		// O ile bitów przesuwaæ?
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
	// Podstawa to nie potêga dwójki
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
		// Podstawa to potêga dwójki - bêdzie proœciej, bo przesuwanie bitowe zamiast dzielenia
		if (IsPow2<uint4>(Base))
		{
			// O ile bitów przesuwaæ?
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
		// Podstawa to nie potêga dwójki
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
	// Liczba ujemna - tu bêdzie musia³o byæ inaczej
	// Modulo jest UB, wiêc u¿yjemy obejœcia.
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

// Konwersja ³añcucha na liczbê ca³kowit¹
// Zwraca:
//  0 - sukces
// -1 - b³¹d sk³adni ³añcucha
// -2 - przekroczenie zakresu
// Convert digit in any base ('0'..'9', 'A'..'Z', 'a'..'z') to number, return false on error
// [Wewnêtrzna]
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

// Konwersja liczby na ³añcuch o minimalnej podanej d³ugoœci.
// Zostanie do tej d³ugoœci uzupe³niony zerami.
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

// Konwertuje znak na ³añcuch, jako ¿e automatycznie to siê niestety nie odbywa
inline void CharToStr(string *Out, char ch) { Out->clear(); *Out += ch; }
inline string CharToStrR(char ch) { string s; s += ch; return s; }

// Konwersja liczb zmiennoprzecinkowych na ³añcuch
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

// Konwersja ³añcucha na liczbê zmiennoprzecinkow¹.
// - W przypadku b³êdu sk³adni zwracaj¹ wartoœæ != 0.
// - B³êdy zakresu nie s¹ sprawdzane - wynik niezdefiniowany.
int StrToDouble(double *out, const string &s);
int StrToFloat (float *out,  const string &s);

// Konwertuje wartoœæ logiczn¹ na ³añcuch
// mode:
// - '0' : 0 / 1
// - 'f' : false / true
// - 'F' : False / True
// - 'U' : FALSE / TRUE
// - 'g' : f / t
// - 'G' : F / T
void BoolToStr(string *Out, bool x, char mode = 'f');
inline string BoolToStrR(bool x, char mode = 'f') { string R; BoolToStr(&R, x, mode); return R; }
// Konwertuje ³añcuch na wartoœæ logiczn¹
// - Dozwolone wszelkie wartoœci jak w BoolToStr.
// - B³¹d: zwraca false.
bool StrToBool(bool *result, const string &s);

// Konwertuje wskaŸnik na ³añcuch 8 znaków zapisany szesnastkowo
void PtrToStr(string *Out, const void* p);
inline string PtrToStrR(const void *p) { string R; PtrToStr(&R, p); return R; }

// Liczba bajtów na ³añcuch z rozmiarem np. "1 B", "10.5 kB"
// - Space - czy miêdzy liczbê a jednostkê dawaæ spacjê
// - Precision - iloœæ cyfr po przecinku (mo¿e byæ te¿ 0)
// - Jako T uzywac uint1, uint2, uint4, uint8 lub int ale na pewno wartoœci dodaniej.
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

// Zatrzymuje bie¿¹cy w¹tek na podan¹ liczbê milisekund
// - Dzia³a z jak¹œtam dok³adnoœci¹ - mo¿e nie super, ale na pewno wiêksz¹ ni¿
//   ca³a sekunda.
void Wait(uint4 Miliseconds);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Matematyka

// Maksymalne i minimalne wartoœci liczb ró¿nych typów
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
// O dziwo to makro zamienione na funkcjê inline dzia³a wolniej - nigdy nie ufaj
// optymalizacji kompilatora!
#define FLOAT_ALMOST_ZERO(F) ((absolute_cast<uint4>(F) & 0x7f800000L) == 0)

// Sta³e matematyczne
// Bo sta³e z cmath/math.h nie chc¹ dzia³aæ mimo zdefiniowania wczeœniej USE_MATH_CONSTANTS :P
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

// Zwraca true, jeœli liczba jest niezwyczajna (INF lub NaN)
bool is_finite(float x);
bool is_finite(double x);
// Zwraca true, jeœli liczba jest NaN
bool is_nan(float x);
bool is_nan(double x);

// Dodaje dwie liczby z ograniczeniem zakresu zamiast zawiniêcia przy przepe³nieniu
// Jako T u¿ywaæ uint1, uint2, uint4, uint8
// (Autor: Tarlandil)
template <typename T>
T safe_add(T a, T b)
{
	T R = a + b;
	if (R < std::max(a, b)) return std::numeric_limits<T>::max();
	else return R;
}

// Odejmuje dwie liczby z ograniczeniem zakresu zamiast zawiniêcia przy przepe³nieniu
// Jako T u¿ywaæ uint1, uint2, uint4, uint8
// (Autor: Tarlandil)
template <typename T>
T safe_sub(T a, T b)
{
	if (b > a) return T();
	else return a - b;
}

// Mno¿y dwie liczby z ograniczeniem zakresu zamiast zawiniêcia przy przepe³nieniu
// Jako T u¿ywaæ uint1, uint2, uint4, uint8
// (Autor: Tarlandil)
template <typename T>
T safe_mul(T a, T b)
{
	if (b == T()) return T();
	T R = a * b;
	if (R / b != a) return std::numeric_limits<T>::max();
	else return R;
}

// Bezpieczny arcus cosinus, ogranicza zakres wejœciowy do -1...+1 zwracaj¹c
// w razie przekrocznia skrajne wartoœci wyniku.
inline float safe_acos(float x)
{
	if (x <= -1.0f) return PI;
	if (x >= 1.0f) return 0.0f;
	return acosf(x);
}

// Zaokr¹gla liczbê, zamiast j¹ obcinaæ
// Tak matematycznie, czyli do góry lub w dó³ zale¿nie od czêœci u³amkowej.
inline int round(float x)
{
	return static_cast<int>(floorf(x+0.5f));
}
inline int round(double x)
{
	return static_cast<int>(floor(x+0.5));
}

// Dzieli 2 liczby ca³kowite zaokr¹glaj¹c wynik w górê
// Jako typ stosowaæ int, uint itp.
// Dzia³a tylko dla liczb dodatnich.
// Uwa¿aæ na przekroczenie zakresu liczby (x+y).
// (Author: Tarlandil)
template <typename T>
inline T ceil_div(T x, T y)
{
	return (x+y-1) / y;
}

// Zwraca true, jeœli liczba le¿y w epsilonowym otoczeniu zera
inline bool around(float x, float epsilon)
{
	return (fabsf(x) <= epsilon);
}
inline bool around(double x, double epsilon)
{
	return (fabs(x) <= epsilon);
}

// Zwraca true, jeœli liczba le¿y w epsilonowym otoczeniu drugiej liczby
inline bool around(float x, float y, float epsilon)
{
	return (fabsf(x-y) <= epsilon);
}
inline bool around(double x, double y, double epsilon)
{
	return (fabs(x-y) <= epsilon);
}

// Zwraca true, jeœli dwie liczby zmiennoprzecinkowe s¹ praktycznie równe
// Te funkcje s¹ fajne bo nie trzeba znaæ epsilona, ale za to jest wiêcej liczenia.
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

// Zwraca najmniejsz¹ potêgê dwójki wiêksz¹ lub równ¹ podanej liczbie
// (Autor: Tarlandil)
inline uint4 greater_power_of_2(uint4 x)
{
	// x & (x-1) zeruje najm³odsz¹ jedynkê, czyli zwraca 0 wtedy i tylko wtedy, gdy x jest potêg¹ 2 b¹dŸ 0.
	if (!(x & (x-1))) return x;
	int R[16]={0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
	uint4 r=0;
	if (x&0xFFFF0000) {r|=16;x>>=16;}
	if (x&0xFF00) {r|=8;x>>=8;}
	if (x&0xF0) {r|=4;x>>=4;}
    return 1<<(r+R[x]);
}

// Liczy potêge o ca³kowitym wyk³adniku (bardzo szybko!)
// T mo¿e byæ dowoln¹ liczb¹ - int, uint4, float itp.
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

// Zwraca najwiêksz¹ z podanych 3 liczb
template <typename T>
inline T max3(const T &a, const T &b, const T &c)
{
	return std::max(a, std::max(b, c));
}

// Zwraca najmniejsz¹ z podanych 3 liczb
template <typename T>
inline T min3(const T &a, const T &b, const T &c)
{
	return std::min(a, std::min(b, c));
}

// Zwraca liczbê x ograniczon¹ do podanego zakresu od a do b
template <typename T>
inline T minmax(const T &a, const T &x, const T &b)
{
	if (x < a) return a; else if (x > b) return b; else return x;
	//return std::min(b, std::max(a, x));
}

// Zwraca czêœæ ca³kowit¹ liczby wraz ze znakiem
// (Autor: Tarlandil)
inline float trunc(float x)
{
	return (x < 0.0f) ? ceilf(x) : floorf(x);
}
inline double trunc(double x)
{
	return (x < 0.0) ? ceil(x) : floor(x);
}

// Zwraca czêœæ u³amkow¹ liczby wraz ze znakiem
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
// tx tyczy siê pierwszego indeksu, ty drugiego.
inline float Lerp2D(float x11, float x21, float x12, float x22, float tx, float ty)
{
	return Lerp(Lerp(x11, x21, tx), Lerp(x12, x22, tx), ty);
}

// Normalizuje k¹t do przedzia³u < 0..2PI )
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

// Normalizuje k¹t do przedzia³u -PI..+PI
inline float NormalizeAngle2(float Angle)
{
	Angle += PI;
	Angle -= floorf(Angle * (1.0f/PI_X_2)) * PI_X_2;
	Angle -= PI;
	return Angle;
}

// Przelicza k¹t ze stopni na radiany
inline float DegToRad(float Angle)
{
	// Angle * PI / 180
	return Angle * 0.017453292519943295769222222222222f;
}
// Przelicza k¹t z radianów na stopnie
inline float RadToDeg(float Angle)
{
	// Angle * 180 / PI
	return Angle * 57.295779513082320876846364344191f;
}

// Oblicza ró¿nicê k¹tów A-B, gdzie A, B nale¿¹ do: 0..2*PI
// Uwzglêdnia przekrêcanie siê k¹tów, znajduje wiêc mniejsz¹ ró¿nicê miêdzy nimi.
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

// Funkcja wyg³adzaj¹ca - Critically Damped Smoothing
// To jest wyliczane wg równania ró¿niczkowego na jak¹œtam sprê¿ynê, dok³adnie
// analitycznie, wiêc TimeDelta mo¿e byæ dowolnie du¿e.
// InOutPos   - pozycja przed i po przeliczeniu tego kroku
// InOutVel   - prêdkoœæ przed i po przeliczeniu tego kroku
// Dest       - pozycja docelowa
// SmoothTime - wspó³czynnik "bezw³adnoœci" w jednostkach czasu
// TimeDelta  - czas kroku (np. czas od poprzedniej klatki)
// MaxSpeed   - maksymalna dopuszczalna prêdkoœæ (maksymalna odleg³oœæ na jak¹ pozycja mo¿e nie nad¹¿aæ?)
void SmoothCD(float *InOutPos, float Dest, float *InOutVel, float SmoothTime, float TimeDelta);
void SmoothCD(float *InOutPos, float Dest, float *InOutVel, float SmoothTime, float TimeDelta, float MaxSpeed);

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Parser wiersza poleceñ

// Po szczegó³y patrz dokumentacja - Base.txt.

class CmdLineParser_pimpl;

class CmdLineParser
{
private:
	scoped_ptr<CmdLineParser_pimpl> pimpl;

public:
	enum RESULT
	{
		RESULT_OPT,       // Opcja
		RESULT_PARAMETER, // Go³y parametr bez opcji
		RESULT_END,       // Koniec parametrów
		RESULT_ERROR,     // B³¹d sk³adni
	};

	// Wersja dla main(int argc, char **argv)
	CmdLineParser(int argc, char **argv);
	// Wersja dla WinMain(HINSTANCE Instance, HINSTANCE, char *CmdLine, int CmdShow)
	CmdLineParser(const char *CmdLine);

	~CmdLineParser();

	// Rejestruje opcjê
	// Jako Id podawaæ liczby wiêksze od 0.
	// - Rejestruje opcjê jednoznakow¹
	void RegisterOpt(uint Id, char Opt, bool Parameter);
	// - Rejestruje opcjê wieloznakow¹
	void RegisterOpt(uint Id, const string &Opt, bool Parameter);

	// Wczytuje nastêpn¹ informacjê, zwraca jej rodzaj
	RESULT ReadNext();

	// Zwraca dane na temat ostatnio wczytanej informacji
	// - Identyfikator opcji. Jeœli nie zosta³a wczytana opcja, zwraca 0.
	uint GetOptId();
	// - Treœæ parametru. Jeœli nie ma, ³añcuch pusty.
	const string & GetParameter();
};

} // namespace common


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Uniwersalny i rozszerzalny mechanizm konwersji na i z ³añcucha

template <typename T>
struct SthToStr_obj
{
	void operator () (string *Str, const T &Sth)
	{
		// Runtime error
		assert(0 && "SthToStr: Unsupported type.");
		// Compilation error
		// Dzia³a tylko w Visual C++, g++ pokazuje b³¹d kompilacji nawet kiedy nieu¿yte
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
		// Dzia³a tylko w Visual C++, g++ pokazuje b³¹d kompilacji nawet kiedy nieu¿yte
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
	// Dodaje kolejn¹ informacjê do formatowanego ³añcucha
	// - To taki jakby dziwny konstuktor kopiuj¹cy.
	// - U¿ywam za ka¿dym razem tworzenia osobnego obiektu, bo nie mo¿na w g++
	//   modyfikowaæ obiektu tymczasowego i nie da³oby siê tego tak wygodnie
	//   u¿ywaæ.
	// - Do u¿ytku wewnêtrznego - nie u¿ywaæ!
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
