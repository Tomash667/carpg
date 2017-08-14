/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
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

// [Wewnêtrzne]
void _ThrowBufEndError(const char *File, int Line);

// Abstrakcyjna klasa bazowa strumieni danych binarnych
class Stream
{
public:
	// wirtualny destruktor (klasa jest polimorficzna)
	virtual ~Stream() { }

	// ======== ZAPISYWANIE ========

	// Zapisuje dane
	// (W oryginale: zg³asza b³¹d)
	virtual void Write(const void *Data, size_t Size);
	// Domyœlnie nie robi nic.
	virtual void Flush() { }

	// Zapisuje dane, sama odczytuje rozmiar przekazanej zmiennej
	template <typename T>
	void WriteEx(const T &x) { return Write(&x, sizeof(x)); }
	// Zapisuje rozmiar na 1B i ³añcuch
	// Jeœli ³añcuch za d³ugi - b³¹d.
	void WriteString1(const string &s);
	// Zapisuje rozmiar na 2B i ³añcuch
	// Jeœli ³añcuch za d³ugi - b³¹d.
	void WriteString2(const string &s);
	// Zapisuje rozmiar na 4B i ³añcuch
	void WriteString4(const string &s);
	// Zapisuje ³añcuch bez rozmiaru
	void WriteStringF(const string &s);
	// Zapisuje wartoœæ logiczn¹ za pomoc¹ jednego bajtu
	void WriteBool(bool b);

	// ======== ODCZYTYWANIE ========

	// Odczytuje dane
	// Size to liczba bajtów do odczytania
	// Zwraca liczbê bajtów, jakie uda³o siê odczytaæ
	// Jeœli osi¹gniêto koniec, funkcja nie zwraca b³êdu, a liczba odczytanych bajtów to 0.
	// (W oryginale: zg³asza b³¹d)
	virtual size_t Read(void *Data, size_t MaxLength);
	// Odczytuje tyle bajtów, ile siê za¿¹da
	// Jeœli koniec pliku albo jeœli odczytano mniej, zg³asza b³¹d.
	// (Mo¿na j¹ prze³adowaæ, ale nie trzeba - ma swoj¹ wersjê oryginaln¹)
	virtual void MustRead(void *Data, size_t Length);
	// Tak samo jak MustRead, ale sama odczytuje rozmiar przekazanej zmiennej
	// Zwraca true, jeœli osi¹gniêto koniec strumienia
	// (W oryginale: zg³asza b³¹d)
	virtual bool End();
	// Pomija co najwy¿ej podan¹ liczbê bajtów (chyba ¿e wczeœniej osi¹gniêto koniec).
	// Zwraca liczbê pominiêtych bajtów.
	// (Mo¿na j¹ prze³adowaæ, ale nie trzeba - ma swoj¹ wersjê oryginaln¹)
	virtual size_t Skip(size_t MaxLength);
	// Odczytuje ³añcuch poprzedzony rozmiarem 1B
	void ReadString1(string *s);
	// Odczytuje ³añcuch poprzedzony rozmiarem 2B
	void ReadString2(string *s);
	// Odczytuje ³añcuch poprzedzony rozmiarem 4B
	void ReadString4(string *s);
	// Odczytuje ³añcuch bez zapisanego rozmiaru
	void ReadStringF(string *s, size_t Length);
	// Odczytuje tyle znaków do ³añcucha, ile siê da do koñca strumienia
	void ReadStringToEnd(string *s);
	// Odczytuje wartoœæ logiczn¹ za pomoc¹ jednego bajtu
	void ReadBool(bool *b);
	// Pomija koniecznie podan¹ liczbê bajtów.
	// Jeœli wczeœniej osi¹gniêto koniec, zg³asza b³¹d.
	void MustSkip(size_t Length);

	// ======== KOPIOWANIE ========
	// Odczytuje podan¹ co najwy¿ej liczbê bajtów z podanego strumienia
	// Zwraca liczbê odczytanych bajtów
	size_t CopyFrom(Stream *s, size_t Size);
	// Odczytuje dok³adnie podan¹ liczbê bajtów z podanego strumienia
	// Jeœli odczyta mniej, zg³asza b³¹d.
	void MustCopyFrom(Stream *s, size_t Size);
	// Odczytuje dane do koñca z podanego strumienia
	void CopyFromToEnd(Stream *s);
};

