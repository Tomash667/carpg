#include "Pch.h"
#include "Base.h"

using namespace tokenizer;

cstring ALTER_START = "${";
cstring ALTER_END = "}$";
cstring WHITESPACE_SYMBOLS = " \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\"";
cstring WHITESPACE_SYMBOLS_DOT = " \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\".";
cstring SYMBOLS = ",./;'\\[]`<>?:|{}=~!@#$%^&*()+-";

//=================================================================================================
void Tokenizer::FromString(cstring _str)
{
	assert(_str);
	g_tmp_string = _str;
	str = &g_tmp_string;
	Reset();
}

//=================================================================================================
void Tokenizer::FromString(const string& _str)
{
	str = &_str;
	Reset();
}

//=================================================================================================
bool Tokenizer::FromFile(cstring path)
{
	assert(path);
	if(!LoadFileToString(path, g_tmp_string))
		return false;
	str = &g_tmp_string;
	filename = path;
	Reset();
	return true;
}

//=================================================================================================
void Tokenizer::FromTokenizer(const Tokenizer& t)
{
	str = t.str;
	normal_seek.pos = t.normal_seek.pos;
	normal_seek.line = t.normal_seek.line;
	normal_seek.charpos = t.normal_seek.charpos;
	normal_seek.item = t.normal_seek.item;
	normal_seek.token = t.normal_seek.token;
	normal_seek._int = t.normal_seek._int;
	flags = t.flags;
	normal_seek._float = t.normal_seek._float;
	normal_seek._char = t.normal_seek._char;
	normal_seek._uint = t.normal_seek._uint;

	if(normal_seek.token == T_KEYWORD)
	{
		// need to check keyword because keywords are not copied from other tokenizer, it may be item here
		CheckItemOrKeyword(normal_seek, normal_seek.item);
	}
}

