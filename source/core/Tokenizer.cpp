#include "Pch.h"
#include "Base.h"

//=================================================================================================
bool Tokenizer::Next(bool return_eol)
{
	CheckSorting();

redo:
	if(IsEof())
		return false;

	if(pos >= str->length())
	{
		token = T_EOF;
		return false;
	}

	// szukaj czegoœ
	uint pos2 = FindFirstNotOf(return_eol ? " \t" : " \t\n\r", pos);
	if(pos2 == string::npos)
	{
		// same spacje, entery, taby
		// to koniec pliku
		token = T_EOF;
		return false;
	}

	cstring symbols = ",./;'\\[]`<>?:|{}=~!@#$%^&*()+-";
	char c = str->at(pos2);

	if(c == '\r')
	{
		pos = pos2+1;
		if(pos < str->length() && str->at(pos) == '\n')
			++pos;
		token = T_EOL;
	}
	else if(c == '\n')
	{
		pos = pos2+1;
		token = T_EOL;
	}
	if(c == '/')
	{
		char c2 = str->at(pos2+1);
		if(c2 == '/')
		{
			pos = FindFirstOf("\n", pos2+1);
			if(pos == string::npos)
			{
				token = T_EOF;
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
				formatter.Throw(Format("Not closed comment started at line %d, character %d.", prev_line + 1, prev_charpos + 1));
			goto redo;
		}
		else
		{
			++charpos;
			pos = pos2+1;
			token = T_SYMBOL;
			_char = c;
			item = c;
		}
	}
	else if(c == '"')
	{
		// szukaj koñca ci¹gu znaków
		uint cp = charpos;
		pos = FindEndOfQuote(pos2+1);

		if(pos == string::npos || str->at(pos) != '"')
			formatter.Throw(Format("Not closed \" opened at %d.", cp + 1));

		if(IS_SET(flags, F_UNESCAPE))
			Unescape(*str, pos2 + 1, pos - pos2 - 1, item);
		else
			item = str->substr(pos2 + 1, pos - pos2 - 1);
		token = T_STRING;
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
			token = T_SYMBOL;
			_char = c;
			item = c;
		}
		else
		{
			char c = str->at(pos2);
			if(c >= '0' && c <= '9')
			{
				// liczba z minusem z przodu
				pos = FindFirstOf(" \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\"", pos2);
				if(pos2 == string::npos)
				{
					pos = str->length();
					item = str->substr(pos2);
				}
				else
					item = str->substr(pos2, pos - pos2);

				// czy to liczba?
				__int64 val;
				int co = TextHelper::ToNumber(item.c_str(), val, _float);
				_int = -(int)val;
				_uint = 0;
				_float = -_float;
				if(val > UINT_MAX)
					WARN(Format("Tokenizer: Too big number %I64, stored as int(%d) and uint(%u).", val, _int, _uint));
				if(co == 2)
					token = T_FLOAT;
				else if(co == 1)
					token = T_INT;
				else
					token = T_ITEM;
			}
			else
			{
				// nie liczba, zwróc minus
				token = T_SYMBOL;
				_char = '-';
				item = '-';
				pos = old_pos;
				charpos = old_charpos;
				line = old_line;
			}
		}
	}
	else if(strchr(symbols, c))
	{
		// symbol
		++charpos;
		pos = pos2+1;
		token = T_SYMBOL;
		_char = c;
		item = c;
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
			item = str->substr(pos2);
		}
		else
			item = str->substr(pos2, pos-pos2);

		// czy to liczba?
		if(c >= '0' && c <= '9')
		{
			if(item.length() > 2 && tolower(item[1]) == 'x')
			{
				// hex number
				uint num = 0;
				for(uint i = 2; i < item.length(); ++i)
				{
					c = tolower(item[i]);
					if(c >= '0' && c <= '9')
					{
						num <<= 4;
						num += c - '0';
					}
					else if(c >= 'a' && c <= 'f')
					{
						num <<= 4;
						num += c - 'a' + 10;
					}
					else
					{
						WARN(Format("Tokenizer: Broken hex number at %u:%u.", line + 1, charpos + 1));
						num = 0;
					}
				}
				token = T_INT;
				_int = num;
				_float = (float)num;
				_uint = num;
			}
			else
			{
				__int64 val;
				int co = TextHelper::ToNumber(item.c_str(), val, _float);
				_int = (int)val;
				_uint = (uint)val;
				if(val > UINT_MAX)
					WARN(Format("Tokenizer: Too big number %I64, stored as int(%d) and uint(%u).", val, _int, _uint));
				if(co == 2)
					token = T_FLOAT;
				else if(co == 1)
					token = T_INT;
				else
					token = T_ITEM;
			}
		}
		else
		{
			Keyword k = { item.c_str(), 0, 0 };
			auto it = std::lower_bound(keywords.begin(), keywords.end(), k);
			if(it != keywords.end() && item == it->name)
			{
				// keyword
				token = T_KEYWORD;
				keyword.clear();
				keyword.push_back(&*it);
				if(IS_SET(flags, F_MULTI_KEYWORDS))
				{
					do
					{
						++it;
						if(it == keywords.end() || item != it->name)
							break;
						keyword.push_back(&*it);
					} while(true);
				}
			}
			else
			{
				// normal text, item
				token = T_ITEM;
			}
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
		token = T_EOF;
		return false;
	}

	uint pos2 = FindFirstNotOf(" \t", pos);
	if(pos2 == string::npos)
	{
		pos = string::npos;
		token = T_EOF;
		return false;
	}

	uint pos3 = FindFirstOf("\n\r", pos2+1);
	if(pos3 == string::npos)
		item = str->substr(pos2);
	else
		item = str->substr(pos2, pos3-pos2);
	
	token = T_ITEM;
	pos = pos3;
	return !item.empty();
}

