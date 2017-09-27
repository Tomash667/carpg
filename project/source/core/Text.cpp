#include "Pch.h"
#include "Core.h"

static const uint FORMAT_STRINGS = 8;
static const uint FORMAT_LENGTH = 2048;
static char format_buf[FORMAT_STRINGS][FORMAT_LENGTH];
static int format_marker;
static string g_escp;
string g_tmp_string;
static char escape_from[] = { '\n', '\t', '\r', ' ' };
static cstring escape_to[] = { "\\n", "\\t", "\\r", " " };

//=================================================================================================
// Formatowanie ci�gu znak�w
//=================================================================================================
cstring Format(cstring str, ...)
{
	assert(str);

	va_list list;
	va_start(list, str);
	char* cbuf = format_buf[format_marker];
	_vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	cbuf[FORMAT_LENGTH - 1] = 0;
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	va_end(list);

	return cbuf;
}

//=================================================================================================
cstring FormatList(cstring str, va_list list)
{
	assert(str);

	char* cbuf = format_buf[format_marker];
	_vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	cbuf[FORMAT_LENGTH - 1] = 0;
	format_marker = (format_marker + 1) % FORMAT_STRINGS;

	return cbuf;
}

//=================================================================================================
void FormatStr(string& s, cstring str, ...)
{
	assert(str);
	va_list list;
	va_start(list, str);
	s.resize(FORMAT_LENGTH);
	char* cbuf = (char*)s.data();
	int len = _vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	if(len >= 0)
	{
		cbuf[len] = 0;
		s.resize(len);
	}
	else
	{
		cbuf[0] = 0;
		s.clear();
	}
	va_end(list);
}

//=================================================================================================
cstring Upper(cstring str)
{
	assert(str);

	char* cbuf = format_buf[format_marker];
	if(*str == 0)
		cbuf[0] = 0;
	else
	{
		cbuf[0] = toupper(str[0]);
		strcpy(cbuf + 1, str + 1);
	}

	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return cbuf;
}

//=================================================================================================
int TextHelper::ToNumber(cstring s, __int64& i, float& f)
{
	assert(s);

	i = 0;
	f = 0;
	uint diver = 10;
	uint digits = 0;
	char c;
	bool sign = false;
	if(*s == '-')
	{
		sign = true;
		++s;
	}

	while((c = *s) != 0)
	{
		if(c == '.')
		{
			++s;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			i *= 10;
			i += (int)c - '0';
		}
		else
			return 0;
		++s;
	}

	if(c == 0)
	{
		if(sign)
			i = -i;
		f = (float)i;
		return 1;
	}

	while((c = *s) != 0)
	{
		if(c == 'f')
		{
			if(digits == 0)
				return 0;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			++digits;
			f += ((float)((int)c - '0')) / diver;
			diver *= 10;
		}
		else
			return 0;
		++s;
	}
	f += (float)i;
	if(sign)
	{
		f = -f;
		i = -i;
	}
	return 2;
}

