/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Base - Modu³ podstawowy
 * Dokumentacja: Patrz plik doc/Base.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include <cstdio> // dla sprintf
#include <cctype> // dla tolower, isalnum itp.
#include <ctime> // dla time potrzebnego w RandomGenerator
#include <memory.h>
#include <windows.h>
#include <float.h> // dla _finite i _isnan

namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Ogólne

uint4 log2u(uint4 x)
{
	if (x & 0xFFFF0000) {
		if (x & 0xFF000000) {
			if (x & 0xF0000000) {
				if (x & 0xC0000000) { if (x & 0x80000000) return 31; else return 30; }
				else                { if (x & 0x20000000) return 29; else return 28; }
			}
			else {
				if (x & 0x0C000000) { if (x & 0x08000000) return 27; else return 26; }
				else                { if (x & 0x02000000) return 25; else return 24; }
			}
		}
		else {
			if (x & 0x00F00000) {
				if (x & 0x00C00000) { if (x & 0x00800000) return 23; else return 22; }
				else                { if (x & 0x00200000) return 21; else return 20; }
			}
			else {
				if (x & 0x000C0000) { if (x & 0x00080000) return 19; else return 18; }
				else                { if (x & 0x00020000) return 17; else return 16; }
			}
		}
	}
	else {
		if (x & 0x0000FF00) {
			if (x & 0x0000F000) {
				if (x & 0x0000C000) { if (x & 0x00008000) return 15; else return 14; }
				else                { if (x & 0x00002000) return 13; else return 12; }
			}
			else {
				if (x & 0x00000C00) { if (x & 0x00000800) return 11; else return 10; }
				else                { if (x & 0x00000200) return  9; else return  8; }
			}
		}
		else {
			if (x & 0x000000F0) {
				if (x & 0x000000C0) { if (x & 0x00000080) return  7; else return  6; }
				else                { if (x & 0x00000020) return  5; else return  4; }
			}
			else {
				if (x & 0x0000000C) { if (x & 0x00000008) return  3; else return  2; }
				else                { if (x & 0x00000002) return  1; else return  0; }
			}
		}
	}
}

uint4 GetBitMask(uint4 n)
{
	if (n < 16) {
		if (n < 8) {
			if (n < 4) {
				if (n <  2) { if (n ==  1) return 0x00000001; else return 0x00000000; }
				else        { if (n ==  3) return 0x00000007; else return 0x00000003; }
			}
			else {
				if (n <  6) { if (n ==  5) return 0x0000001F; else return 0x0000000F; }
				else        { if (n ==  7) return 0x0000007F; else return 0x0000003F; }
			}
		}
		else {
			if (n < 12) {
				if (n < 10) { if (n ==  9) return 0x000001FF; else return 0x000000FF; }
				else        { if (n == 11) return 0x000007FF; else return 0x000003FF; }
			}
			else {
				if (n < 14) { if (n == 13) return 0x00001FFF; else return 0x00000FFF; }
				else        { if (n == 15) return 0x00007FFF; else return 0x00003FFF; }
			}
		}
	}
	else if (n < 32) {
		if (n < 24) {
			if (n < 20) {
				if (n < 18) { if (n == 17) return 0x0001FFFF; else return 0x0000FFFF; }
				else        { if (n == 19) return 0x0007FFFF; else return 0x0003FFFF; }
			}
			else {
				if (n < 22) { if (n == 21) return 0x001FFFFF; else return 0x000FFFFF; }
				else        { if (n == 23) return 0x007FFFFF; else return 0x003FFFFF; }
			}
		}
		else {
			if (n < 28) {
				if (n < 26) { if (n == 25) return 0x01FFFFFF; else return 0x00FFFFFF; }
				else        { if (n == 27) return 0x07FFFFFF; else return 0x03FFFFFF; }
			}
			else {
				if (n < 30) { if (n == 29) return 0x1FFFFFFF; else return 0x0FFFFFFF; }
				else        { if (n == 31) return 0x7FFFFFFF; else return 0x3FFFFFFF; }
			}
		}
	}
	else return 0xFFFFFFFF;
}

void ZeroMem(void *Data, size_t NumBytes)
{
	ZeroMemory(Data, NumBytes);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// £añcuchy

const char * const EOL = "\r\n";

// Liczba obslugiwanych stron kodowych
const int CHARSET_COUNT = 5;
// Kody znakow poszczegolnych stron kodowych
const string CHARSET_CHARS[CHARSET_COUNT][18] = {
	{ "\x61", "\x63", "\x65", "\x6C", "\x6E", "\x6F", "\x73", "\x7A", "\x7A",    // brak, male
	  "\x41", "\x43", "\x45", "\x4C", "\x4E", "\x4F", "\x53", "\x5A", "\x5A", }, // brak, duze
	{ "\xB9", "\xE6", "\xEA", "\xB3", "\xF1", "\xF3", "\x9C", "\x9F", "\xBF",    // Windows, male
	  "\xA5", "\xC6", "\xCA", "\xA3", "\xD1", "\xD3", "\x8C", "\x8F", "\xAF", }, // Windows, duze
	{ "\xB1", "\xE6", "\xEA", "\xB3", "\xF1", "\xF3", "\xB6", "\xBC", "\xBF",    // ISO, male
	  "\xA1", "\xC6", "\xCA", "\xA3", "\xD1", "\xD3", "\xA6", "\xAC", "\xAF", }, // ISO, duze
	{ "\xA5", "\x86", "\xA9", "\x88", "\xE4", "\xA2", "\x98", "\xAB", "\xBE",    // IBM, male
	  "\xA4", "\x8F", "\xA8", "\x9D", "\xE3", "\xE0", "\x97", "\x8D", "\xBD", }, // IBM, duze
	{ "\xC4\x85", "\xC4\x87", "\xC4\x99", "\xC5\x82", "\xC5\x84", "\xC3\xB3", "\xC5\x9B", "\xC5\xBA", "\xC5\xBC",    // UTF-8, male
	  "\xC4\x84", "\xC4\x86", "\xC4\x98", "\xC5\x81", "\xC5\x83", "\xC3\x93", "\xC5\x9A", "\xC5\xB9", "\xC5\xBB", }, // UTF-8, duze
};

bool CharIsAlphaNumeric(char ch)
{
	return (IsCharAlphaNumeric(ch) != 0);
}

bool CharIsAlphaNumeric_f(char ch)
{
	return
		(ch >= '0' && ch <= '9') ||
		(ch >= 'A' && ch <= 'Z') ||
		(ch >= 'a' && ch <= 'z') ||
		(ch == '¹') || (ch == 'æ') || (ch == 'ê') || (ch == '³') || (ch == 'ñ') || (ch == 'ó') || (ch == 'œ') || (ch == '¿') || (ch == 'Ÿ') ||
		(ch == '¥') || (ch == 'Æ') || (ch == 'Ê') || (ch == '£') || (ch == 'Ñ') || (ch == 'Ó') || (ch == 'Œ') || (ch == '¯') || (ch == '');
}

bool CharIsAlpha(char ch)
{
	return (IsCharAlpha(ch) != 0);
}

bool CharIsLower(char ch)
{
	return (IsCharLower(ch) != 0);
}

bool CharIsLower(char ch, CHARSET Charset)
{
	if (CharIsLower(ch))
		return true;
	for (int i = 0; i < 9; i++)
		if (CHARSET_CHARS[Charset][i][0] == ch)
			return true;
	return false;
}

bool CharIsUpper(char ch)
{
	return (IsCharUpper(ch) != 0);
}

bool CharIsUpper(char ch, CHARSET Charset)
{
	if (CharIsUpper(ch))
		return true;
	for (int i = 9; i < 18; i++)
		if (CHARSET_CHARS[Charset][i][0] == ch)
			return true;
	return false;
}

void ReverseString(string *s)
{
	size_t i = 0;
	size_t j = s->length()-1;
	while (i < j)
		std::swap( (*s)[i++], (*s)[j--] );
}

void Trim(string *s)
{
	if (s->empty()) return;
	// pocz¹tek
	size_t i = 0;
	while (CharIsWhitespace((*s)[i]))
		if (++i == s->size())
		{
			s->clear();
			return;
		}
	s->erase(0, i);
	// koniec
	i = s->size()-1;
	while (CharIsWhitespace((*s)[i]))
		i--;
	s->erase(i+1);
}

void Trim(string *Out, const string &s)
{
	if (s.empty())
	{
		Out->clear();
		return;
	}
	// pocz¹tek
	size_t start = 0;
	while (CharIsWhitespace(s[start]))
		if (++start == s.size())
		{
			Out->clear();
			return;
		}
	// koniec
	size_t count = s.size()-start;
	while (CharIsWhitespace(s[start+count-1]))
		count--;
	// zwrócenie
	*Out = s.substr(start, count);
}

/* UWAGA!
Funkcje CharUpper() i CharLower() z Win32API powoduja blad ochorony, kiedy
uruchamia sie je w trybie konwersji pojedynczego znaku, a tym znakiem jest
polska litera!
W sumie to chyba daloby sie napisac lepiej, ten blad wynika przeciez z tego,
¿e podaje liczbe ze znakiem (char jest signed, polskie litery sa na minusie).
Gdyby to przekonwertowac... No ale to wymaga dosc zaawansowanego rzutowania,
wiec niech tak sobie juz zostanie :PPP
*/

char CharToLower(char ch)
{
	char s[2];
	s[0] = ch;
	s[1] = '\0';
	CharLowerA(s);
	return s[0];
}

char CharToLower(char ch, CHARSET Charset)
{
	char r = CharToLower(ch);
	if (r != ch)
		return r;
	for (int i = 9; i < 18; i++)
		if (CHARSET_CHARS[Charset][i][0] == ch)
			return CHARSET_CHARS[Charset][i-9][0];
	return r;
}

char CharToUpper(char ch)
{
	char s[2];
	s[0] = ch;
	s[1] = '\0';
	CharUpperA(s);
	return s[0];
}

char CharToUpper(char ch, CHARSET Charset)
{
	char r = CharToUpper(ch);
	if (r != ch)
		return r;
	for (int i = 0; i < 9; i++)
		if (CHARSET_CHARS[Charset][i][0] == ch)
			return CHARSET_CHARS[Charset][i+9][0];
	return r;
}

void LowerCase(string *s)
{
	for (size_t i = 0; i < s->size(); i++)
		(*s)[i] = CharToLower((*s)[i]);
}

void UpperCase(string *s)
{
	for (size_t i = 0; i < s->size(); i++)
		(*s)[i] = CharToUpper((*s)[i]);
}

void LowerCase(string *s, CHARSET Charset)
{
	for (size_t i = 0; i < s->size(); i++)
		(*s)[i] = CharToLower((*s)[i], Charset);
}

void UpperCase(string *s, CHARSET Charset)
{
	for (size_t i = 0; i < s->size(); i++)
		(*s)[i] = CharToUpper((*s)[i], Charset);
}

void EolModeToStr(string *Out, EOLMODE EolMode)
{
	switch (EolMode)
	{
	case EOL_CRLF:
		*Out = "\r\n";
		return;
	case EOL_LF:
		*Out = "\n";
		return;
	case EOL_CR:
		*Out = "\r";
		return;
	default:
		Out->clear();
		return;
	}
}

void Replace(string *result, const string &s, const string &s1, const string &s2)
{
	if (s1.empty())
	{
		*result = s;
		return;
	}
	result->clear();
	size_t index1 = 0;
	size_t index2 = 0;
	while ( (index2 = s.find(s1, index1)) != string::npos )
	{
		*result += s.substr(index1, index2-index1);
		*result += s2;
		index1 = index2 + s1.size();
	}
	*result += s.substr(index1);
}

void Replace(string *Out, const string &s, char Ch1, char Ch2)
{
	size_t Length = s.length();
	Out->resize(Length);
	char Ch;
	for (size_t i = 0; i < Length; i++)
	{
		Ch = s[i];
		if (Ch == Ch1)
			(*Out)[i] = Ch2;
		else
			(*Out)[i] = Ch;
	}
}

void Replace(string *InOut, char Ch1, char Ch2)
{
	size_t Length = InOut->length();
	for (size_t i = 0; i < Length; i++)
	{
		if ((*InOut)[i] == Ch1)
			(*InOut)[i] = Ch2;
	}
}

void ReplaceEOL(string *result, const string &s, EOLMODE EOLMode)
{
	if (EOLMode == EOL_NONE)
	{
		*result = s;
		return;
	}

	result->clear();
	string Tmp;
	bool ItWasCR = false;
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] == '\r')
			ItWasCR = true;
		else if (s[i] == '\n')
		{
			EolModeToStr(&Tmp, EOLMode);
			*result += Tmp;
			ItWasCR = false;
		}
		else
		{
			if (ItWasCR)
			{
				EolModeToStr(&Tmp, EOLMode);
				*result += Tmp;
				ItWasCR = false;
			}
			*result += s[i];
		}
	}
	if (ItWasCR)
	{
		EolModeToStr(&Tmp, EOLMode);
		*result += Tmp;
	}
}