//=================================================================================================
bool Tokenizer::DoNext(SeekData& s, bool return_eol)
{
	CheckSorting();

redo:
	if(s.token == T_EOF)
		return false;

	if(s.pos >= str->length())
	{
		s.token = T_EOF;
		return false;
	}

	s.start_pos = s.pos;

	// szukaj czegoœ
	uint pos2 = FindFirstNotOf(s, return_eol ? " \t" : " \t\n\r", s.pos);
	if(pos2 == string::npos)
	{
		// same spacje, entery, taby
		// to koniec pliku
		s.token = T_EOF;
		return false;
	}

	char c = str->at(pos2);

	if(c == '\r')
	{
		s.pos = pos2 + 1;
		if(s.pos < str->length() && str->at(s.pos) == '\n')
			++s.pos;
		s.token = T_EOL;
	}
	else if(c == '\n')
	{
		s.pos = pos2 + 1;
		s.token = T_EOL;
	}
	else if(c == '/')
	{
		char c2 = str->at(pos2 + 1);
		if(c2 == '/')
		{
			s.pos = FindFirstOf(s, "\n", pos2 + 1);
			if(s.pos == string::npos)
			{
				s.token = T_EOF;
				return false;
			}
			else
				goto redo;
		}
		else if(c2 == '*')
		{
			int prev_line = s.line;
			int prev_charpos = s.charpos;
			s.pos = FindFirstOfStr(s, "*/", pos2 + 1);
			if(s.pos == string::npos)
				formatter.Throw(Format("Not closed comment started at line %d, character %d.", prev_line + 1, prev_charpos + 1));
			goto redo;
		}
		else
		{
			++s.charpos;
			s.pos = pos2 + 1;
			s.token = T_SYMBOL;
			s._char = c;
			s.item = c;
		}
	}
	else if(c == '"')
	{
		// szukaj koñca ci¹gu znaków
		uint cp = s.charpos;
		s.pos = FindEndOfQuote(s, pos2 + 1);

		if(s.pos == string::npos || str->at(s.pos) != '"')
			formatter.Throw(Format("Not closed \" opened at %d.", cp + 1));

		if(IS_SET(flags, F_UNESCAPE))
			Unescape(*str, pos2 + 1, s.pos - pos2 - 1, s.item);
		else
			s.item = str->substr(pos2 + 1, s.pos - pos2 - 1);
		s.token = T_STRING;
		++s.pos;
	}
	else if(StringInString(str->c_str() + pos2, ALTER_START))
	{
		// alter string
		int len = strlen(ALTER_START);
		s.pos = pos2 + len;
		s.charpos += len;
		uint block_start = s.pos;
		bool ok = false;
		len = strlen(ALTER_END);

		for(; s.pos < str->length(); ++s.pos)
		{
			if(StringInString(str->c_str() + s.pos, ALTER_END))
			{
				s.item = str->substr(block_start, s.pos - block_start);
				s.token = T_STRING;
				s.pos += len;
				s.charpos += len;
				ok = true;
				break;
			}
			else if(str->at(s.pos) == '\n')
			{
				++s.line;
				s.charpos = 0;
			}
			else
				++s.charpos;
		}

		if(!ok)
			Throw("Missing closing alternate string '%s'.", ALTER_END);
	}
	else if(c == '-' && IS_SET(flags, F_JOIN_MINUS))
	{
		++s.charpos;
		s.pos = pos2 + 1;
		int old_pos = s.pos;
		int old_charpos = s.charpos;
		int old_line = s.line;
		// znajdŸ nastêpny znak
		pos2 = FindFirstNotOf(s, return_eol ? " \t" : " \t\n\r", s.pos);
		if(pos2 == string::npos)
		{
			// same spacje, entery, taby
			// to koniec pliku
			s.token = T_SYMBOL;
			s._char = c;
			s.item = c;
		}
		else
		{
			char c = str->at(pos2);
			if(c >= '0' && c <= '9')
				ParseNumber(s, pos2, true);
			else
			{
				// nie liczba, zwróc minus
				s.token = T_SYMBOL;
				s._char = '-';
				s.item = '-';
				s.pos = old_pos;
				s.charpos = old_charpos;
				s.line = old_line;
			}
		}
	}
	else if(strchr(SYMBOLS, c))
	{
		// symbol
		++s.charpos;
		s.pos = pos2 + 1;
		s.token = T_SYMBOL;
		s._char = c;
		s.item = c;
	}
	else if(c >= '0' && c <= '9')
	{
		// number
		if(c == '0' && str->at(pos2 + 1) == 'x')
		{
			// hex number
			s.pos = FindFirstOf(s, WHITESPACE_SYMBOLS_DOT, pos2);
			if(pos2 == string::npos)
			{
				s.pos = str->length();
				s.item = str->substr(pos2);
			}
			else
				s.item = str->substr(pos2, s.pos - pos2);

			uint num = 0;
			for(uint i = 2; i < s.item.length(); ++i)
			{
				c = tolower(s.item[i]);
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
					WARN(Format("Tokenizer: Broken hex number at %u:%u.", s.line + 1, s.charpos + 1));
					s.token = T_BROKEN_NUMBER;
					return true;
				}
			}
			s.token = T_INT;
			s._int = num;
			s._float = (float)num;
			s._uint = num;
		}
		else
			ParseNumber(s, pos2, false);
	}
	else
	{
		// find end of this item
		bool ignore_dot = false;
		if(IS_SET(flags, F_JOIN_DOT))
			ignore_dot = true;
		s.pos = FindFirstOf(s, ignore_dot ? WHITESPACE_SYMBOLS : WHITESPACE_SYMBOLS_DOT, pos2);
		if(pos2 == string::npos)
		{
			s.pos = str->length();
			s.item = str->substr(pos2);
		}
		else
			s.item = str->substr(pos2, s.pos - pos2);

		CheckItemOrKeyword(s, s.item);
	}

	return true;
}

//=================================================================================================
void Tokenizer::ParseNumber(SeekData& s, uint pos2, bool negative)
{
	s.item.clear();
	if(negative)
		s.item = "-";
	int have_dot = 0;
	/*
	0 - number
	1 - number.
	2 - number.number
	3 - number.number.
	*/

	for(uint i = pos2, len = str->length(); i < len; ++i, ++s.charpos)
	{
		char c = str->at(i);
		if(c >= '0' && c <= '9')
		{
			s.item += c;
			if(have_dot == 1)
				have_dot = 2;
		}
		else if(c == '.')
		{
			if(have_dot == 0)
			{
				have_dot = 1;
				s.item += c;
			}
			else
			{
				// second dot, end parsing
				s.pos = i;
				break;
			}
		}
		else if(strchr2(c, WHITESPACE_SYMBOLS) != 0)
		{
			// found symbol or whitespace, break
			s.pos = i;
			break;
		}
		else
		{
			if(have_dot == 0 || have_dot == 2)
			{
				// int item -> broken number
				// int . int item -> broken number
				// find end of item
				s.pos = FindFirstOf(s, WHITESPACE_SYMBOLS_DOT, i);
				if(s.pos == string::npos)
					s.item += str->substr(i);
				else
					s.item += str->substr(i, s.pos - i);
				s.token = T_BROKEN_NUMBER;
				return;
			}
			else if(have_dot == 1 || have_dot == 3)
			{
				// int dot item
				s.pos = i - 1;
				s.item.pop_back();
				break;
			}
		}
	}

	// parse number
	__int64 val;
	int type = TextHelper::ToNumber(s.item.c_str(), val, s._float);
	assert(type > 0);
	s._int = (int)val;
	if(s._int < 0)
		s._uint = 0;
	else
		s._uint = s._int;
	if(val > UINT_MAX)
	{
		WARN(Format("Tokenizer: Too big number %I64.", val));
		type = 0;
	}
	if(type == 2)
		s.token = T_FLOAT;
	else if(type == 1)
		s.token = T_INT;
	else
		s.token = T_BROKEN_NUMBER;
}