//=================================================================================================
bool TextHelper::ToInt(cstring s, int& result)
{
	__int64 i;
	float f;
	if(ToNumber(s, i, f) != 0 && InRange<int>(i))
	{
		result = (int)i;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool TextHelper::ToUint(cstring s, uint& result)
{
	__int64 i;
	float f;
	if(ToNumber(s, i, f) != 0 && InRange<uint>(i))
	{
		result = (uint)i;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool TextHelper::ToFloat(cstring s, float& result)
{
	__int64 i;
	float f;
	if(ToNumber(s, i, f) != 0)
	{
		result = f;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool TextHelper::ToBool(cstring s, bool& result)
{
	if(_stricmp(s, "true") == 0)
	{
		result = true;
		return true;
	}
	else if(_stricmp(s, "false") == 0)
	{
		result = false;
		return true;
	}
	else
	{
		int value;
		if(!ToInt(s, value) && value != 0 && value != 1)
			return false;
		result = (value == 1);
		return true;
	}
}

//=================================================================================================
void SplitText(char* buf, vector<cstring>& lines)
{
	cstring start = buf;
	int len = 0;

	while(true)
	{
		char c = *buf;
		if(c == 0x0D || c == 0x0A)
		{
			if(len)
			{
				lines.push_back(start);
				len = 0;
			}
			start = buf + 1;
			*buf = 0;
		}
		else if(c == 0)
		{
			if(len)
				lines.push_back(start);
			break;
		}
		else
			++len;
		++buf;
	}
}

//=================================================================================================
bool Unescape(const string& str_in, uint pos, uint size, string& str_out)
{
	str_out.clear();
	str_out.reserve(str_in.length());

	cstring unesc = "nt\\\"'";
	cstring esc = "\n\t\\\"'";
	uint end = pos + size;

	for(; pos < end; ++pos)
	{
		if(str_in[pos] == '\\')
		{
			++pos;
			if(pos == size)
			{
				Error("Unescape error in string \"%.*s\", character '\\' at end of string.", size, str_in.c_str() + pos);
				return false;
			}
			int index = StrCharIndex(unesc, str_in[pos]);
			if(index != -1)
				str_out += esc[index];
			else
			{
				Error("Unescape error in string \"%.*s\", unknown escape sequence '\\%c'.", size, str_in.c_str() + pos, str_in[pos]);
				return false;
			}
		}
		else
			str_out += str_in[pos];
	}

	return true;
}

//=================================================================================================
cstring Escape(const InString& s, char quote)
{
	cstring str = s.s;
	char* out = format_buf[format_marker];
	char* out_buf = out;
	cstring from = "\n\t\r";
	cstring to = "ntr";

	char c;
	while((c = *str) != 0)
	{
		int index = StrCharIndex(from, c);
		if(index == -1)
		{
			if(c == quote)
				*out++ = '\\';
			*out++ = c;
		}
		else
		{
			*out++ = '\\';
			*out++ = to[index];
		}
		++str;
	}

	*out = 0;
	out_buf[FORMAT_LENGTH - 1] = 0;
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return out_buf;
}

//=================================================================================================
cstring Escape(const InString& str, string& out, char quote)
{
	cstring s = str.s;
	out.clear();
	cstring from = "\n\t\r";
	cstring to = "ntr";

	char c;
	while((c = *s) != 0)
	{
		int index = StrCharIndex(from, c);
		if(index == -1)
		{
			if(c == quote)
				out += '\\';
			out += c;
		}
		else
		{
			out += '\\';
			out += to[index];
		}
		++s;
	}

	return out.c_str();
}

//=================================================================================================
cstring EscapeChar(char c)
{
	char* out = format_buf[format_marker];
	for(int i = 0; i < countof(escape_from); ++i)
	{
		if(c == escape_from[i])
		{
			strcpy(out, escape_to[i]);
			format_marker = (format_marker + 1) % FORMAT_STRINGS;
			return out;
		}
	}

	if(isprint(c))
	{
		out[0] = c;
		out[1] = 0;
	}
	else
		_snprintf_s(out, FORMAT_LENGTH, FORMAT_LENGTH - 1, "0x%u", (uint)c);

	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return out;
}

//=================================================================================================
cstring EscapeChar(char c, string& out)
{
	cstring esc = EscapeChar(c);
	out = esc;
	return out.c_str();
}

//=================================================================================================
bool StringInString(cstring s1, cstring s2)
{
	while(true)
	{
		if(*s1 == *s2)
		{
			++s1;
			++s2;
			if(*s2 == 0)
				return true;
		}
		else
			return false;
	}
}

//=================================================================================================
string* ToString(const wchar_t* str)
{
	string* s = StringPool.Get();
	if(str == nullptr)
	{
		*s = "null";
		return s;
	}
	int len = lstrlenW(str);
	s->resize(len);
	wcstombs((char*)s->c_str(), str, len);
	return s;
}

//=================================================================================================
void RemoveEndOfLine(string& str, bool remove)
{
	if(remove)
	{
		uint pos = 0;
		while(pos < str.length())
		{
			char c = str[pos];
			if(c == '\n' || c == '\r')
				str.erase(pos, 1);
			else
				++pos;
		}
	}
	else
	{
		uint pos = 0;
		while(pos < str.length())
		{
			char c = str[pos];
			if(c == '\r')
			{
				if(pos + 1 < str.length() && str[pos + 1] == '\n')
					str.erase(pos, 1);
				else
					str[pos] = '\n';
				++pos;
			}
			else
				++pos;
		}
	}
}

//=================================================================================================
void Replace(string& s, cstring in_chars, cstring out_chars)
{
	assert(in_chars && out_chars && strlen(in_chars) == strlen(out_chars));

	for(char& c : s)
	{
		cstring i_in_chars = in_chars,
			i_out_chars = out_chars;
		char i_char;
		while((i_char = *i_in_chars) != 0)
		{
			if(c == i_char)
				c = *i_out_chars;
			++i_in_chars;
			++i_out_chars;
		}
	}
}