void NormalizeWhitespace(string *result, const string &s)
{
	bool inside = false;
	for (size_t i = 0; i < s.size(); i++)
	{
		if (CharIsWhitespace(s[i]))
			inside = true;
		else
		{
			if (inside)
			{
				*result += ' ';
				inside = false;
			}
			*result += s[i];
		}
	}
	if (inside)
		*result += ' ';
}

void DupeString(string *Out, const string &s, size_t count)
{
	Out->clear();
	for (size_t i = 0; i < count; i++)
		*Out += s;
}

void RightStr(string *Out, const string &s, size_t Length)
{
	Length = std::min(Length, s.length());
	*Out = s.substr(s.length() - Length);
}

size_t SubstringCount(const string &str, const string &substr)
{
	if (substr.empty())
		return 0;
	size_t result = 0;
	size_t index = 0;
	size_t i;
	for (;;)
	{
		i = str.find(substr, index);
		if (i == string::npos)
			break;
		++result;
		index = i + substr.size();
	}
	return result;
}

bool StrEqualI(const string &s1, const string &s2)
{
	size_t Size1 = s1.size();
	if (Size1 != s2.size())
		return false;
	for (size_t i = 0; i < Size1; ++i)
	{
		if (CharToLower(s1[i]) != CharToLower(s2[i]))
			return false;
	}
	return true;
}

bool SubStrEqual(const string &s1, size_t off1, const string &s2, size_t off2, size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		if (s1[off1] != s2[off2])
			return false;
		off1++;
		off2++;
	}

	return true;
}

bool SubStrEqualI(const string &s1, size_t off1, const string &s2, size_t off2, size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		if (CharToLower(s1[off1]) != CharToLower(s2[off2]))
			return false;
		off1++;
		off2++;
	}

	return true;
}

bool ContainsEOL(const string &s)
{
	for (size_t i = 0; i < s.size(); ++i)
		if (s[i] == '\r' || s[i] == '\n')
			return true;
	return false;
}

bool StrBegins(const string &s, const string &sub, size_t Begin)
{
	if (Begin+sub.size() > s.size())
		return false;
	for (size_t i = 0; i < sub.size(); i++)
		if (s[Begin+i] != sub[i])
			return false;
	return true;
}

bool StrEnds(const string &s, const string &Sub)
{
	if (Sub.length() > s.length())
		return false;
	size_t si = s.length() - Sub.length();
	size_t subi = 0;
	for ( ; si < s.length(); si++, subi++)
	{
		if (s[si] != Sub[subi])
			return false;
	}
	return true;
}

bool Split(const string &s, const string &delimiter, string *out, size_t *index)
{
	if (delimiter.empty())
		return false;
	if (*index >= s.size())
		return false;

	size_t pos = s.find(delimiter, *index);
	if (pos == string::npos)
	{
		*out = s.substr(*index);
		*index = s.size();
		return true;
	}

	*out = s.substr(*index, pos-*index);
	*index = pos + delimiter.size();
	return true;
}

bool SplitFirstOf(const string &s, const string &delimiters, string *out, size_t *index)
{
	if (delimiters.empty())
		return false;
	if (*index >= s.size())
		return false;

	size_t pos = s.find_first_of(delimiters, *index);
	if (pos == string::npos)
	{
		*out = s.substr(*index);
		*index = s.size();
		return true;
	}

	*out = s.substr(*index, pos-*index);
	*index = pos + 1;
	return true;
}

void Split(STRING_VECTOR *Out, const string &s, const string &Delimiter)
{
	size_t Index = 0;
	string Tmp;
	Out->clear();
	while (Split(s, Delimiter, &Tmp, &Index))
		Out->push_back(Tmp);
}

bool SplitEOL(const string &s, string *out, size_t *index)
{
	if (*index >= s.size())
		return false;

	size_t posr = s.find('\r', *index);
	size_t posn = s.find('\n', *index);
	if (posr == string::npos && posn == string::npos)
	{
		*out = s.substr(*index);
		*index = s.size();
		return true;
	}

	if (posn == string::npos || posr < posn)
	{
		*out = s.substr(*index, posr-*index);
		if (*index < s.size()-1 && s[posr+1] == '\n')
			*index = posr + 2;
		else
			*index = posr + 1;
		return true;
	}

	*out = s.substr(*index, posn-*index);
	*index = posn + 1;
	return true;
}

