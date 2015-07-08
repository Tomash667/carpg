#include "Pch.h"
#include "Base.h"

//=================================================================================================
bool Tokenizer::Next(bool return_eol)
{
redo:
	if(IsEof())
		return false;

	if(pos >= str->length())
	{
		type = T_EOF;
		return false;
	}

	// szukaj czegoœ
	uint pos2 = FindFirstNotOf(return_eol ? " \t" : " \t\n\r", pos);
	if(pos2 == string::npos)
	{
		// same spacje, entery, taby
		// to koniec pliku
		type = T_EOF;
		return false;
	}

	cstring symbols = ",./;'\\[]`<>?:|{}=~!@#$%^&*()+-";
	char c = str->at(pos2);

	if(c == '\r')
	{
		pos = pos2+1;
		if(pos < str->length() && str->at(pos) == '\n')
			++pos;
		type = T_EOL;
	}
	else if(c == '\n')
	{
		pos = pos2+1;
		type = T_EOL;
	}
	if(c == '/')
	{
		char c2 = str->at(pos2+1);
		if(c2 == '/')
		{
			pos = FindFirstOf("\n", pos2+1);
			if(pos == string::npos)
			{
				type = T_EOF;
				return false;
			}
			else
				goto redo;
		}
		else if(c2 == '*')
		{
			int prev_line = line;
			int prev_charpos = charpos;
			pos = FindFirstOfStr("*/", pos2+1);
			if(pos == string::npos)
				throw Format("(%d,%d) Not closed comment started at line %d, character %d!", line+1, charpos+1, prev_line+1, prev_charpos+1);
			goto redo;
		}
		else
		{
			++charpos;
			pos = pos2+1;
			type = T_SYMBOL;
			_char = c;
			token = c;
		}
	}
	else if(c == '"')
	{
		// szukaj koñca ci¹gu znaków
		uint cp = charpos;
		pos = FindEndOfQuote(pos2+1);

		if(pos == string::npos || str->at(pos) != '"')
			throw Format("(%d,%d) Not closed \" opened at %d!", line+1, charpos+1, cp+1);

		if(IS_SET(flags, F_UNESCAPE))
			Unescape(*str, pos2 + 1, pos - pos2 - 1, token);
		else
			token = str->substr(pos2 + 1, pos - pos2 - 1);
		type = T_STRING;
		++pos;
	}
	else if(c == '-' && IS_SET(flags, F_JOIN_MINUS))
	{
		++charpos;
		pos = pos2+1;
		int old_pos = pos;
		int old_charpos = charpos;
		int old_line = line;
		// znajdŸ nastêpny znak
		pos2 = FindFirstNotOf(return_eol ? " \t" : " \t\n\r", pos);
		if(pos2 == string::npos)
		{
			// same spacje, entery, taby
			// to koniec pliku
			type = T_SYMBOL;
			_char = c;
			token = c;
			return true;
		}

		char c = str->at(pos2);
		if(c >= '0' && c <= '9')
		{
			// liczba z minusem z przodu
			pos = FindFirstOf(" \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\"", pos2);
			if(pos2 == string::npos)
			{
				pos = str->length();
				token = str->substr(pos2);
			}
			else
				token = str->substr(pos2, pos-pos2);

			// czy to liczba?
			__int64 val;
			int co = StringToNumber(token.c_str(), val, _float);
			_int = -(int)val;
			_uint = 0;
			_float = -_float;
			if(val > UINT_MAX)
				WARN(Format("Tokenizer: Too big number %I64, stored as int(%d) and uint(%u).", val, _int, _uint));
			if(co == 2)
				type = T_FLOAT;
			else if(co == 1)
				type = T_INT;
			else
				type = T_ITEM;
			return true;
		}
		else
		{
			// nie liczba, zwróc minus
			type = T_SYMBOL;
			_char = '-';
			token = '-';
			pos = old_pos;
			charpos = old_charpos;
			line = old_line;
			return true;
		}
	}
	else if(strchr(symbols, c))
	{
		// symbol
		++charpos;
		pos = pos2+1;
		type = T_SYMBOL;
		_char = c;
		token = c;
	}
	else
	{
		// coœ znaleziono, znajdŸ koniec tego czegoœæ
		bool ignore_dot = false;
		if((c >= '0' && c <= '9') || IS_SET(flags, F_JOIN_DOT))
			ignore_dot = true;
		pos = FindFirstOf(ignore_dot ? " \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\"" : " \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\".", pos2);
		if(pos2 == string::npos)
		{
			pos = str->length();
			token = str->substr(pos2);
		}
		else
			token = str->substr(pos2, pos-pos2);

		// czy to liczba?
		if(c >= '0' && c <= '9')
		{
			__int64 val;
			int co = StringToNumber(token.c_str(), val, _float);
			_int = (int)val;
			_uint = (uint)val;
			if(val > UINT_MAX)
				WARN(Format("Tokenizer: Too big number %I64, stored as int(%d) and uint(%u).", val, _int, _uint));
			if(co == 2)
				type = T_FLOAT;
			else if(co == 1)
				type = T_INT;
			else
				type = T_ITEM;
		}
		else
		{
			// czy to s³owo kluczowe
			for(uint i=0; i<keywords.size(); ++i)
			{
				if(token == keywords[i].name)
				{
					type = T_KEYWORD;
					_int = i;
					return true;
				}
			}

			// zwyk³y tekst
			type = T_ITEM;
		}
	}

	return true;
}

