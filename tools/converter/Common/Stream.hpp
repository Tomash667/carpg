/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Stream - Hierarchia klas strumieni
 * Dokumentacja: Patrz plik doc/Stream.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

namespace common
{

extern const size_t BUFFER_SIZE;

// [Wewn�trzne]
void _ThrowBufEndError(const char *File, int Line);

// Abstrakcyjna klasa bazowa strumieni danych binarnych
class Stream
{
public:
	// wirtualny destruktor (klasa jest polimorficzna)
	virtual ~Stream() { }

	// ======== ZAPISYWANIE ========

	// Zapisuje dane
	// (W oryginale: zg�asza b��d)
	virtual void Write(const void *Data, size_t Size);
	// Domy�lnie nie robi nic.
	virtual void Flush() { }

	// Zapisuje dane, sama odczytuje rozmiar przekazanej zmiennej
	template <typename T>
	void WriteEx(const T &x) { return Write(&x, sizeof(x)); }
	// Zapisuje rozmiar na 1B i �a�cuch
	// Je�li �a�cuch za d�ugi - b��d.
	void WriteString1(const string &s);
	// Zapisuje rozmiar na 2B i �a�cuch
	// Je�li �a�cuch za d�ugi - b��d.
	void WriteString2(const string &s);
	// Zapisuje rozmiar na 4B i �a�cuch
	void WriteString4(const string &s);
	// Zapisuje �a�cuch bez rozmiaru
	void WriteStringF(const string &s);
	// Zapisuje warto�� logiczn� za pomoc� jednego bajtu
	void WriteBool(bool b);

	// ======== ODCZYTYWANIE ========

	// Odczytuje dane
	// Size to liczba bajt�w do odczytania
	// Zwraca liczb� bajt�w, jakie uda�o si� odczyta�
	// Je�li osi�gni�to koniec, funkcja nie zwraca b��du, a liczba odczytanych bajt�w to 0.
	// (W oryginale: zg�asza b��d)
	virtual size_t Read(void *Data, size_t MaxLength);
	// Odczytuje tyle bajt�w, ile si� za��da
	// Je�li koniec pliku albo je�li odczytano mniej, zg�asza b��d.
	// (Mo�na j� prze�adowa�, ale nie trzeba - ma swoj� wersj� oryginaln�)
	virtual void MustRead(void *Data, size_t Length);
	// Tak samo jak MustRead, ale sama odczytuje rozmiar przekazanej zmiennej
	// Zwraca true, je�li osi�gni�to koniec strumienia
	// (W oryginale: zg�asza b��d)
	virtual bool End();
	// Pomija co najwy�ej podan� liczb� bajt�w (chyba �e wcze�niej osi�gni�to koniec).
	// Zwraca liczb� pomini�tych bajt�w.
	// (Mo�na j� prze�adowa�, ale nie trzeba - ma swoj� wersj� oryginaln�)
	virtual size_t Skip(size_t MaxLength);
	// Odczytuje �a�cuch poprzedzony rozmiarem 1B
	void ReadString1(string *s);
	// Odczytuje �a�cuch poprzedzony rozmiarem 2B
	void ReadString2(string *s);
	// Odczytuje �a�cuch poprzedzony rozmiarem 4B
	void ReadString4(string *s);
	// Odczytuje �a�cuch bez zapisanego rozmiaru
	void ReadStringF(string *s, size_t Length);
	// Odczytuje tyle znak�w do �a�cucha, ile si� da do ko�ca strumienia
	void ReadStringToEnd(string *s);
	// Odczytuje warto�� logiczn� za pomoc� jednego bajtu
	void ReadBool(bool *b);
	// Pomija koniecznie podan� liczb� bajt�w.
	// Je�li wcze�niej osi�gni�to koniec, zg�asza b��d.
	void MustSkip(size_t Length);

	// ======== KOPIOWANIE ========
	// Odczytuje podan� co najwy�ej liczb� bajt�w z podanego strumienia
	// Zwraca liczb� odczytanych bajt�w
	size_t CopyFrom(Stream *s, size_t Size);
	// Odczytuje dok�adnie podan� liczb� bajt�w z podanego strumienia
	// Je�li odczyta mniej, zg�asza b��d.
	void MustCopyFrom(Stream *s, size_t Size);
	// Odczytuje dane do ko�ca z podanego strumienia
	void CopyFromToEnd(Stream *s);
};

// Abstrakcyjna klasa bazowa strumieni pozwalaj�cych na odczytywanie rozmiaru i zmian� pozycji
class SeekableStream : public Stream
{
public:
	// ======== Implementacja Stream ========
	// (W Stream domy�lnie rzuca b��d, tu w SeekableStream dostaje ju� wersj� oryginaln�. Mo�na j� dalej nadpisa�.)
	virtual bool End();
	// (W Stream ma wersj� oryginaln�, tu w SeekableStream dostaje lepsz�. Mo�na j� dalej nadpisa�.)
	virtual size_t Skip(size_t MaxLength);