// Abstrakcyjna klasa bazowa strumieni pozwalaj¹cych na odczytywanie rozmiaru i zmianê pozycji
class SeekableStream : public Stream
{
public:
	// ======== Implementacja Stream ========
	// (W Stream domyœlnie rzuca b³¹d, tu w SeekableStream dostaje ju¿ wersjê oryginaln¹. Mo¿na j¹ dalej nadpisaæ.)
	virtual bool End();
	// (W Stream ma wersjê oryginaln¹, tu w SeekableStream dostaje lepsz¹. Mo¿na j¹ dalej nadpisaæ.)
	virtual size_t Skip(size_t MaxLength);

	// Zwraca rozmiar
	// (W oryginale: zg³asza b³¹d)
	virtual size_t GetSize();
	// Zwraca pozycjê wzglêdem pocz¹tku
	// (W oryginale: zg³asza b³¹d)
	virtual int GetPos();
	// Ustawia pozycjê wzglêdem pocz¹tku
	// pos musi byæ dodatnie.
	// (W oryginale: zg³asza b³¹d)
	virtual void SetPos(int pos);
	// Ustawia pozycjê wzglêdem bie¿¹cej
	// pos mo¿e byæ dodatnie albo ujemne.
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void SetPosFromCurrent(int pos);
	// Ustawia pozycjê wzglêdem koñcowej
	// Np. abu ustawiæ na ostatni znak, przesuñ do -1.
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void SetPosFromEnd(int pos);
	// Ustawia pozycjê na pocz¹tku
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void Rewind();
	// Ustawia rozmiar na podany
	// Po tym wywo³aniu pozycja kursora jest niezdefiniowana.
	// (W oryginale: zg³asza b³¹d)
	virtual void SetSize(size_t Size);
	// Obcina strumieñ od bie¿¹cej pozycji do koñca, czyli ustawia taki rozmiar jak bie¿¹ca pozycja.
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void Truncate();
	// Opró¿nia strumieñ
	// (Mo¿na j¹ nadpisaæ, ale ma wersjê oryginaln¹)
	virtual void Clear();
};

// Nak³adka przyspieszaj¹ca odczytuj¹ca ze strumienia pojedyncze znaki i inne rzeczy.
/*
- Nie jest strumieniem.
- Tylko odczyt.
- Jest szybka, poniewa¿:
  1. Nie ma metod wirtualnych.
  2. Buforuje, wiêc nie u¿ywa za ka¿dym razem metod wirtualnych strumienia.
- Szybkoœæ wzglêdem wczytywania pojedynczych znaków prosto ze strumienia: Debug = 10 razy, Release = 450 razy!
- Celowo nie u¿ywam w tej klasie ERR_TRY .. ERR_CATCH_FUNC, ¿eby by³o szybciej.
*/
class CharReader
{
private:
	Stream *m_Stream;
	std::vector<char> m_Buf;
	// Miejsce, do którego doczyta³em z bufora
	uint m_BufBeg;
	// Miejsce, do którego bufor jest wype³niony danymi
	uint m_BufEnd;

	// Wczytuje now¹ porcjê danych do strumienia
	// Wywo³ywaæ tylko kiedy bufor siê skoñczy³, tzn. m_BufBeg == m_BufEnd.
	// Jeœli skoñczy³ siê strumieñ i nic nie uda³o siê wczytaæ, zwraca false.
	// Jeœli nie, zapewnia ¿e w buforze bêdzie co najmniej jeden znak.
	bool EnsureNewChars();

public:
	CharReader(Stream *a_Stream) : m_Stream(a_Stream), m_BufBeg(0), m_BufEnd(0) { m_Buf.resize(BUFFER_SIZE); }