bool SplitQuery(const string &s, string *out, size_t *index)
{
	if (*index >= s.size())
		return false;

	size_t i;
	size_t l = s.size();

	while (CharIsWhitespace(s[*index]))
		if (++(*index) >= l)
			return false;

    if (s[*index] == '"')
	{
		(*index)++;
		i = s.find('"', *index);
	}
	else
	{
		i = string::npos;
		for (size_t j = *index; j < s.size(); j++)
			if (CharIsWhitespace(s[j]))
			{
				i = j;
				break;
			}
	}

	if (i == string::npos)
	{
		*out = s.substr(*index);
		*index = l+1;
	}
	else
	{
		*out = s.substr(*index, i-*index);
		*index = i+1;
	}
	return true;
}

bool ValidateWildcard_Asterisk(const string &Mask, const string &S, size_t *Mask_i, size_t *S_i, bool CaseSensitive)
{
	// Warning: uses multiple returns.
	bool Fit = true;

	// Erase the leading asterisk
	(*Mask_i)++;
	while (*S_i < S.length() && (Mask[*Mask_i] == '?' || Mask[*Mask_i] == '*'))
	{
		if (Mask[*Mask_i] == '?')
			(*S_i)++;
		(*Mask_i)++;
	}
	// Now it could be that test is empty and wildcard contains aterisks. Then we delete them to get a proper state
	while (Mask[*Mask_i] == '*')
		(*Mask_i)++;
	if (*S_i == S.length() && *Mask_i < Mask.length())
		return !Fit;
	if (*S_i == S.length() && *Mask_i < Mask.length())
		return Fit;
	// Neither test nor wildcard are empty! the first character of wildcard isn't in [*?]
	if (!ValidateWildcard(Mask, S, CaseSensitive, *Mask_i, *S_i))
	{
		do
		{
			(*S_i)++;
			// skip as much characters as possible in the teststring; stop if a character match occurs
			if (CaseSensitive)
			{
				while (*S_i < S.length() && Mask[*Mask_i] != S[*S_i])
					(*S_i)++;
			}
			else
			{
				while (*S_i < S.length() && CharToLower(Mask[*Mask_i]) != CharToLower(S[*S_i]))
					(*S_i)++;
			}
		}
		while ( (*S_i < S.length()) ? !ValidateWildcard(Mask, S, CaseSensitive, *Mask_i, *S_i) : ((Fit = 0) == true) );
	}
	if (*S_i == S.length() && *Mask_i == Mask.length())
		Fit = true;
	return Fit;
}

bool ValidateWildcard(const string &Mask, const string &S, bool CaseSensitive, size_t MaskOff, size_t SOff)
{
	// Algoryt na podstawie:
	// Wildcards Pattern Matching Algorithm, Florian Schintke
	// http://user.cs.tu-berlin.de/~schintke/references/wildcards/

	bool Fit = true;

	for ( ; MaskOff < Mask.length() && Fit && SOff < S.length(); MaskOff++)
	{
		switch (Mask[MaskOff])
		{
		case '?':
			SOff++;
			break;
		case '*':
			Fit = ValidateWildcard_Asterisk(Mask, S, &MaskOff, &SOff, CaseSensitive);
			MaskOff--;
			break;
		default:
			if (CaseSensitive)
				Fit = (S[SOff] == Mask[MaskOff]);
			else
				Fit = (CharToLower(S[SOff]) == CharToLower(Mask[MaskOff]));
			SOff++;
		}
	}

	while (Mask[MaskOff] == '*' && Fit)
		MaskOff++;

	return (Fit && SOff == S.length() && MaskOff == Mask.length());
}

// Funkcja do u¿ytku wewnêtrznego dla FineSearch
// Zwraca liczbê zale¿n¹ od okolicznoœci, w jakich wystêpuje znaleziony string: 1.0 .. 4.0
// Mno¿ona jest przez ni¹ obliczana trafnoœæ.
float _MnoznikOkolicznosci(const string &str, size_t start, size_t length)
{
	bool l = (start == 0);
	bool r = (start+length == str.size());
	// Ca³e pole
	if (l && r)
		return 4.f;
	// Pocz¹tek pola
	else if (l)
		return 3.f;
	else
	{
//			if (!l)
			if (CharIsAlphaNumeric(str[start-1]))
				l = true;
		if (!r)
			if (CharIsAlphaNumeric(str[start+length]))
				r = true;
		// Ca³y ci¹g
		if (l && r)
			return 2.f;
		// Nic ciekawego
		else
			return 1.f;
	}
}

float FineSearch(const string &SubStr, const string &Str)
{
	string usubstr; LowerCase(&usubstr, SubStr);
	string ustr; LowerCase(&ustr, Str);
	float result = 0.f;

	size_t i = 0;
	size_t p; float x;

	while (true)
	{
		p = ustr.find(usubstr, i);
		// Koniec szukania
		if (p == string::npos)
			break;
		x = std::max( usubstr.size()/5.f - Str.size()/100.f + 1.f, 1.f);
		// Jeœli zgadza siê wielkoœæ liter
		if (SubStrEqual(Str, p, SubStr, 0, SubStr.size()))
			x *= 1.5;
		// Mno¿nik okolicznoœci - jeœli to ca³e pole, pocz¹tek pola lub ca³y ci¹g
		x *= _MnoznikOkolicznosci(Str, p+Str.size()-Str.size(), SubStr.size());
		// Dodajemy wyliczon¹ trafnoœæ
		result += x;
		i = p + SubStr.length();
	}

	return result;
}

size_t LevenshteinDistance(const string &s1, const string &s2)
{
	size_t L1 = s1.length();
	size_t L2 = s2.length();
	size_t i, j, cost;
	char c1, c2;

	if (L1 == 0) return L2;
	if (L2 == 0) return L1;

	size_t *D_PrevRow = new size_t[L2+1];
	size_t *D_CurrRow = new size_t[L2+1];

	for (j = 0; j <= L2; j++)
		D_PrevRow[j] = j;

	for (i = 1; i <= L1; i++)
	{
		D_CurrRow[0] = i;
		c1 = s1[i-1];
		for (j = 1; j <= L2; j++)
		{
			c2 = s2[j-1];
			cost = (c1 == c2 ? 0 : 1);
			D_CurrRow[j] = min3(D_PrevRow[j] + 1, D_CurrRow[j-1] + 1, D_PrevRow[j-1] + cost);
		}
		std::swap(D_PrevRow, D_CurrRow);
	}

	size_t R = D_PrevRow[L2]; // Prev, bo zosta³o na koniec ostatniego przebiegu pêtli zamienione z Curr
	delete [] D_CurrRow;
	delete [] D_PrevRow;
	return R;
}

size_t LevenshteinDistanceI(const string &s1, const string &s2)
{
	size_t L1 = s1.length();
	size_t L2 = s2.length();
	size_t i, j, cost;
	char c1, c2;

	if (L1 == 0) return L2;
	if (L2 == 0) return L1;

	size_t *D_PrevRow = new size_t[L2+1];
	size_t *D_CurrRow = new size_t[L2+1];

	for (j = 0; j <= L2; j++)
		D_PrevRow[j] = j;

	for (i = 1; i <= L1; i++)
	{
		D_CurrRow[0] = i;
		c1 = CharToLower(s1[i-1]);
		for (j = 1; j <= L2; j++)
		{
			c2 = CharToLower(s2[j-1]);
			cost = (c1 == c2 ? 0 : 1);
			D_CurrRow[j] = min3(D_PrevRow[j] + 1, D_CurrRow[j-1] + 1, D_PrevRow[j-1] + cost);
		}
		std::swap(D_PrevRow, D_CurrRow);
	}

	size_t R = D_PrevRow[L2]; // Prev, bo zosta³o na koniec ostatniego przebiegu pêtli zamienione z Curr
	delete [] D_CurrRow;
	delete [] D_PrevRow;
	return R;
}

/*
Sortowanie naturalne
Napisane od pocz¹tku, ale na podstawie kodu:
  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>
  http://sourcefrog.net/projects/natsort/
Mój algorytm dzia³a jednak trochê inaczej i daje inne wyniki (przyda³oby siê
kiedyœ go poprawiæ).
*/