	// Zwraca rozmiar
	// (W oryginale: zg�asza b��d)
	virtual size_t GetSize();
	// Zwraca pozycj� wzgl�dem pocz�tku
	// (W oryginale: zg�asza b��d)
	virtual int GetPos();
	// Ustawia pozycj� wzgl�dem pocz�tku
	// pos musi by� dodatnie.
	// (W oryginale: zg�asza b��d)
	virtual void SetPos(int pos);
	// Ustawia pozycj� wzgl�dem bie��cej
	// pos mo�e by� dodatnie albo ujemne.
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void SetPosFromCurrent(int pos);
	// Ustawia pozycj� wzgl�dem ko�cowej
	// Np. abu ustawi� na ostatni znak, przesu� do -1.
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void SetPosFromEnd(int pos);
	// Ustawia pozycj� na pocz�tku
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void Rewind();
	// Ustawia rozmiar na podany
	// Po tym wywo�aniu pozycja kursora jest niezdefiniowana.
	// (W oryginale: zg�asza b��d)
	virtual void SetSize(size_t Size);
	// Obcina strumie� od bie��cej pozycji do ko�ca, czyli ustawia taki rozmiar jak bie��ca pozycja.
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void Truncate();
	// Opr�nia strumie�
	// (Mo�na j� nadpisa�, ale ma wersj� oryginaln�)
	virtual void Clear();
};

// Nak�adka przyspieszaj�ca odczytuj�ca ze strumienia pojedyncze znaki i inne rzeczy.
/*
- Nie jest strumieniem.
- Tylko odczyt.
- Jest szybka, poniewa�:
  1. Nie ma metod wirtualnych.
  2. Buforuje, wi�c nie u�ywa za ka�dym razem metod wirtualnych strumienia.
- Szybko�� wzgl�dem wczytywania pojedynczych znak�w prosto ze strumienia: Debug = 10 razy, Release = 450 razy!
- Celowo nie u�ywam w tej klasie ERR_TRY .. ERR_CATCH_FUNC, �eby by�o szybciej.
*/
class CharReader
{
private:
	Stream *m_Stream;
	std::vector<char> m_Buf;
	// Miejsce, do kt�rego doczyta�em z bufora
	uint m_BufBeg;
	// Miejsce, do kt�rego bufor jest wype�niony danymi
	uint m_BufEnd;

	// Wczytuje now� porcj� danych do strumienia
	// Wywo�ywa� tylko kiedy bufor si� sko�czy�, tzn. m_BufBeg == m_BufEnd.
	// Je�li sko�czy� si� strumie� i nic nie uda�o si� wczyta�, zwraca false.
	// Je�li nie, zapewnia �e w buforze b�dzie co najmniej jeden znak.
	bool EnsureNewChars();

public:
	CharReader(Stream *a_Stream) : m_Stream(a_Stream), m_BufBeg(0), m_BufEnd(0) { m_Buf.resize(BUFFER_SIZE); }