//=================================================================================================
bool Tokenizer::NextLine()
{
	if(IsEof())
		return false;

	if(pos >= str->length())
	{
		type = T_EOF;
		return false;
	}

	// szukaj czegoœ
	uint pos2 = FindFirstOf("\n\r", pos);
	if(pos2 == string::npos)
		token = str->substr(pos);
	else
		token = str->substr(pos, pos2-pos);
	
	type = T_ITEM;
	pos = pos2;
	return !token.empty();
}

//=================================================================================================
bool Tokenizer::PeekSymbol(char symbol)
{
	char c = str->at(pos);
	if(c == symbol)
	{
		token = c;
		_char = c;
		++charpos;
		++pos;
		type = T_SYMBOL;
		return true;
	}
	else
		return false;
}

//=================================================================================================
uint Tokenizer::FindFirstNotOf(cstring _str, uint _start)
{
	assert(_start < str->length());

	uint ile = strlen(_str);
	char c;
	bool jest;

	for(uint i=_start, end = str->length(); i<end; ++i)
	{
		c = str->at(i);
		jest = false;

		for(uint j=0; j<ile; ++j)
		{
			if(c == _str[j])
			{
				jest = true;
				break;
			}
		}

		if(!jest)
			return i;

		if(c == '\n')
		{
			++line;
			charpos = 0;
		}
		else
			++charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindFirstOf(cstring _str, uint _start)
{
	assert(_start < str->length());

	uint ile = strlen(_str);
	char c;

	for(uint i=_start, end = str->length(); i<end; ++i)
	{
		c = str->at(i);

		for(uint j=0; j<ile; ++j)
		{
			if(c == _str[j])
				return i;
		}

		if(c == '\n')
		{
			++line;
			charpos = 0;
		}
		else
			++charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindFirstOfStr(cstring _str, uint _start)
{
	assert(_start < str->length());

	for(uint i=_start, end = str->length(); i<end; ++i)
	{
		char c = str->at(i);
		if(c == _str[0])
		{
			cstring _s = _str;
			while(true)
			{
				++charpos;
				++i;
				++_s;
				if(*_s == NULL)
					return i;
				if(i == end)
					return string::npos;
				if(*_s != str->at(i))
					break;
			}
		}
		else if(c == '\n')
		{
			++line;
			charpos = 0;
		}
		else
			++charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindEndOfQuote(uint _start)
{
	assert(_start < str->length());

	for(uint i=_start, end = str->length(); i<end; ++i)
	{
		char c = str->at(i);

		if(c == '"')
		{
			if(i == _start || str->at(i-1) != '\\')
				return i;
		}
		else if(c == '\n')
		{
			++line;
			charpos = 0;
		}
		else
			++charpos;
	}

	return string::npos;
}

//=================================================================================================
void Tokenizer::Throw(cstring msg)
{
	throw Format("(%d:%d) %s", line+1, charpos+1, msg);
}

//=================================================================================================
void Tokenizer::Unexpected()
{
	throw Format("(%d:%d) Unexpected %s!", line+1, charpos+1, GetTokenValue());
}

//=================================================================================================
void Tokenizer::Unexpected(int count, ...)
{
	va_list args;
	va_start(args, count);

	LocalString str = Format("(%d:%d) Expecting ", line+1, charpos+1);

	int value = -1, value2 = -1;

	for(int i=0; i<count; ++i)
	{
		TOKEN_TYPE tt = va_arg(args, TOKEN_TYPE);
		
		if(tt == T_SPECIFIC_SYMBOL || tt == T_SPECIFIC_KEYWORD_GROUP)
		{
			value = va_arg(args, int);
			value2 = -1;
		}
		else if(tt == T_SPECIFIC_KEYWORD)
		{
			value = va_arg(args, int);
			value2 = va_arg(args, int);
		}
		str += GetTokenName2(tt, value, value2);

		if(i < count-2)
			str += ", ";
		else if(i == count-2)
			str += " or ";
	}

	str += Format(", found %s!", GetTokenValue());

	va_end(args);

	token = str;
	throw token.c_str();
}

//=================================================================================================
const Tokenizer::Keyword* Tokenizer::FindKeyword(int _id, int _group) const
{
	for(vector<Keyword>::const_iterator it = keywords.begin(), end = keywords.end(); it != end; ++it)
	{
		if(it->id == _id && (it->group == _group || _group == -1))
			return &*it;
	}

	return NULL;
}