int StringNaturalCompare::CompareNumbers(const string &s1, size_t i1, const string &s2, size_t i2)
{
	// pomiñ pocz¹tkowe zera
	while (i1 < s1.length() && s1[i1] == '0')
		i1++;
	while (i2 < s2.length() && s2[i2] == '0')
		i2++;

	// The longest run of digits wins.  That aside, the greatest
	// value wins, but we can't know that it will until we've scanned
	// both numbers to know that they have the same magnitude, so we
	// remember it in BIAS.
	int bias = 0;

	for ( ; ; i1++, i2++)
	{
		// koniec lancucha
		if (i1 == s1.length() || !CharIsDigit(s1[i1]))
		{
			if (i2 == s2.length() || !CharIsDigit(s2[i2]))
				return bias;
			else
				return -1;
		}
		else if (i2 == s2.length() || !CharIsDigit(s2[i2]))
			return +1;

		// porównanie znaków
		if (s1[i1] < s2[i2])
		{
			if (bias == 0)
				bias = -1;
		}
		else if (s1[i1] > s2[i2])
		{
			if (bias == 0)
				bias = +1;
		}
	}
	return 0;
}

int StringNaturalCompare::Compare(const string &s1, const string &s2)
{
	size_t i1 = 0, i2 = 0;
	char c1, c2;
	int result;

	for (;;)
	{
		// koniec ³añcucha
		if (i1 == s1.length())
		{
			if (i2 == s2.length())
				return 0;
			else
				return -1;
		}
		else if (i2 == s2.length())
			return +1;

		// pobierz znaki
		c1 = s1[i1];
		c2 = s2[i2];

		// pomiñ bia³e znaki
		if (CharIsWhitespace(c1))
		{
			i1++;
			continue;
		}
		if (CharIsWhitespace(c2))
		{
			i2++;
			continue;
		}

		// obydwa to cyfry - porównujemy liczby
		if (CharIsDigit(c1) && CharIsDigit(c2))
		{
			if ((result = CompareNumbers(s1, i1, s2, i2)) != 0)
				return result;
		}

		// nieuwzglêdnianie wielkoœci liter
		if (m_CaseInsensitive)
		{
			c1 = CharToLower(c1);
			c2 = CharToLower(c2);
		}

		// porównanie znaków
		if (c1 < c2)
			return -1;
		if (c1 > c2)
			return +1;

		// nastêpne znaki
		i1++;
		i2++;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Œcie¿ki do plików

// [Wewnêtrzna]
// Zwraca true, jeœli znak jest jakimœ separatorem katalogów
inline bool IsPathSlash(char Ch)
{
	return (Ch == '\\') || (Ch == '/');
}

// [Wewnêtrzna]
// Porównuje dwie nazwy plików lub katalogów
// Z rozró¿nianiem lub bez rozró¿niania wielkoœci znaków, zale¿nie od systemu
bool PathCmp(const string &s1, const string &s2)
{
	return StrEqualI(s1, s2);
}

// [Wewnêtrzna]
// Rozk³ada œcie¿kê na czêœci
void DecomposePath(const string &s, string *OutPrefix, string *OutPath, string *OutTrailingPathDelimiter)
{
	OutPrefix->clear();
	OutPath->clear();
	OutTrailingPathDelimiter->clear();
	uint Beg = 0, End = s.length();

	// Prefix
	if (!s.empty())
	{
		// Zaczyna siê od <slash>
		if (IsPathSlash(s[0]))
		{
			// <slash> <slash> <serwer> <slash> <udzia³> <slash>
			if (s.length() > 1 && IsPathSlash(s[1]))
			{
				uint i = s.find_first_of("/\\", 2);
				if (i == string::npos) { *OutPrefix = s; return; }
				i = s.find_first_of("/\\", i+1);
				if (i == string::npos) { *OutPrefix = s; return; }
				*OutPrefix = s.substr(0, i+1);
				Beg = i+1;
			}
			// Nie - po prostu <slash>
			else
			{
				CharToStr(OutPrefix, s[0]);
				Beg = 1;
			}
		}
		// <dysk> : <slash>
		else
		{
			uint i = s.find(':');
			if (i != string::npos)
			{
				uint j = s.find_first_of("/\\");
				if (j == string::npos || j > i+1)
				{
					*OutPrefix = s.substr(0, i+1);
					Beg = i+1;
				}
				else if (j > i)
				{
					*OutPrefix = s.substr(0, j+1);
					Beg = j+1;
				}
			}
		}
	}

	// TrailingPathDelimiter
	if (End > Beg)
	{
		if (IsPathSlash(s[End-1]))
		{
			CharToStr(OutTrailingPathDelimiter, s[End-1]);
			End--;
		}
	}

	assert(End >= Beg);
	*OutPath = s.substr(Beg, End-Beg);
}

bool PathIsAbsolute(const string &s)
{
	// Przepisane z czêœci Prefix funkcji DecomposePath
	if (s.empty())
		return false;
	// Zaczyna siê od <slash>
	if (IsPathSlash(s[0]))
		return true;
	// <dysk> : <slash>
	uint i = s.find(':');
	if (i == string::npos)
		return false;
	uint j = s.find_first_of("/\\");
	if (j == string::npos || j > i)
		return true;
	return false;
}

void IncludeTrailingPathDelimiter(string *InOutPath)
{
	if (InOutPath->empty())
		CharToStr(InOutPath, DIR_SEP);
	else
	{
		if (!IsPathSlash((*InOutPath)[InOutPath->length()-1]))
			*InOutPath += DIR_SEP;
	}
}

void IncludeTrailingPathDelimiter(string *OutPath, const string &InPath)
{
	if (InPath.empty())
		CharToStr(OutPath, DIR_SEP);
	else
	{
		if (!IsPathSlash(InPath[InPath.length()-1]))
			*OutPath = InPath + CharToStrR(DIR_SEP);
		else
			*OutPath = InPath;
	}
}

void ExcludeTrailingPathDelimiter(string *InOutPath)
{
	if (InOutPath->empty())
		return;
	if (!IsPathSlash((*InOutPath)[InOutPath->length()-1]))
		InOutPath->erase(InOutPath->length()-1, 1);
}

void ExcludeTrailingPathDelimiter(string *OutPath, const string &InPath)
{
	if (InPath.empty())
		OutPath->clear();
	else
	{
		if (!IsPathSlash(InPath[InPath.length()-1]))
			*OutPath = InPath.substr(0, InPath.length()-1);
		else
			*OutPath = InPath;
	}
}

void ExtractPathPrefix(string *OutPrefix, const string &s)
{
	OutPrefix->clear();
	uint Beg = 0;

	// Skopiowana czêœæ Prefix z funkcji DecomposePath
	if (!s.empty())
	{
		// Zaczyna siê od <slash>
		if (IsPathSlash(s[0]))
		{
			// <slash> <slash> <serwer> <slash> <udzia³> <slash>
			if (s.length() > 1 && IsPathSlash(s[1]))
			{
				uint i = s.find_first_of("/\\", 2);
				if (i == string::npos) { *OutPrefix = s; return; }
				i = s.find_first_of("/\\", i+1);
				if (i == string::npos) { *OutPrefix = s; return; }
				*OutPrefix = s.substr(0, i+1);
				Beg = i+1;
			}
			// Nie - po prostu <slash>
			else
			{
				CharToStr(OutPrefix, s[0]);
				Beg = 1;
			}
		}
		// <dysk> : <slash>
		else
		{
			uint i = s.find(':');
			if (i != string::npos)
			{
				uint j = s.find_first_of("/\\");
				if (j == string::npos || j > i+1)
				{
					*OutPrefix = s.substr(0, i+1);
					Beg = i+1;
				}
				else if (j > i)
				{
					*OutPrefix = s.substr(0, j+1);
					Beg = j+1;
				}
			}
		}
	}

	if (Beg > 0)
		*OutPrefix = s.substr(0, Beg+1);
}

void ExtractFilePath(string *OutPath, const string &s)
{
	size_t i = s.find_last_of("\\/:");
	if (i == string::npos)
		OutPath->clear();
	else
		*OutPath = s.substr(0, i + 1);
}

void ExtractFileName(string *OutFileName, const string &s)
{
	size_t i = s.find_last_of("\\/:");
	if (i == string::npos)
		*OutFileName = s;
	else
		*OutFileName = s.substr(i + 1);
}

void ExtractFileExt(string *OutExt, const string &s)
{
	size_t i = s.find_last_of(".\\/:");
	if ( (i != string::npos) && (i > 0) && (s[i] == '.') )
		*OutExt = s.substr(i);
	else
		OutExt->clear();
}

void ChangeFileExt(string *Out, const string &FileName, const string &Ext)
{
	size_t i = FileName.find_last_of(".\\/:");
	if ( (i == string::npos) || (FileName[i] != '.') )
		*Out = FileName + Ext;
	else
		*Out = FileName.substr(0, i) + Ext;
}

void NormalizePath(string *OutPath, const string &s)
{
	const string Delimiters = "/\\";
	string Path, Trailing, Tmp;
	DecomposePath(s, OutPath, &Path, &Trailing);
	STRING_VECTOR Dirs;
	uint Index = 0;
	while (SplitFirstOf(Path, Delimiters, &Tmp, &Index))
	{
		if (Tmp.empty())
		{
			*OutPath = s;
			return;
		}
		if (Tmp[0] == '.')
		{
			// ".."
			if (Tmp.length() == 2 && Tmp[1] == '.')
			{
				if (Dirs.empty())
					Dirs.push_back(Tmp);
				else
					Dirs.pop_back();
			}
			// "." - nie rób nic
		}
		// Zwyk³y katalog
		else
			Dirs.push_back(Tmp);
	}

	for (uint i = 0; i < Dirs.size(); i++)
	{
		if (!OutPath->empty())
			IncludeTrailingPathDelimiter(OutPath);
		*OutPath += Dirs[i];
	}
	OutPath->append(Trailing);
}

void RelativeToAbsolutePath(string *Out, const string &Base, const string &Path)
{
	if (PathIsAbsolute(Path))
		*Out = Path;
	else
	{
		string Tmp = Base;
		IncludeTrailingPathDelimiter(&Tmp);
		Tmp.append(Path);
		NormalizePath(Out, Tmp);
	}
}

void AbsoluteToRelativePath(string *Out, const string &Base, const string &Target)
{
	string BasePrefix, BasePath, BaseTrailing;
	string TargetPrefix, TargetPath, TargetTrailing;
	DecomposePath(Base, &BasePrefix, &BasePath, &BaseTrailing);
	DecomposePath(Target, &TargetPrefix, &TargetPath, &TargetTrailing);

	if (!PathCmp(BasePrefix, TargetPrefix))
	{
		*Out = Target;
		return;
	}

	// Spisz katalogi Base i Target
	const string DELIMITER = "/\\";
	STRING_VECTOR BaseDirs, TargetDirs;
	size_t i = 0; string Tmp;
	while (SplitFirstOf(BasePath, DELIMITER, &Tmp, &i))
		BaseDirs.push_back(Tmp);
	i = 0;
	while (SplitFirstOf(TargetPath, DELIMITER, &Tmp, &i))
		TargetDirs.push_back(Tmp);

	string R;
	// Wyeliminuj wspólne katalogi z pocz¹tku œcie¿ek
	uint DirsDifferenceIndex = 0;
	while (DirsDifferenceIndex < BaseDirs.size() && DirsDifferenceIndex < TargetDirs.size() && PathCmp(BaseDirs[DirsDifferenceIndex], TargetDirs[DirsDifferenceIndex]))
		DirsDifferenceIndex++;

	// Przepisz na wyjœcie "/.." lub "\.."
	for (uint i = DirsDifferenceIndex; i < BaseDirs.size(); i++)
	{
		if (!R.empty())
			IncludeTrailingPathDelimiter(&R);
		R.append("..");
	}
	// Przepisz na wyjœcie to co w Target
	for (uint i = DirsDifferenceIndex; i < TargetDirs.size(); i++)
	{
		if (!R.empty())
			IncludeTrailingPathDelimiter(&R);
		R.append(TargetDirs[i]);
	}
	// Opcjonalny koñcz¹cy '/' lub '\'
	bool TrailingPathDelimiter = (!Target.empty() && Target[Target.length()-1] == DIR_SEP);
	if (TrailingPathDelimiter)
		IncludeTrailingPathDelimiter(&R);
	NormalizePath(Out, R);
}

const char _DIGITS_L[] = "0123456789abcdefghijklmnopqrstuvwxyz";
const char _DIGITS_U[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Zale¿nie od precyzji
const char * const DOUBLE_TO_STR_FORMAT[] = {
	"%.0", "%.1", "%.2", "%.3", "%.4", "%.5", "%.6", "%.7", "%.8", "%.9", "%.10", "%.11", "%.12", "%.13", "%.14", "%.15", "%.16", "%.17", "%.18", "%.19", "%.20",
};

void DoubleToStr(string *Out, double x, char mode, int precision)
{
	char sz[256], Format[6];

	strcpy(Format, DOUBLE_TO_STR_FORMAT[precision]);
	size_t l = strlen(Format);
	Format[l] = mode;
	Format[l+1] = '\0';

	sprintf(sz, Format, x);
	*Out = sz;
}

void FloatToStr(string *Out, float x, char mode, int precision)
{
	DoubleToStr(Out, static_cast<double>(x), mode, precision);
}

int StrToDouble(double *out, const string &s)
{
	uint4 pos = 0;
	// Whitespace
	for (;;)
	{
		if (pos == s.length())
			return -1;
		if (s[pos] == ' ' || s[pos] == '\t')
			pos++;
		else
			break;
	}
	// Sign
	if (pos == s.length())
		return -1;
	if (s[pos] == '+' || s[pos] == '-')
		pos++;
	// Digits
	if (pos == s.length())
		return -1;
	bool DigitsBeforePoint = false;
	while (pos < s.length() && (s[pos] >= '0' && s[pos] <= '9'))
	{
		DigitsBeforePoint = true;
		pos++;
	}
	// .Digits
	bool DigitsAfterPoint = false;
	if (pos == s.length() && !DigitsBeforePoint)
		return -1;
	if (pos < s.length() && s[pos] == '.')
	{
		pos++;
		while (pos < s.length())
		{
			if (s[pos] >= '0' && s[pos] <= '9')
			{
				pos++;
				DigitsAfterPoint = true;
			}
			else
				break;
		}
	}
	if (!DigitsBeforePoint && !DigitsAfterPoint)
		return -1;
	// {DdEe}
	if (pos < s.length())
	{
		if (!(s[pos] == 'd' || s[pos] == 'D' || s[pos] == 'e' || s[pos] == 'E'))
			return -1;
		pos++;
		// Sign
		if (pos <= s.length() && (s[pos] == '+' || s[pos] == '-'))
			pos++;
		// Digits
		if (pos == s.length())
			return -1;
		while (pos < s.length() && (s[pos] >= '0' && s[pos] <= '9'))
			pos++;
		if (pos < s.length())
			return -1;
	}

	*out = atof(s.c_str());
	return 0;
}

int StrToFloat(float *out, const string &s)
{
	double d;
	int r = StrToDouble(&d, s);
	if (r != 0)
		return r;
	*out = (float)d;
	return 0;
}

void BoolToStr(string *Out, bool x, char mode)
{
	switch (mode)
	{
	case '0':
		*Out = (x ? "1" : "0");
		return;
	case 'F':
		*Out = (x ? "True" : "False");
		return;
	case 'U':
		*Out = (x ? "TRUE" : "FALSE");
		return;
	case 'g':
		*Out = (x ? "t" : "f" );
		return;
	case 'G':
		*Out = (x ? "T" : "F" );
		return;
	default: // 'f'
		*Out = (x ? "true" : "false");
		return;
	}
}

bool StrToBool(bool *result, const string &s)
{
	if (s == "0" || s == "f" || s == "F" || s == "false" || s == "False" || s == "FALSE")
		*result = false;
	else if (s == "1" || s == "t" || s == "T" || s == "true" || s == "True" || s == "TRUE")
		*result = true;
	else
		return false;

	return true;
}

void PtrToStr(string *Out, const void* p)
{
	char sz[9];
	sprintf(sz, "%p", p);
	*Out = sz;
}

bool Charset_WindowsSpecialChar(char a_c, string *a_s)
{
	switch (a_c)
	{
	case '\x84':
	case '\x94':
	case '\x93':
		if (a_s) *a_s = "\""; return true;
	case '\x85':
		if (a_s) *a_s = "..."; return true;
	case '\x96':
	case '\x97':
		if (a_s) *a_s = "-"; return true;
	case '\x92':
		if (a_s) *a_s = "'"; return true;
	default:
		return false;
	}
}

char Charset_Convert_Char(char a_c, CHARSET a_Charset1, CHARSET a_Charset2)
{
	if (a_Charset1 == a_Charset2) return a_c;
	for (int i = 0; i < 18; ++i)
		if (CHARSET_CHARS[a_Charset1][i][0] == a_c)
			return CHARSET_CHARS[a_Charset2][i][0];
	return a_c;
}

void Charset_Convert(string *out, const string &s, CHARSET Charset1, CHARSET Charset2)
{
	if (Charset1 == Charset2) {
		*out = s;
		return;
	}

	size_t i, index = 0; string tmp; bool converted;
	out->clear();
	while (index < s.size()) {
		if (Charset1 == CHARSET_WINDOWS && Charset_WindowsSpecialChar(s[index], &tmp)) {
			*out += tmp;
			index++;
		}
		else {
			converted = false;
			for (i = 0; i < 18; i++) {
				if (StrBegins(s, CHARSET_CHARS[Charset1][i], index)) {
					*out += CHARSET_CHARS[Charset2][i];
					index += CHARSET_CHARS[Charset1][i].size();
					converted = true;
					break;
				}
			}
			if (!converted) {
				*out += s[index];
				index++;
			}
		}
	}
}

uint1 ROT13_TABLE[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,78,79,80,81,82,83,84,85,86,87,88,89,90,65,66,67,68,69,70,71,72,73,74,75,76,77,91,92,93,94,95,96,110,111,112,113,114,115,116,117,118,119,120,121,122,97,98,99,100,101,102,103,104,105,106,107,108,109,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255 };

void Rot13(string *InOut)
{
	for (size_t i = 0; i < InOut->length(); i++)
		(*InOut)[i] = (char)ROT13_TABLE[(uint1)(*InOut)[i]];
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa TimeMeasurer

// Czêœciowo bazuje na kodzie ze standardowego frameworka z DirectX SDK od Microsoftu.

TimeMeasurer::TimeMeasurer()
{
	// Use QueryPerformanceFrequency() to get frequency of timer. If QPF is
	// not supported, we will GetTickCount() which returns milliseconds.
	LARGE_INTEGER li;
	m_QPF_Using = (QueryPerformanceFrequency(&li) != 0);
	if(m_QPF_Using)
	{
		m_QPF_TicksPerSec = li.QuadPart;
		QueryPerformanceCounter(&li);
		m_QPF_Start = li.QuadPart;
	}
	else
		m_Start = GetTickCount();
}

double TimeMeasurer::GetTimeD()
{
	if(m_QPF_Using) {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		int8 DeltaTime = li.QuadPart - m_QPF_Start;
		return (double)DeltaTime / (double)m_QPF_TicksPerSec;
	}
	else
		return (GetTickCount() - m_Start) * 0.001f;
}

float TimeMeasurer::GetTimeF()
{
	if(m_QPF_Using) {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		uint8 DeltaTime = li.QuadPart - m_QPF_Start;
		return (float)DeltaTime / (float)m_QPF_TicksPerSec;
	}
	else
		return (GetTickCount() - m_Start) * 0.001f;
}

TimeMeasurer g_Timer;

void Wait(uint4 Miliseconds)
{
	Sleep(Miliseconds);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Matematyka

bool is_finite(float x)  { return (_finite(x) != 0); }
bool is_finite(double x) { return (_finite(x) != 0); }
bool is_nan(float x)  { return (_isnan(x) != 0); }
bool is_nan(double x) { return (_isnan(x) != 0); }

// Funkcje SmoothCD s¹ napisane na podstawie ksi¹¿ki:
// "Game Programming Gems", tom 4, rozdz. 1.10, str. 95.

void SmoothCD(float *InOutPos, float Dest, float *InOutVel, float SmoothTime, float TimeDelta)
{
	float Omega = 2.f / SmoothTime;
	float x = Omega * TimeDelta;
	// Przybli¿enie funkcji 1 / exp(x) doœæ dok³adne dla wartoœci 0..1
	float Exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
	float Change = *InOutPos - Dest;
	float Temp = (*InOutVel + Omega * Change) * TimeDelta;
	*InOutVel = (*InOutVel - Omega * Temp) * Exp;
	*InOutPos = Dest + (Change + Temp) * Exp;
}

void SmoothCD(float *InOutPos, float Dest, float *InOutVel, float SmoothTime, float TimeDelta, float MaxSpeed)
{
	float Omega = 2.f / SmoothTime;
	float x = Omega * TimeDelta;
	// Przybli¿enie funkcji 1 / exp(x) doœæ dok³adne dla wartoœci 0..1
	float Exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
	float Change = *InOutPos - Dest;
	float MaxChange = MaxSpeed*SmoothTime;
	Change = minmax(-MaxChange, Change, MaxChange);
	float Temp = (*InOutVel + Omega * Change) * TimeDelta;
	*InOutVel = (*InOutVel - Omega * Temp) * Exp;
	*InOutPos = Dest + (Change + Temp) * Exp;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Parser wiersza poleceñ

class CmdLineParser_pimpl
{
private:
	struct SHORT_OPT
	{
		uint Id;
		char Opt;
		bool Parameter;

		SHORT_OPT(uint Id, char Opt, bool Parameter) : Id(Id), Opt(Opt), Parameter(Parameter) { }
	};

	struct LONG_OPT
	{
		uint Id;
		string Opt;
		bool Parameter;

		LONG_OPT(uint Id, string Opt, bool Parameter) : Id(Id), Opt(Opt), Parameter(Parameter) { }
	};

	// ==== Warstwa I ====

	// Zawsze niezerowe jest albo m_argv, albo m_CmdLine.
	char **m_argv;
	const char *m_CmdLine;
	// Wa¿ne tylko, jeœli m_argv != NULL
	int m_argc;
	// Wa¿ne tylko jeœli m_CmdLine != NULL
	size_t m_CmdLineLength;
	// Jeœli m_argv != NULL, przechowuje indeks nastêpnego argumentu w m_argv.
	// Jeœli m_CmdLine != NULL, przechowuje indeks nastêpnego znaku w m_CmdLine.
	int m_ArgIndex;

	// Zwraca nastêpny argumet
	// Zwraca false, jeœli nie odczytano nastêpnego argumentu, bo ju¿ koniec.
	bool ReadNextArg(string *OutArg);

	// ==== Zarejestrowane argumenty ====

	std::vector<SHORT_OPT> m_ShortOpts;
	std::vector<LONG_OPT> m_LongOpts;

	// Zwraca wskaŸnk do struktury ze znalezion¹ opcj¹ lub NULL jeœli nie znalaz³
	SHORT_OPT * FindShortOpt(char Opt);
	LONG_OPT * FindLongOpt(const string &Opt);

	// ==== Warstwa II ====

	// Stan - czy jesteœmy wewn¹rz parsowania ci¹gu po³¹czonych opcji jednoznakowych
	bool m_InsideMultioption;
	// Zapamiêtany ostatnio pobrany argument - tylko jeœli m_InsideMultioption == true
	string m_LastArg;
	// Indeks nastêpnego znaku do sparsowania w tym argumencie - tylko jeœli m_InsideMultioption == true
	size_t m_LastArgIndex;
	// Dane do zwracania w zwi¹zku z ostatnio odczytan¹ informacj¹
	uint m_LastOptId;
	string m_LastParameter;

public:
	CmdLineParser_pimpl(int argc, char **argv);
	CmdLineParser_pimpl(const char *CmdLine);

	void RegisterOpt(uint Id, char Opt, bool Parameter);
	void RegisterOpt(uint Id, const string &Opt, bool Parameter);

	CmdLineParser::RESULT ReadNext();
	uint GetOptId();
	const string & GetParameter();
};

bool CmdLineParser_pimpl::ReadNextArg(string *OutArg)
{
	if (m_argv != NULL)
	{
		if (m_ArgIndex >= m_argc) return false;

		*OutArg = m_argv[m_ArgIndex];
		m_ArgIndex++;
		return true;
	}
	else
	{
		if (m_ArgIndex >= (int)m_CmdLineLength) return false;
		
		// Algorytm parsowania opracowany na podstawie dog³êbnych badañ zachowania wiersza poleceñ Windows
		// z ró¿nymi dziwnymi kombinacjami znaków w parametrach i sposobu ich przetwarzania na argc i argv
		// przekazywane przez system do main().
		OutArg->clear();
		bool InsideQuotes = false;
		while (m_ArgIndex < (int)m_CmdLineLength)
		{
			char Ch = m_CmdLine[m_ArgIndex];
			if (Ch == '\\')
			{
				// Analiza dalszego ci¹gu ³añcucha
				bool FollowedByQuote = false; // wynik analizy 1
				size_t BackslashCount = 1; // wynik analizy 2
				int TmpIndex = m_ArgIndex + 1;
				while (TmpIndex < (int)m_CmdLineLength)
				{
					char TmpCh = m_CmdLine[TmpIndex];
					if (TmpCh == '\\')
					{
						BackslashCount++;
						TmpIndex++;
					}
					else if (TmpCh == '"')
					{
						FollowedByQuote = true;
						break;
					}
					else
						break;
				}

				// Stosowne do wyników analizy dalsze postêpowanie
				if (FollowedByQuote)
				{
					// Parzysta liczba '\'
					if (BackslashCount % 2 == 0)
					{
						for (size_t i = 0; i < BackslashCount / 2; i++)
							*OutArg += '\\';
						m_ArgIndex += BackslashCount + 1;
						InsideQuotes = !InsideQuotes;
					}
					// Nieparzysta liczba '\'
					else
					{
						for (size_t i = 0; i < BackslashCount / 2; i++)
							*OutArg += '\\';
						*OutArg += '"';
						m_ArgIndex += BackslashCount + 1;
					}
				}
				else
				{
					for (size_t i = 0; i < BackslashCount; i++)
						*OutArg += '\\';
					m_ArgIndex += BackslashCount;
				}
			}
			else if (Ch == '"')
			{
				InsideQuotes = !InsideQuotes;
				m_ArgIndex++;
			}
			else if (CharIsWhitespace(Ch))
			{
				if (InsideQuotes)
				{
					*OutArg += Ch;
					m_ArgIndex++;
				}
				else
				{
					m_ArgIndex++;
					break;
				}
			}
			else
			{
				*OutArg += Ch;
				m_ArgIndex++;
			}
		}

		// Pomiñ dodatkowe odstêpy
		while (m_ArgIndex < (int)m_CmdLineLength && CharIsWhitespace(m_CmdLine[m_ArgIndex]))
			m_ArgIndex++;

		return true;
	}
}

CmdLineParser_pimpl::SHORT_OPT * CmdLineParser_pimpl::FindShortOpt(char Opt)
{
	for (uint i = 0; i < m_ShortOpts.size(); i++)
		if (m_ShortOpts[i].Opt == Opt)
			return &m_ShortOpts[i];
	return NULL;
}

CmdLineParser_pimpl::LONG_OPT * CmdLineParser_pimpl::FindLongOpt(const string &Opt)
{
	for (uint i = 0; i < m_LongOpts.size(); i++)
		if (m_LongOpts[i].Opt == Opt)
			return &m_LongOpts[i];
	return NULL;
}

CmdLineParser_pimpl::CmdLineParser_pimpl(int argc, char **argv) :
	m_argv(argv),
	m_CmdLine(NULL),
	m_argc(argc),
	m_ArgIndex(1),
	m_InsideMultioption(false),
	m_LastArgIndex(0),
	m_LastOptId(0)
{
	assert(argc > 0);
	assert(argv != NULL);
}

CmdLineParser_pimpl::CmdLineParser_pimpl(const char *CmdLine) :
	m_argv(NULL),
	m_CmdLine(CmdLine),
	m_argc(0),
	m_ArgIndex(0),
	m_InsideMultioption(false),
	m_LastArgIndex(0),
	m_LastOptId(0)
{
	assert(CmdLine != NULL);

	m_CmdLineLength = strlen(m_CmdLine);

	// Pomiñ pocz¹tkowe odstêpy
	while (m_ArgIndex < (int)m_CmdLineLength && CharIsWhitespace(m_CmdLine[m_ArgIndex]))
		m_ArgIndex++;
}

void CmdLineParser_pimpl::RegisterOpt(uint Id, char Opt, bool Parameter)
{
	assert(Opt != '\0');

	m_ShortOpts.push_back(SHORT_OPT(Id, Opt, Parameter));
}

void CmdLineParser_pimpl::RegisterOpt(uint Id, const string &Opt, bool Parameter)
{
	assert(!Opt.empty());
	
	m_LongOpts.push_back(LONG_OPT(Id, Opt, Parameter));
}

CmdLineParser::RESULT CmdLineParser_pimpl::ReadNext()
{
	if (m_InsideMultioption)
	{
		// £añcuch rozpoczynaj¹cy siê od m_LastArg[m_LastArgIndex] jest krótk¹ opcj¹ - jeden znak, dalej mog¹ byæ ró¿ne rzeczy.
		assert(m_LastArgIndex < m_LastArg.length());
		SHORT_OPT *so = FindShortOpt(m_LastArg[m_LastArgIndex]);
		// Nie ma takiej opcji - b³¹d
		if (so == NULL)
		{
			m_LastOptId = 0;
			m_LastParameter.clear();
			return CmdLineParser::RESULT_ERROR;
		}
		// Ta opcja powinna mieæ parametr
		if (so->Parameter)
		{
			// Co jest dalej?
			// Dalej nie ma nic
			if (m_LastArg.length() == m_LastArgIndex+1)
			{
				// Nastêpny argument to parameter - wczytaj go prosto tam gdzie trzeba
				if (!ReadNextArg(&m_LastParameter))
				{
					// Nie ma nastêpnego argumentu, a oczekiwany jest parametr - b³¹d
					m_LastOptId = 0;
					m_LastParameter.clear();
					return CmdLineParser::RESULT_ERROR;
				}
				m_InsideMultioption = false;
				m_LastOptId = so->Id;
				return CmdLineParser::RESULT_OPT;
			}
			// Dalej jest '='
			else if (m_LastArg[m_LastArgIndex+1] == '=')
			{
				m_InsideMultioption = false;
				m_LastParameter = m_LastArg.substr(m_LastArgIndex+2);
				m_LastOptId = so->Id;
				return CmdLineParser::RESULT_OPT;
			}
			// Dalej jest coœ innego - to bezpoœrednio parametr
			else
			{
				m_InsideMultioption = false;
				m_LastParameter = m_LastArg.substr(m_LastArgIndex+1);
				m_LastOptId = so->Id;
				return CmdLineParser::RESULT_OPT;
			}
		}
		// Ta opcja nie powinna mieæ parametru
		else
		{
			// Koniec
			if (m_LastArg.length() == m_LastArgIndex+1)
			{
				m_InsideMultioption = false;
				m_LastParameter.clear();
				m_LastOptId = so->Id;
				return CmdLineParser::RESULT_OPT;
			}
			// Nie koniec - nastêpne parametry w tej samej komendzie - dalej ten specjalny stan!
			else
			{
				// m_InsideMultioption = true; - pozostaje
				// m_LastArg pozostaje wype³niony
				m_LastArgIndex++;

				m_LastParameter.clear();
				m_LastOptId = so->Id;
				return CmdLineParser::RESULT_OPT;
			}
		}
	}
	else
	{
		// Pobierz nastêpny argument
		// U¿yjemy tu sobie m_LastArg, bo i czemu nie - dopóki m_InsideMultioption == false, i tak jest nieistotny.
		if (!ReadNextArg(&m_LastArg))
		{
			// Koniec
			m_LastParameter.clear();
			m_LastOptId = 0;
			return CmdLineParser::RESULT_END;
		}
		
		// SprawdŸ pierwszy znak
		// - Zaczyna siê od '-'
		if (!m_LastArg.empty() && m_LastArg[0] == '-')
		{
			// Zaczyna siê od '--'
			if (m_LastArg.length() > 1 && m_LastArg[1] == '-')
			{
				size_t EqualIndex = m_LastArg.find('=', 2);
				// Jest znak '='
				if (EqualIndex != string::npos)
				{
					// To ma byæ d³uga opcja "--nazwa=param"
					LONG_OPT *lo = FindLongOpt(m_LastArg.substr(2, EqualIndex-2));
					// Jeœli nie ma takiej d³ugiej opcji - b³¹d
					// Ta opcja musi oczekiwaæ parametru skoro jest '=' - jeœli nie oczekuje, to b³¹d
					if (lo == NULL || lo->Parameter == false)
					{
						m_LastOptId = 0;
						m_LastParameter.clear();
						return CmdLineParser::RESULT_ERROR;
					}
					m_LastParameter = m_LastArg.substr(EqualIndex+1);
					m_LastOptId = lo->Id;
					return CmdLineParser::RESULT_OPT;
				}
				// Nie ma znaku '='
				else
				{
					// To ma byæ d³uga opcja "--nazwa" lub "--nazwa param"
					LONG_OPT *lo = FindLongOpt(m_LastArg.substr(2));
					// Jeœli nie ma takiej d³ugiej opcji - b³¹d
					if (lo == NULL)
					{
						m_LastOptId = 0;
						m_LastParameter.clear();
						return CmdLineParser::RESULT_ERROR;
					}
					// Jeœli ta opcja oczekuje parametru
					if (lo->Parameter)
					{
						// Nastêpny argument to parameter - wczytaj go prosto tam gdzie trzeba
						if (!ReadNextArg(&m_LastParameter))
						{
							// Nie ma nastêpnego argumentu, a oczekiwany jest parametr - b³¹d
							m_LastOptId = 0;
							m_LastParameter.clear();
							return CmdLineParser::RESULT_ERROR;
						}
					}
					// Nie oczekuje parametru
					else
						m_LastParameter.clear();
					// Koniec - sukces
					m_LastOptId = lo->Id;
					return CmdLineParser::RESULT_OPT;
				}
			}
			// Zaczyna siê od samego '-'
			else
			{
				// Krótka opcja
				// To jest opcja jednoznakowa. Dalej mog¹ byæ ró¿ne rzeczy
				// Samo '-' - b³¹d
				if (m_LastArg.length() < 2)
				{
					m_LastOptId = 0;
					m_LastParameter.clear();
					return CmdLineParser::RESULT_ERROR;
				}
				SHORT_OPT *so = FindShortOpt(m_LastArg[1]);
				// Nie ma takiej opcji - b³¹d
				if (so == NULL)
				{
					m_LastOptId = 0;
					m_LastParameter.clear();
					return CmdLineParser::RESULT_ERROR;
				}
				// Ta opcja powinna mieæ parametr
				if (so->Parameter)
				{
					// Co jest dalej?
					// Dalej nie ma nic
					if (m_LastArg.length() == 2)
					{
						// Nastêpny argument to parameter - wczytaj go prosto tam gdzie trzeba
						if (!ReadNextArg(&m_LastParameter))
						{
							// Nie ma nastêpnego argumentu, a oczekiwany jest parametr - b³¹d
							m_LastOptId = 0;
							m_LastParameter.clear();
							return CmdLineParser::RESULT_ERROR;
						}
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
					// Dalej jest '='
					else if (m_LastArg[2] == '=')
					{
						m_LastParameter = m_LastArg.substr(3);
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
					// Dalej jest coœ innego - to bezpoœrednio parametr
					else
					{
						m_LastParameter = m_LastArg.substr(2);
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
				}
				// Ta opcja nie powinna mieæ parametru
				else
				{
					// Koniec
					if (m_LastArg.length() == 2)
					{
						m_LastParameter.clear();
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
					// Nie koniec - nastêpne parametry w tej samej komendzie - specjalny stan!
					else
					{
						m_InsideMultioption = true;
						// m_LastArg ju¿ wype³niony
						m_LastArgIndex = 2;

						m_LastParameter.clear();
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
				}
			}
		}
		// - Zaczyna siê od '/'
		else if (!m_LastArg.empty() && m_LastArg[0] == '/')
		{
			size_t EqualIndex = m_LastArg.find('=', 1);
			// Jest znak '='
			if (EqualIndex != string::npos)
			{
				// To ma byæ krótka lub d³uga opcja "/nazwa=param"
				// To mo¿e byæ krótka
				if (EqualIndex == 2)
				{
					SHORT_OPT *so = FindShortOpt(m_LastArg[1]);
					// Znaleziono krótk¹ opcjê
					if (so != NULL)
					{
						// Ta opcja musi oczekiwaæ parametru skoro jest '=' - jeœli nie oczekuje, to b³¹d
						if (so->Parameter == false)	
						{
							m_LastOptId = 0;
							m_LastParameter.clear();
							return CmdLineParser::RESULT_ERROR;
						}
						m_LastParameter = m_LastArg.substr(EqualIndex+1);
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
				}
				// To nie jest krótka opcja bo nazwa za d³uga lub bo nie znaleziono takiej krótkiej - to musi byæ d³uga opcja
				LONG_OPT *lo = FindLongOpt(m_LastArg.substr(1, EqualIndex-1));
				// Jeœli nie ma takiej d³ugiej opcji - b³¹d
				// Ta opcja musi oczekiwaæ parametru skoro jest '=' - jeœli nie oczekuje, to b³¹d
				if (lo == NULL || lo->Parameter == false)
				{
					m_LastOptId = 0;
					m_LastParameter.clear();
					return CmdLineParser::RESULT_ERROR;
				}
				m_LastParameter = m_LastArg.substr(EqualIndex+1);
				m_LastOptId = lo->Id;
				return CmdLineParser::RESULT_OPT;
			}
			// Nie ma znaku '='
			else
			{
				// To ma byæ krótka lub d³uga opcja "/nazwa" lub "/nazwa param"
				// To mo¿e byæ krótka opcja
				if (m_LastArg.length() == 2)
				{
					SHORT_OPT *so = FindShortOpt(m_LastArg[1]);
					// Znaleziono krótk¹ opcjê
					if (so != NULL)
					{
						// Jeœli ta opcja oczekuje parametru
						if (so->Parameter)
						{
							// Nastêpny argument to parameter - wczytaj go prosto tam gdzie trzeba
							if (!ReadNextArg(&m_LastParameter))
							{
								// Nie ma nastêpnego argumentu, a oczekiwany jest parametr - b³¹d
								m_LastOptId = 0;
								m_LastParameter.clear();
								return CmdLineParser::RESULT_ERROR;
							}
						}
						// Nie ma parametru
						else
							m_LastParameter.clear();
						// Koniec - sukces
						m_LastOptId = so->Id;
						return CmdLineParser::RESULT_OPT;
					}
				}
				// To nie jest krótka opcja bo nazwa za d³uga lub bo nie znaleziono takiej krótkiej - to musi byæ d³uga opcja
				LONG_OPT *lo = FindLongOpt(m_LastArg.substr(1));
				// Jeœli nie ma takiej d³ugiej opcji - b³¹d
				if (lo == NULL)
				{
					m_LastOptId = 0;
					m_LastParameter.clear();
					return CmdLineParser::RESULT_ERROR;
				}
				// Jeœli ta opcja oczekuje parametru
				if (lo->Parameter)
				{
					// Nastêpny argument to parameter - wczytaj go prosto tam gdzie trzeba
					if (!ReadNextArg(&m_LastParameter))
					{
						// Nie ma nastêpnego argumentu, a oczekiwany jest parametr - b³¹d
						m_LastOptId = 0;
						m_LastParameter.clear();
						return CmdLineParser::RESULT_ERROR;
					}
				}
				// Nie ma parametru
				else
					m_LastParameter.clear();
				// Koniec - sukces
				m_LastOptId = lo->Id;
				return CmdLineParser::RESULT_OPT;
			}
		}
		// - Zaczyna siê od innego znaku lub jest pusty
		else
		{
			// To jest go³y parametr bez opcji
			m_LastOptId = 0;
			m_LastParameter = m_LastArg;
			return CmdLineParser::RESULT_PARAMETER;
		}
	}
}

uint CmdLineParser_pimpl::GetOptId()
{
	return m_LastOptId;
}

const string & CmdLineParser_pimpl::GetParameter()
{
	return m_LastParameter;
}

CmdLineParser::CmdLineParser(int argc, char **argv) :
	pimpl(new CmdLineParser_pimpl(argc, argv))
{
}

CmdLineParser::CmdLineParser(const char *CmdLine) :
	pimpl(new CmdLineParser_pimpl(CmdLine))
{
}

CmdLineParser::~CmdLineParser()
{
	pimpl.reset();
}

void CmdLineParser::RegisterOpt(uint Id, char Opt, bool Parameter)
{
	pimpl->RegisterOpt(Id, Opt, Parameter);
}

void CmdLineParser::RegisterOpt(uint Id, const string &Opt, bool Parameter)
{
	pimpl->RegisterOpt(Id, Opt, Parameter);
}

CmdLineParser::RESULT CmdLineParser::ReadNext()
{
	return pimpl->ReadNext();
}

uint CmdLineParser::GetOptId()
{
	return pimpl->GetOptId();
}

const string & CmdLineParser::GetParameter()
{
	return pimpl->GetParameter();
}


Format::Format(const Format &f, const string &Element) :
	pimpl(f.pimpl)
{
	size_t index = pimpl->m_String.find(pimpl->m_Sep, pimpl->m_Index);

	// nie ma ju¿ gdzie dodaæ - nie zmieniamy
	if (index == string::npos) return;

	pimpl->m_String.erase(index, 1);
	pimpl->m_String.insert(index, Element);
	pimpl->m_Index = index + Element.length();
}

Format::Format(const Format &f, const char *Element) :
	pimpl(f.pimpl)
{
	size_t index = pimpl->m_String.find(pimpl->m_Sep, pimpl->m_Index);

	// nie ma ju¿ gdzie dodaæ - nie zmieniamy
	if (index == string::npos) return;

	pimpl->m_String.erase(index, 1);
	pimpl->m_String.insert(index, Element);
	pimpl->m_Index = index + strlen(Element);
}

} // namespace common

cstring format(cstring str, ...)
{
	const uint FORMAT_STRINGS = 8;
	const uint FORMAT_LENGTH = 2048;

	assert(str);

	static char buf[FORMAT_STRINGS][FORMAT_LENGTH];
	static int marker = 0;

	va_list list;
	va_start(list, str);
	_vsnprintf_s((char*)buf[marker], FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	char* cbuf = buf[marker];
	cbuf[FORMAT_LENGTH - 1] = 0;
	va_end(list);

	marker = (marker + 1) % FORMAT_STRINGS;

	return cbuf;
}