	// Czy jeste�my na ko�cu danych?
	bool End() {
		return (m_BufBeg == m_BufEnd) && m_Stream->End(); // Nic nie zosta�o w buforze i nic nie zosta�o w strumieniu.
	}
	// Je�li mo�na odczyta� nast�pny znak, wczytuje go i zwraca true.
	// Je�li nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool ReadChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg++]; return true; }
	// Je�li mo�na odczyta� nast�pny znak, wczytuje go.
	// Je�li nie, rzuca wyj�tek.
	char MustReadChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg++]; }
	// Je�li mo�na odczyta� nast�pny znak, podgl�da go zwracaj�c przez Out i zwraca true. Nie przesuwa "kursora".
	// Je�li nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool PeekChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg]; return true; }
	// Je�li mo�na odczyta� nast�pny znak, podgl�da go zwracaj�c. Nie przesuwa "kursora".
	// Je�li nie, rzuca wyj�tek.
	char MustPeekChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg]; }
	// Wczytuje co najwy�ej MaxLength znak�w do podanego stringa.
	// StringStream jest czyszczony - nie musi by� pusty ani zaalokowany.
	// Zwraca liczb� odczytanych znak�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t ReadString(string *Out, size_t MaxLength);
	// Wczytuje Length znak�w do podanego stringa.
	// StringStream jest czyszczony - nie musi by� pusty ani zaalokowany.
	// Je�li nie uda si� odczyta� wszystkich Length znak�w, rzuca wyj�tek.
	void MustReadString(string *Out, size_t Length);
	// Wczytuje co najwy�ej MaxLength znak�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej MaxLength.
	// Zwraca liczb� odczytanych znak�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t ReadString(char *Out, size_t MaxLength);
	// Wczytuje Length znak�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej Length.
	// Je�li nie uda si� odczyta� wszystkich Length znak�w, rzuca wyj�tek.
	void MustReadString(char *Out, size_t Length);
	// Wczytuje co najwy�ej MaxLength bajt�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej MaxLength bajt�w.
	// Zwraca liczb� odczytanych bajt�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t ReadData(void *Out, size_t MaxLength);
	// Wczytuje Length bajt�w pod podany wska�nik.
	// Out musi by� tablic� zaalokowan� do d�ugo�ci co najmniej Length bajt�w.
	// Je�li nie uda si� odczyta� wszystkich Length bajt�w, rzuca wyj�tek.
	void MustReadData(void *Out, size_t Length);
	// Je�li jest nast�pny znak do odczytania, pomija go i zwraca true.
	// Je�li nie, zwraca false. Oznacza to koniec strumienia.
	bool SkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } m_BufBeg++; return true; }
	// Je�li jest nast�pny znak do odczytania, pomija go.
	// Je�li nie, rzuca wyj�tek.
	void MustSkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } m_BufBeg++; }
	// Pomija co najwy�ej MaxLength znak�w.
	// Zwraca liczb� pomini�tych znak�w. Mniej ni� ��dano oznacza koniec strumienia.
	size_t Skip(size_t MaxLength);
	// Pomija Length znak�w.
	// Je�li nie uda si� pomin�� wszystkich Length znak�w, rzuca wyj�tek.
	void MustSkip(size_t Length);
	// Wczytuje linijk� tekstu i je�li si� uda�o, zwraca true.
	// Je�li nie uda�o si� wczyta� ani jednego znaku, bo to ju� koniec strumienia, zwraca false.
	// Czyta znaki do napotkania ko�ca strumienia lub ko�ca wiersza, kt�rym s� "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do��cza go do Out.
	bool ReadLine(string *Out);
	// Wczytuje linijk� tekstu.
	// Je�li nie uda�o si� wczyta� ani jednego znaku, bo to ju� koniec strumienia, rzuca wyj�tek.
	// Czyta znaki do napotkania ko�ca strumienia lub ko�ca wiersza, kt�rym s� "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do��cza go do Out.
	void MustReadLine(string *Out);
};

// Strumie� pami�ci statycznej
// Strumie� ma sta�� d�ugo�� i nie mo�e by� rozszerzany - ani automatycznie, ani r�cznie.
class MemoryStream : public SeekableStream
{
private:
	char *m_Data;
	size_t m_Size;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wska�nik 0, aby strumie� sam zaalkowa�, a w destruktorze zwolni� pami��.
	// Podaj wska�nik, je�li ma korzysta� z twojego obszaru pami�ci.
	MemoryStream(size_t Size, void *Data = 0);
	virtual ~MemoryStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void MustRead(void *Data, size_t Size);

	virtual size_t GetSize();
	virtual int GetPos();
	virtual void SetPos(int pos);
	virtual void Rewind();

	char *Data() { return m_Data; }
};

// Strumie� oparty na �a�cuchu std::string
// Potrafi si� rozszerza�.
class StringStream : public SeekableStream
{
private:
	std::string *m_Data;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wska�nik 0, aby strumie� sam zaalokowa�, a w destruktorze zwolni� �a�cuch.
	// Podaj wska�nik, je�li ma korzysta� z twojego obszaru pami�ci.
	StringStream(string *Data = 0);
	virtual ~StringStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void MustRead(void *Data, size_t Size);
	virtual size_t GetSize() { return m_Data->length(); }
	virtual int GetPos() { return m_Pos; }
	virtual void SetPos(int pos) { m_Pos = pos; }
	virtual void Rewind() { m_Pos = 0; }
	virtual void SetSize(size_t Size) { m_Data->resize(Size); }
	virtual void Clear() { m_Data->clear(); }

	size_t GetCapacity() { return m_Data->capacity(); }
	void SetCapacity(size_t Capacity) { m_Data->reserve(Capacity); }
	string *Data() { return m_Data; }
};

} // namespace common
