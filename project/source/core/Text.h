#pragma once

// gdy trzeba coœ na chwilê wczytaæ to mo¿na u¿ywaæ tego stringa
extern string g_tmp_string;

//-----------------------------------------------------------------------------
struct AnyString
{
	AnyString(cstring s) : s(s)
	{
		assert(s);
		assert(strlen(s) > 0);
	}
	AnyString(const string& str) : s(str.c_str())
	{
		assert(!str.empty());
	}

	operator cstring() const
	{
		return s;
	}

	cstring s;
};

inline bool operator == (const string& s1, const AnyString& s2)
{
	return s1 == s2.s;
}
inline bool operator == (const AnyString& s1, const string& s2)
{
	return s2 == s1.s;
}
inline bool operator == (cstring s1, const AnyString& s2)
{
	return strcmp(s1, s2.s) == 0;
}
inline bool operator == (const AnyString& s1, cstring s2)
{
	return strcmp(s1.s, s2) == 0;
}
inline bool operator != (const string& s1, const AnyString& s2)
{
	return s1 != s2.s;
}
inline bool operator != (const AnyString& s1, const string& s2)
{
	return s2 != s1.s;
}
inline bool operator != (cstring s1, const AnyString& s2)
{
	return strcmp(s1, s2.s) != 0;
}
inline bool operator != (const AnyString& s1, cstring s2)
{
	return strcmp(s1.s, s2) != 0;
}

//-----------------------------------------------------------------------------
struct AnyStringNull
{
	AnyStringNull(cstring s) : s(s)
	{
		if(s)
			assert(strlen(s) > 0);
	}
	AnyStringNull(const string& str) : s(str.c_str())
	{
		assert(!str.empty());
	}

	operator cstring() const
	{
		return s;
	}

	cstring s;
};

//-----------------------------------------------------------------------------
template<bool AllowNull, uint MinLength>
struct _InString
{
	_InString(cstring s) : s(s)
	{
		if(!AllowNull)
			assert(s);
		if(MinLength > 0u)
			assert(s && strlen(s) >= MinLength);
	}

	_InString(const string& str) : s(str.c_str())
	{
		if(MinLength > 0u)
			assert(str.length() >= MinLength);
	}

	cstring s;
};

typedef _InString<false, 0> InString;
typedef _InString<false, 1> InString1;
typedef _InString<true, 0> InStringN;

cstring Format(cstring fmt, ...);
cstring FormatList(cstring fmt, va_list lis);
void FormatStr(string& str, cstring fmt, ...);
cstring Upper(cstring str);
void SplitText(char* buf, vector<cstring>& lines);
bool Unescape(const string& str_in, uint pos, uint length, string& str_out);
inline bool Unescape(const string& str_in, string& str_out)
{
	return Unescape(str_in, 0u, str_in.length(), str_out);
}
bool StringInString(cstring s1, cstring s2);
cstring Escape(const InString& str, char quote = '"');
cstring Escape(const InString& str, string& out, char quote = '"');
cstring EscapeChar(char c);
cstring EscapeChar(char c, string& out);
string* ToString(const wchar_t* str);
void Replace(string& s, cstring in_chars, cstring out_chars);

// return index of character in cstring
inline int StrCharIndex(cstring chrs, char c)
{
	int index = 0;
	do
	{
		if(*chrs == c)
			return index;
		++index;
		++chrs;
	} while(*chrs);

	return -1;
}

namespace TextHelper
{
	// parse string to number, return 0-broken, 1-int, 2-float
	int ToNumber(cstring s, __int64& i, float& f);
	bool ToInt(cstring s, int& result);
	bool ToUint(cstring s, uint& result);
	bool ToFloat(cstring s, float& result);
	bool ToBool(cstring s, bool& result);
}

struct Trimmer
{
	bool done;
	Trimmer() : done(false) {}
	bool operator () (char c) const
	{
		if(done)
			return false;
		return c == ' ';
	}
};

// trim from start
inline string& Ltrim(string& str)
{
	str.erase(str.begin(), find_if(str.begin(), str.end(), [](char& ch)->bool { return !isspace(ch); }));
	return str;
}

// trim from end
inline string& Rtrim(string& str)
{
	str.erase(find_if(str.rbegin(), str.rend(), [](char& ch)->bool { return !isspace(ch); }).base(), str.end());
	return str;
}

// trim from both ends
inline string& Trim(string& str)
{
	return Ltrim(Rtrim(str));
}

inline string Trimmed(const string& str)
{
	string s = str;
	Trim(s);
	return s;
}

template<typename T, class Pred>
inline void Join(const vector<T>& v, string& s, cstring separator, Pred pred)
{
	if(v.empty())
		return;
	if(v.size() == 1)
		s += pred(v.front());
	else
	{
		for(vector<T>::const_iterator it = v.begin(), end = v.end() - 1; it != end; ++it)
		{
			s += pred(*it);
			s += separator;
		}
		s += pred(*(v.end() - 1));
	}
}

inline char StrContains(cstring s, cstring chrs)
{
	assert(s && chrs);

	while(true)
	{
		char c = *s++;
		if(c == 0)
			return 0;
		cstring ch = chrs;
		while(true)
		{
			char c2 = *ch++;
			if(c2 == 0)
				break;
			if(c == c2)
				return c;
		}
	}
}

inline char CharInStr(char c, cstring chrs)
{
	assert(chrs);

	while(true)
	{
		char c2 = *chrs++;
		if(c2 == 0)
			return 0;
		if(c == c2)
			return c;
	}
}

//-----------------------------------------------------------------------------
struct CstringComparer
{
	bool operator() (cstring s1, cstring s2) const
	{
		return _stricmp(s1, s2) > 0;
	}
};

// convert \r \r\n -> \n or remove all
void RemoveEndOfLine(string& str, bool remove);
