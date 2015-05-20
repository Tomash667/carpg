/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Tokenizer - Parser sk³adni jêzyka podobnego do C/C++
 * Dokumentacja: Patrz plik doc/Tokenizer.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

#include "Error.hpp"

namespace common
{

class Stream;

// Klasa b³êdu dla tokenizera
class TokenizerError : public Error
{
private:
	size_t m_Char, m_Row, m_Col;
public:
	TokenizerError(size_t Char, size_t Row, size_t Col, const string &Msg = "", const string &File = "", int a_Line = 0);

	size_t GetCharNum() { return m_Char; }
	size_t GetRowNum() { return m_Row; }
	size_t GetColNum() { return m_Col; }
};

class Tokenizer_pimpl;

class Tokenizer
{
private:
	scoped_ptr<Tokenizer_pimpl> pimpl;

public:
	// Koniec wiersza '\n' ma byæ uznawany za osobnego rodzaju token, nie za zwyczajny bia³y znak
	static const uint FLAG_TOKEN_EOL         = 0x01;
	// £añcuch "" mo¿e siê rozci¹gaæ na wiele wierszy zawieraj¹c w treœci koniec wiersza
	static const uint FLAG_MULTILINE_STRINGS = 0x02;

	// Rodzaje tokenów
	enum TOKEN
	{
		TOKEN_EOF,        // Koniec ca³ego dokumentu (dalej nie parsowaæ)
		TOKEN_EOL,        // Token koñca linii (tylko kiedy FLAG_TOKEN_EOL)
		TOKEN_SYMBOL,     // Jednoznakowy symbol (np. kropka)
		TOKEN_INTEGER,    // Liczba ca³kowita, np. -123
		TOKEN_FLOAT,      // Liczba zmiennoprzecinkowa, np. 10.5
		TOKEN_CHAR,       // Sta³a znakowa, np. 'A'
		TOKEN_IDENTIFIER, // Identyfikator, np. abc_123
		TOKEN_KEYWORD,    // S³owo kluczowe, czyli identyfikator pasuj¹cy do zarejestrowanych s³ów kluczowych
		TOKEN_STRING,     // Sta³a ³añcuchowa, np. "ABC"
	};

	// Tworzy z dokumentu podanego przez ³añcuch char*
	Tokenizer(const char *Input, size_t InputLength, uint Flags);
	// Tworzy z dokumentu podanego przez string
	Tokenizer(const string *Input, uint Flags);
	// Tworzy z dokumentu wczytywanego z dowolnego strumienia
	Tokenizer(Stream *Input, uint Flags);

	~Tokenizer();

	// ==== Konfiguracja ====

	void RegisterKeyword(uint Id, const string &Keyword);
	// Wczytuje ca³¹ listê s³ów kluczowych w postaci tablicy ³añcuchów char*.
	// S³owa kluczowe bêd¹ mia³y kolejne identyfikatory 0, 1, 2 itd.
	void RegisterKeywords(const char **Keywords, size_t KeywordCount);

	// ==== Zwraca informacje na temat ostatnio odczytanego tokena ====

	// Odczytuje nastêpny token
	void Next();

	// Zwraca typ
	TOKEN GetToken();

	// Zwraca pozycjê
	size_t GetCharNum();
	size_t GetRowNum();
	size_t GetColNum();

	// Dzia³a zawsze, ale zastosowanie g³ównie dla GetToken() == TOKEN_IDENTIFIER lub TOKEN_STRING
	const string & GetString();
	// Tylko jeœli GetToken() == TOKEN_CHAR lub TOKEN_SYMBOL
	char GetChar();
	// Tylko jeœli GetToken() == TOKEN_KEYWORD
	uint GetId();
	// Tylko jeœli GetToken() == TOKEN_INTEGER
	bool GetUint1(uint1 *Out);
	bool GetUint2(uint2 *Out);
	bool GetUint4(uint4 *Out);
	bool GetUint8(uint8 *Out);
	bool GetInt1(int1 *Out);
	bool GetInt2(int2 *Out);
	bool GetInt4(int4 *Out);
	bool GetInt8(int8 *Out);
	uint1 MustGetUint1();
	uint2 MustGetUint2();
	uint4 MustGetUint4();
	uint8 MustGetUint8();
	int1 MustGetInt1();
	int2 MustGetInt2();
	int4 MustGetInt4();
	int8 MustGetInt8();
	// Tylko jeœli GetToken() == TOKEN_INTEGER lub TOKEN_FLOAT
	bool GetFloat(float *Out);
	bool GetDouble(double *Out);
	float MustGetFloat();
	double MustGetDouble();

	// ==== Funkcje pomocnicze ====

	// Rzuca wyj¹tek z b³êdem parsowania. Domyœlny komunikat.
	void CreateError();
	// Rzuca wyj¹tek z b³êdem parsowania. W³asny komunikat.
	void CreateError(const string &Msg);
	// Zwraca true, jeœli ostatnio sparsowany token jest taki jak tu podany.
	bool QueryToken(TOKEN Token);
	bool QueryToken(TOKEN Token1, TOKEN Token2);
	bool QueryEOF();
	bool QueryEOL();
	bool QuerySymbol(char Symbol);
	bool QueryIdentifier(const string &Identifier);
	bool QueryKeyword(uint KeywordId);
	bool QueryKeyword(const string &Keyword);
	// Rzuca wyj¹tek z b³êdem parsowania, jeœli ostatnio sparsowany token nie jest podanego rodzaju.
	void AssertToken(TOKEN Token);
	// Rzuca wyj¹tek z b³êdem parsowania, jeœli ostatnio sparsowany token nie jest jednego z dwóch podanych rodzajów.
	void AssertToken(TOKEN Token1, TOKEN Token2);
	// Rzuca wyj¹tek z b³êdem parsowania, jeœli ostatnio sparsowany token nie jest taki jak tu podany.
	void AssertEOF();
	void AssertEOL();
	void AssertSymbol(char Symbol);
	void AssertIdentifier(const string &Identifier);
	void AssertKeyword(uint KeywordId);
	void AssertKeyword(const string &Keyword);

	// Zamienia typ tokena na jego czytelny dla u¿ytkownika opis tekstowy, np. "Sta³a znakowa"
	static const char * GetTokenName(TOKEN Token);

	// Escapuje ³añcuch zgodnie ze sk³adni¹ tokenizera
	static const uint ESCAPE_EOL   = 0x01; // Zamieniaæ znaki koñca wiersza na \r \n na sekwencje ucieczki
	static const uint ESCAPE_OTHER = 0x02; // Zamieniaæ znaki niedostêne z klawiatury (w tym polskie litery) na \xHH
	static void Escape(string *Out, const string &In, uint EscapeFlags);
};

} // namespace common