	// Czy jesteœmy na koñcu danych?
	bool End() {
		return (m_BufBeg == m_BufEnd) && m_Stream->End(); // Nic nie zosta³o w buforze i nic nie zosta³o w strumieniu.
	}
	// Jeœli mo¿na odczytaæ nastêpny znak, wczytuje go i zwraca true.
	// Jeœli nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool ReadChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg++]; return true; }
	// Jeœli mo¿na odczytaæ nastêpny znak, wczytuje go.
	// Jeœli nie, rzuca wyj¹tek.
	char MustReadChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg++]; }
	// Jeœli mo¿na odczytaæ nastêpny znak, podgl¹da go zwracaj¹c przez Out i zwraca true. Nie przesuwa "kursora".
	// Jeœli nie, nie zmienia Out i zwraca false. Oznacza to koniec strumienia.
	bool PeekChar(char *Out) { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } *Out = m_Buf[m_BufBeg]; return true; }
	// Jeœli mo¿na odczytaæ nastêpny znak, podgl¹da go zwracaj¹c. Nie przesuwa "kursora".
	// Jeœli nie, rzuca wyj¹tek.
	char MustPeekChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } return m_Buf[m_BufBeg]; }
	// Wczytuje co najwy¿ej MaxLength znaków do podanego stringa.
	// StringStream jest czyszczony - nie musi byæ pusty ani zaalokowany.
	// Zwraca liczbê odczytanych znaków. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t ReadString(string *Out, size_t MaxLength);
	// Wczytuje Length znaków do podanego stringa.
	// StringStream jest czyszczony - nie musi byæ pusty ani zaalokowany.
	// Jeœli nie uda siê odczytaæ wszystkich Length znaków, rzuca wyj¹tek.
	void MustReadString(string *Out, size_t Length);
	// Wczytuje co najwy¿ej MaxLength znaków pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej MaxLength.
	// Zwraca liczbê odczytanych znaków. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t ReadString(char *Out, size_t MaxLength);
	// Wczytuje Length znaków pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej Length.
	// Jeœli nie uda siê odczytaæ wszystkich Length znaków, rzuca wyj¹tek.
	void MustReadString(char *Out, size_t Length);
	// Wczytuje co najwy¿ej MaxLength bajtów pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej MaxLength bajtów.
	// Zwraca liczbê odczytanych bajtów. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t ReadData(void *Out, size_t MaxLength);
	// Wczytuje Length bajtów pod podany wskaŸnik.
	// Out musi byæ tablic¹ zaalokowan¹ do d³ugoœci co najmniej Length bajtów.
	// Jeœli nie uda siê odczytaæ wszystkich Length bajtów, rzuca wyj¹tek.
	void MustReadData(void *Out, size_t Length);
	// Jeœli jest nastêpny znak do odczytania, pomija go i zwraca true.
	// Jeœli nie, zwraca false. Oznacza to koniec strumienia.
	bool SkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) return false; } m_BufBeg++; return true; }
	// Jeœli jest nastêpny znak do odczytania, pomija go.
	// Jeœli nie, rzuca wyj¹tek.
	void MustSkipChar() { if (m_BufBeg == m_BufEnd) { if (!EnsureNewChars()) _ThrowBufEndError(__FILE__, __LINE__); } m_BufBeg++; }
	// Pomija co najwy¿ej MaxLength znaków.
	// Zwraca liczbê pominiêtych znaków. Mniej ni¿ ¿¹dano oznacza koniec strumienia.
	size_t Skip(size_t MaxLength);
	// Pomija Length znaków.
	// Jeœli nie uda siê pomin¹æ wszystkich Length znaków, rzuca wyj¹tek.
	void MustSkip(size_t Length);
	// Wczytuje linijkê tekstu i jeœli siê uda³o, zwraca true.
	// Jeœli nie uda³o siê wczytaæ ani jednego znaku, bo to ju¿ koniec strumienia, zwraca false.
	// Czyta znaki do napotkania koñca strumienia lub koñca wiersza, którym s¹ "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do³¹cza go do Out.
	bool ReadLine(string *Out);
	// Wczytuje linijkê tekstu.
	// Jeœli nie uda³o siê wczytaæ ani jednego znaku, bo to ju¿ koniec strumienia, rzuca wyj¹tek.
	// Czyta znaki do napotkania koñca strumienia lub koñca wiersza, którym s¹ "\r", "\n" i "\r\n".
	// Koniec wiersza przeskakuje, ale nie do³¹cza go do Out.
	void MustReadLine(string *Out);
};

// Strumieñ pamiêci statycznej
// Strumieñ ma sta³¹ d³ugoœæ i nie mo¿e byæ rozszerzany - ani automatycznie, ani rêcznie.
class MemoryStream : public SeekableStream
{
private:
	char *m_Data;
	size_t m_Size;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wskaŸnik 0, aby strumieñ sam zaalkowa³, a w destruktorze zwolni³ pamiêæ.
	// Podaj wskaŸnik, jeœli ma korzystaæ z twojego obszaru pamiêci.
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

// Strumieñ oparty na ³añcuchu std::string
// Potrafi siê rozszerzaæ.
class StringStream : public SeekableStream
{
private:
	std::string *m_Data;
	int m_Pos;
	bool m_InternalAlloc;

public:
	// Podaj wskaŸnik 0, aby strumieñ sam zaalokowa³, a w destruktorze zwolni³ ³añcuch.
	// Podaj wskaŸnik, jeœli ma korzystaæ z twojego obszaru pamiêci.
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