//=================================================================================================
void Tokenizer::SetFlags(int _flags)
{
	flags = _flags;
	if(IS_SET(flags, F_SEEK))
	{
		if(!seek)
			seek = new SeekData;
	}
	else
	{
		delete seek;
		seek = nullptr;
	}
}

//=================================================================================================
void Tokenizer::CheckItemOrKeyword(SeekData& s, const string& _item)
{
	Keyword k = { _item.c_str(), 0, 0 };
	auto end = keywords.end();
	auto it = std::lower_bound(keywords.begin(), end, k);
	if(it != end && _item == it->name)
	{
		// keyword
		s.token = T_KEYWORD;
		s.keyword.clear();
		if(it->enabled)
			s.keyword.push_back(&*it);
		if(IS_SET(flags, F_MULTI_KEYWORDS))
		{
			do
			{
				++it;
				if(it == end || _item != it->name)
					break;
				if(it->enabled)
					s.keyword.push_back(&*it);
			} while(true);
		}
		if(s.keyword.empty())
			s.token = T_ITEM;
	}
	else
	{
		// normal text, item
		s.token = T_ITEM;
	}
}

//=================================================================================================
bool Tokenizer::NextLine()
{
	if(IsEof())
		return false;

	if(normal_seek.pos >= str->length())
	{
		normal_seek.token = T_EOF;
		return false;
	}

	uint pos2 = FindFirstNotOf(normal_seek, " \t", normal_seek.pos);
	if(pos2 == string::npos)
	{
		normal_seek.pos = string::npos;
		normal_seek.token = T_EOF;
		return false;
	}

	uint pos3 = FindFirstOf(normal_seek, "\n\r", pos2 + 1);
	if(pos3 == string::npos)
		normal_seek.item = str->substr(pos2);
	else
		normal_seek.item = str->substr(pos2, pos3 - pos2);

	normal_seek.token = T_ITEM;
	normal_seek.pos = pos3;
	return !normal_seek.item.empty();
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
	assert(normal_seek.token == T_SYMBOL || normal_seek.token == T_COMPOUND_SYMBOL);
	char c = str->at(normal_seek.pos);
	if(c == symbol)
	{
		normal_seek.item += c;
		normal_seek._char = c;
		++normal_seek.charpos;
		++normal_seek.pos;
		normal_seek.token = T_COMPOUND_SYMBOL;
		return true;
	}
	else
		return false;
}