//=================================================================================================
bool Tokenizer::SkipTo(SkipToFunc f)
{
	while(true)
	{
		if(!Next())
			return false;

		if(f(*this))
			return true;
	}
}

//=================================================================================================
bool Tokenizer::SkipToKeywordGroup(int group)
{
	while(true)
	{
		if(!Next())
			return false;

		if(IsKeywordGroup(group))
			return true;
	}
}

//=================================================================================================
bool Tokenizer::PeekSymbol(char symbol)
{
	char c = str->at(pos);
	if(c == symbol)
	{
		item = c;
		_char = c;
		++charpos;
		++pos;
		token = T_SYMBOL;
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
				if(*_s == 0)
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
const Tokenizer::Keyword* Tokenizer::FindKeyword(int _id, int _group) const
{
	for(vector<Keyword>::const_iterator it = keywords.begin(), end = keywords.end(); it != end; ++it)
	{
		if(it->id == _id && (it->group == _group || _group == EMPTY_GROUP))
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
void Tokenizer::AddKeywords(int group, std::initializer_list<KeywordToRegister> const & to_register)
{
	for(const KeywordToRegister& k : to_register)
		AddKeyword(k.name, k.id, group);

	if(to_register.size() > 0)
		need_sorting = true;
}

//=================================================================================================
cstring Tokenizer::FormatToken(TOKEN token, int* what, int* what2)
{
	cstring name = GetTokenName(token);
	if(what == nullptr)
		return name;

	switch(token)
	{
	case T_ITEM:
	case T_STRING:
	case T_TEXT:
		return Format("%s (%s)", name, (cstring)what);
	case T_SYMBOL:
		return Format("%s '%c'", name, *(char*)what);
	case T_INT:
	case T_NUMBER:
	case T_BOOL:
		return Format("%s %d", name, *what);
	case T_FLOAT:
		return Format("%s %g", name, *(float*)what);
	case T_KEYWORD:
		{
			const Keyword* keyword = FindKeyword(*what, what2 ? *what2 : EMPTY_GROUP);
			if(keyword)
				return Format("%s (%d,%d:%s)", name, keyword->id, keyword->group, keyword->name);
			else if(what2)
				return Format("missing %s (%d,%d)", name, *what, *what2);
			else
				return Format("missing %s (%d)", name, *what);
		}
		break;
	case T_KEYWORD_GROUP:
		return Format("%s %d", name, *what);
	default:
		return "missing";
	}
}

//=================================================================================================
void Tokenizer::CheckSorting()
{
	if(!need_sorting)
		return;

	need_sorting = false;
	std::sort(keywords.begin(), keywords.end());

	if(!IS_SET(flags, F_MULTI_KEYWORDS))
	{
		assert(CheckMultiKeywords());
	}
}

//=================================================================================================
bool Tokenizer::CheckMultiKeywords()
{
	int errors = 0;

	Keyword* prev = &keywords[0];
	for(uint i = 1; i < keywords.size(); ++i)
	{
		if(strcmp(keywords[i].name, prev->name) == 0)
		{
			++errors;
			ERROR(Format("Keyword '%s' multiple definitions (%d,%d) and (%d,%d).", prev->name, prev->id, prev->group,
				keywords[i].id, keywords[i].group));
		}
		prev = &keywords[i];
	}
	
	if(errors > 0)
	{
		ERROR(Format("Multiple keywords %d with same id. Use F_MULTI_KEYWORDS or fix that.", errors));
		return false;
	}
	else
		return true;
}

//=================================================================================================
int ReadFlags(Tokenizer& t, int group)
{
	int flags = 0;

	if(t.IsSymbol('{'))
	{
		t.Next();

		do
		{
			flags |= t.MustGetKeywordId(group);
			t.Next();
		} while(!t.IsSymbol('}'));
	}
	else
		flags = t.MustGetKeywordId(group);

	return flags;
}

//=================================================================================================
void ReadFlags(Tokenizer& t, std::initializer_list<FlagGroup> const & flags, bool clear)
{
	if(clear)
	{
		for(FlagGroup const & f : flags)
			*f.flags = 0;
	}

	bool unexpected = false;

	if(t.IsSymbol('{'))
	{
		t.Next();

		do
		{
			bool found = false;

			for(FlagGroup const & f : flags)
			{
				if(t.IsKeywordGroup(f.group))
				{
					*f.flags |= t.GetKeywordId(f.group);
					found = true;
					break;
				}
			}

			if(!found)
			{
				unexpected = true;
				break;
			}

			t.Next();
		} while(!t.IsSymbol('}'));
	}
	else
	{
		bool found = false;

		for(FlagGroup const & f : flags)
		{
			if(t.IsKeywordGroup(f.group))
			{
				*f.flags |= t.GetKeywordId(f.group);
				found = true;
				break;
			}
		}

		if(!found)
			unexpected = true;
	}

	if(unexpected)
	{
		auto& formatter = t.StartUnexpected();

		for(FlagGroup const & f : flags)
		{
			int g = f.group;
			formatter.Add(Tokenizer::T_KEYWORD_GROUP, &g);
		}

		formatter.Throw();
	}
}

//=================================================================================================
void Tokenizer::Parse(INT2& i)
{
	if(IsSymbol('{'))
	{
		Next();
		i.x = MustGetInt();
		Next();
		i.y = MustGetInt();
		Next();
		AssertSymbol('}');
		Next();
	}
	else
	{
		i.x = i.y = MustGetInt();
		Next();
	}
}

//=================================================================================================
void Tokenizer::Parse(IBOX2D& b)
{
	AssertSymbol('{');
	Next();
	b.p1.x = MustGetInt();
	Next();
	b.p1.y = MustGetInt();
	Next();
	b.p2.x = MustGetInt();
	Next();
	b.p2.y = MustGetInt();
	Next();
	AssertSymbol('}');
	Next();
}

//=================================================================================================
#ifndef NO_DIRECT_X
void Tokenizer::Parse(VEC2& v)
{
	if(IsSymbol('{'))
	{
		Next();
		v.x = MustGetNumberFloat();
		Next();
		v.y = MustGetNumberFloat();
		Next();
		AssertSymbol('}');
		Next();
	}
	else
	{
		v.x = v.y = MustGetNumberFloat();
		Next();
	}
}
#endif