//=================================================================================================
uint Tokenizer::FindFirstNotOf(SeekData& s, cstring _str, uint _start)
{
	assert(_start < str->length());

	uint len = strlen(_str);
	char c;
	bool found;

	for(uint i = _start, end = str->length(); i<end; ++i)
	{
		c = str->at(i);
		found = false;

		for(uint j = 0; j<len; ++j)
		{
			if(c == _str[j])
			{
				found = true;
				break;
			}
		}

		if(!found)
			return i;

		if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindFirstOf(SeekData& s, cstring _str, uint _start)
{
	assert(_start < str->length());

	uint len = strlen(_str);
	char c;

	for(uint i = _start, end = str->length(); i<end; ++i)
	{
		c = str->at(i);

		for(uint j = 0; j<len; ++j)
		{
			if(c == _str[j])
				return i;
		}

		if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindFirstOfStr(SeekData& s, cstring _str, uint _start)
{
	assert(_start < str->length());

	for(uint i = _start, end = str->length(); i<end; ++i)
	{
		char c = str->at(i);
		if(c == _str[0])
		{
			cstring _s = _str;
			while(true)
			{
				++s.charpos;
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
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindEndOfQuote(SeekData& s, uint _start)
{
	assert(_start < str->length());

	for(uint i = _start, end = str->length(); i<end; ++i)
	{
		char c = str->at(i);

		if(c == '"')
		{
			if(i == _start || str->at(i - 1) != '\\')
				return i;
		}
		else if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
const Keyword* Tokenizer::FindKeyword(int _id, int _group) const
{
	for(vector<Keyword>::const_iterator it = keywords.begin(), end = keywords.end(); it != end; ++it)
	{
		if(it->id == _id && (it->group == _group || _group == EMPTY_GROUP))
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
const KeywordGroup* Tokenizer::FindKeywordGroup(int group) const
{
	for(const KeywordGroup& g : groups)
	{
		if(g.id == group)
			return &g;
	}

	return nullptr;
}

//=================================================================================================
void Tokenizer::AddKeywords(int group, std::initializer_list<KeywordToRegister> const & to_register, cstring group_name)
{
	for(const KeywordToRegister& k : to_register)
		AddKeyword(k.name, k.id, group);

	if(to_register.size() > 0)
		need_sorting = true;

	if(group_name)
		AddKeywordGroup(group_name, group);
}

//=================================================================================================
bool Tokenizer::RemoveKeyword(cstring name, int id, int group)
{
	Keyword k = { name, 0, 0 };
	auto end = keywords.end();
	auto it = std::lower_bound(keywords.begin(), end, k);
	if(it != end && strcmp(name, it->name) == 0)
	{
		if(it->id == id && it->group == group)
		{
			// found
			keywords.erase(it);
			return true;
		}

		// not found exact id/group, if multikeywords check next items
		if(IS_SET(flags, F_MULTI_KEYWORDS))
		{
			do
			{
				++it;
				if(it == end || strcmp(it->name, name) != 0)
					break;
				if(it->id == id && it->group == group)
				{
					keywords.erase(it);
					return true;
				}
			} while(true);
		}
	}

	return false;
}

//=================================================================================================
bool Tokenizer::RemoveKeyword(int id, int group)
{
	for(vector<Keyword>::iterator it = keywords.begin(), end = keywords.end(); it != end; ++it)
	{
		if(it->id == id && it->group == group)
		{
			keywords.erase(it);
			return true;
		}
	}

	return false;
}

//=================================================================================================
bool Tokenizer::RemoveKeywordGroup(int group)
{
	auto end = groups.end();
	KeywordGroup to_find = { nullptr, group };
	auto it = std::find(groups.begin(), end, to_find);
	if(it == end)
		return false;
	groups.erase(it);
	return true;
}

//=================================================================================================
void Tokenizer::EnableKeywordGroup(int group)
{
	for(Keyword& k : keywords)
	{
		if(k.group == group)
			k.enabled = true;
	}
}

//=================================================================================================
void Tokenizer::DisableKeywordGroup(int group)
{
	for(Keyword& k : keywords)
	{
		if(k.group == group)
			k.enabled = false;
	}
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
		return Format("%s '%s'", name, (cstring)what);
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
			const KeywordGroup* group = (what2 ? FindKeywordGroup(*what2) : nullptr);
			if(keyword)
			{
				if(group)
					return Format("%s '%s'(%d) from group '%s'(%d)", name, keyword->name, keyword->id, group->name, group->id);
				else if(what2)
					return Format("%s '%s'(%d) from group %d", name, keyword->name, keyword->id, *what2);
				else
					return Format("%s '%s'(%d)", name, keyword->name, keyword->id);
			}
			else
			{
				if(group)
					return Format("missing %s %d from group '%s'(%d)", name, *what, group->name, group->id);
				else if(what2)
					return Format("missing %s %d from group %d", name, *what, *what2);
				else
					return Format("missing %s %d", name, *what);
			}
		}
		break;
	case T_KEYWORD_GROUP:
		{
			const KeywordGroup* group = FindKeywordGroup(*what);
			if(group)
				return Format("%s '%s'(%d)", name, group->name, group->id);
			else
				return Format("%s %d", name, *what);
		}
	case T_SYMBOLS_LIST:
		return Format("%s \"%s\"", name, (cstring)what);
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
			formatter.Add(T_KEYWORD_GROUP, &g);
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

//=================================================================================================
const string& Tokenizer::GetBlock(char open, char close)
{
	AssertSymbol(open);
	int opened = 1;
	uint block_start = normal_seek.pos - 1;
	while(Next())
	{
		if(IsSymbol(open))
			++opened;
		else if(IsSymbol(close))
		{
			--opened;
			if(opened == 0)
			{
				normal_seek.item = str->substr(block_start, normal_seek.pos - block_start);
				return normal_seek.item;
			}
		}
	}

	int symbol = (int)open;
	Unexpected(T_SYMBOL, &symbol);
}

//=================================================================================================
bool Tokenizer::IsSymbol(cstring s, char* c) const
{
	assert(s);
	if(!IsSymbol())
		return false;
	char c2, symbol = GetSymbol();
	while((c2 = *s) != 0)
	{
		if(c2 == symbol)
		{
			if(c)
				*c = symbol;
			return true;
		}
		++s;
	}
	return false;
}

//=================================================================================================
char Tokenizer::MustGetSymbol(cstring symbols) const
{
	assert(symbols);
	char c;
	if(IsSymbol(symbols, &c))
		return c;
	Unexpected(T_SYMBOLS_LIST, (int*)symbols);
}

//=================================================================================================
bool Tokenizer::SeekStart(bool return_eol)
{
	assert(seek);
	seek->token = normal_seek.token;
	seek->pos = normal_seek.pos;
	seek->line = normal_seek.line;
	seek->charpos = normal_seek.charpos;
	return SeekNext(return_eol);
}

//=================================================================================================
cstring Tokenizer::GetTokenName(TOKEN _tt)
{
	switch(_tt)
	{
	case T_NONE:
		return "none";
	case T_EOF:
		return "end of file";
	case T_EOL:
		return "end of line";
	case T_ITEM:
		return "item";
	case T_STRING:
		return "string";
	case T_SYMBOL:
		return "symbol";
	case T_INT:
		return "integer";
	case T_FLOAT:
		return "float";
	case T_KEYWORD:
		return "keyword";
	case T_BROKEN_NUMBER:
		return "broken number";
	case T_KEYWORD_GROUP:
		return "keyword group";
	case T_NUMBER:
		return "number";
	case T_TEXT:
		return "text";
	case T_BOOL:
		return "bool";
	case T_SYMBOLS_LIST:
		return "symbols list";
	case T_COMPOUND_SYMBOL:
		return "compound symbol";
	default:
		assert(0);
		return "unknown";
	}
}

//=================================================================================================
cstring Tokenizer::GetTokenValue(const SeekData& s) const
{
	cstring name = GetTokenName(s.token);

	switch(s.token)
	{
	case T_ITEM:
	case T_STRING:
	case T_COMPOUND_SYMBOL:
	case T_BROKEN_NUMBER:
		return Format("%s '%s'", name, s.item.c_str());
	case T_SYMBOL:
		return Format("%s '%c'", name, s._char);
	case T_INT:
		return Format("%s %d", name, s._int);
	case T_FLOAT:
		return Format("%s %g", name, s._float);
	case T_KEYWORD:
		if(s.keyword.size() == 1)
		{
			Keyword& keyword = *s.keyword[0];
			const KeywordGroup* group = FindKeywordGroup(keyword.group);
			if(group)
				return Format("%s '%s'(%d) from group '%s'(%d)", name, keyword.name, keyword.id, group->name, group->id);
			else if(keyword.group != EMPTY_GROUP)
				return Format("%s '%s'(%d) from group %d", name, keyword.name, keyword.id, keyword.group);
			else
				return Format("%s '%s'(%d)", name, keyword.name, keyword.id);
		}
		else
		{
			LocalString str = Format("keyword '%s' in multiple groups {", s.item.c_str());
			bool first = true;
			for(Keyword* k : s.keyword)
			{
				if(first)
					first = false;
				else
					str += ", ";
				const KeywordGroup* group = FindKeywordGroup(k->group);
				if(group)
					str += Format("[%d,'%s'(%d)]", k->id, group->name, group->id);
				else
					str += Format("[%d:%d]", k->id, k->group);
			}
			str += "}";
			return str.c_str();
		}
	case T_NONE:
	case T_EOF:
	case T_EOL:
	default:
		return name;
	}
}

//=================================================================================================
int Tokenizer::IsKeywordGroup(std::initializer_list<int> const & groups) const
{
	if(!IsKeyword())
		return MISSING_GROUP;
	for(int group : groups)
	{
		for(Keyword* k : normal_seek.keyword)
		{
			if(k->group == group)
				return group;
		}
	}
	return MISSING_GROUP;
}

Pos Tokenizer::GetPos()
{
	Pos p;
	p.line = normal_seek.line + 1;
	p.charpos = normal_seek.charpos + 1;
	p.pos = normal_seek.start_pos;
	return p;
}

void Tokenizer::MoveTo(const Pos& p)
{
	normal_seek.line = p.line - 1;
	normal_seek.charpos = p.charpos - 1;
	normal_seek.pos = p.pos;
	DoNext(normal_seek, false);
}

bool Tokenizer::MoveToClosingSymbol(char start, char end)
{
	int depth = 1;
	if(!IsSymbol(start))
		return false;

	while(Next())
	{
		if(IsSymbol(start))
			++depth;
		else if(IsSymbol(end))
		{
			if(--depth == 0)
				return true;
		}
	}

	return false;
}
