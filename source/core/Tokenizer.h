#pragma once

//-----------------------------------------------------------------------------
// Tokenizer
// todo:
// - "costam \" blabla"
// - 'a', ' b'
// - F_JOIN_MINUS
//-----------------------------------------------------------------------------
// U¿ywa g_tmp_string gdy poda siê cstring w konstruktorze
//-----------------------------------------------------------------------------
struct Tokenizer
{
	enum TOKEN_TYPE
	{
		T_NONE,
		T_EOF,
		T_EOL,
		T_ITEM,
		T_STRING,
		T_SYMBOL,
		T_INT,
		T_FLOAT,
		T_KEYWORD,

		// te typy s¹ u¿ywane w Unexpected
		T_SPECIFIC_SYMBOL,
		T_SPECIFIC_KEYWORD,
		T_SPECIFIC_KEYWORD_GROUP
	};

	struct Keyword
	{
		cstring name;
		int id, group;
	};

	enum FLAGS
	{
		// bez tego jest symbol -, potem liczba
		// z ujemna liczba
		F_JOIN_MINUS = 1<<0, // nie zrobione
		F_JOIN_DOT = 1<<1, // ³¹czy . po tekœcie (np log.txt - zwraca jeden item)
	};

	Tokenizer(int _flags=0) : flags(_flags)
	{
		Reset();
	}

	inline void Reset()
	{
		type = T_NONE;
		pos = 0;
		line = 0;
		charpos = 0;
	}

	inline void FromString(cstring _str)
	{
		assert(_str);
		g_tmp_string = _str;
		str = &g_tmp_string;
		Reset();
	}

	inline void FromString(const string& _str)
	{
		str = &_str;
		Reset();
	}

	inline bool FromFile(cstring path)
	{
		assert(path);
		if(!LoadFileToString(path, g_tmp_string))
			return false;
		str = &g_tmp_string;
		Reset();
		return true;
	}

	bool Next(bool return_eol=false);
	bool NextLine();
	void Throw(cstring msg);
	void Unexpected();
	void Unexpected(int count, ...);
	const Keyword* FindKeyword(int id, int group=-1) const;

	inline void AddKeyword(cstring _keyword, int _id, int _group=0)
	{
		assert(_keyword);
		Keyword& k = Add1(keywords);
		k.name = _keyword;
		k.id = _id;
		k.group = _group;
	}

	inline bool IsToken(TOKEN_TYPE _tt) const { return type == _tt; }
	inline bool IsEof() const { return IsToken(T_EOF); }
	inline bool IsEol() const { return IsToken(T_EOL); }
	inline bool IsItem() const { return IsToken(T_ITEM); }
	inline bool IsString() const { return IsToken(T_STRING); }
	inline bool IsSymbol() const { return IsToken(T_SYMBOL); }
	inline bool IsSymbol(char c) const { return IsSymbol() && GetSymbol() == c; }
	inline bool IsInt() const { return IsToken(T_INT); }
	inline bool IsFloat() const { return IsToken(T_FLOAT); }
	inline bool IsNumber() const { return IsToken(T_INT) || IsToken(T_FLOAT); }
	inline bool IsKeyword() const { return IsToken(T_KEYWORD); }
	inline bool IsKeywordGroup(int group) const { return IsKeyword() && GetKeywordGroup() == group; }

	inline cstring GetTokenName(TOKEN_TYPE _tt) const
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
		default:
			assert(0);
			return "unknown";
		}
	}

	inline cstring GetTokenName2(TOKEN_TYPE _tt, int _value, int _value2) const
	{
		switch(_tt)
		{
		case T_SPECIFIC_SYMBOL:
			return Format("symbol '%c'", (char)_value);
		case T_SPECIFIC_KEYWORD:
			return Format("keyword '%s'", FindKeyword(_value, _value2)->name);
		case T_SPECIFIC_KEYWORD_GROUP:
			return Format("keyword group %d", _value);
		default:
			return GetTokenName(_tt);
		}
	}

	inline cstring GetTokenValue() const
	{
		switch(type)
		{
		case T_ITEM:
		case T_STRING:
			return Format("%s (%s)", GetTokenName(type), token.c_str());
		case T_SYMBOL:
			return Format("%s '%c'", GetTokenName(type), _char);
		case T_INT:
			return Format("%s %d", GetTokenName(type), _int);
		case T_FLOAT:
			return Format("%s %g", GetTokenName(type), _float);
		case T_KEYWORD:
			return Format("%s (%d,%d:%s)", GetTokenName(type), keywords[_int].id, keywords[_int].group, token.c_str());
		case T_NONE:
		case T_EOF:
		case T_EOL:
		default:
			return GetTokenName(type);
		}
	}

	inline void AssertToken(TOKEN_TYPE _tt) const
	{
		if(type != _tt)
			throw Format("(%d:%d) Expecting %s, found %s!", line+1, charpos+1, GetTokenName(_tt), GetTokenValue());
	}
	inline void AssertEof() const { AssertToken(T_EOF); }
	inline void AssertItem() const { AssertToken(T_ITEM); }
	inline void AssertString() const { AssertToken(T_STRING); }
	inline void AssertSymbol() const { AssertToken(T_SYMBOL); }
	inline void AssertInt() const { AssertToken(T_INT); }
	inline void AssertFloat() const { AssertToken(T_FLOAT); }
	inline void AssertNumber() const
	{
		if(type != T_INT && type != T_FLOAT)
			throw Format("(%d:%d) Expecting number, found %s!", line+1, charpos+1, GetTokenValue());
	}
	inline void AssertKeyword() const { AssertToken(T_KEYWORD); }
	inline void AssertKeyword(int id) const
	{
		if(type != T_KEYWORD || GetKeywordId() != id)
			throw Format("(%d:%d) Expecting keyword %d, found %s!", line+1, charpos+1, id, GetTokenValue());
	}
	inline void AssertKeywordGroup(int group) const
	{
		if(type != T_KEYWORD || GetKeywordGroup() != group)
			throw Format("(%d:%d) Expecting keyword from grup %d, found %s!", line+1, charpos+1, group, GetTokenValue());
	}
	inline void AssertSymbol(char c) const
	{
		if(type != T_SYMBOL || GetSymbol() != c)
			throw Format("(%d:%d) Expecting symbol '%c', found %s.", line+1, charpos+1, c, GetTokenValue());
	}
	inline void AssertText() const
	{
		if(type != T_ITEM && type != T_STRING && type != T_KEYWORD)
			throw Format("(%d:%d) Expecting text, found %s.", line+1, charpos+1, GetTokenValue());
	}
	inline void AssertBool() const
	{
		if(type != T_INT || (_int != 0 && _int != 1))
			throw Format("(%d:%d) Expecting bool, found %s.", line+1, charpos+1, GetTokenValue());
	}

	inline TOKEN_TYPE GetTokenType() const
	{
		return type;
	}
	inline cstring GetTokenName() const
	{
		return GetTokenName(GetTokenType());
	}
	inline const string& GetToken() const
	{
		return token;
	}
	inline const string& GetItem() const
	{
		assert(IsItem());
		return token;
	}
	inline const string& GetString() const
	{
		assert(IsString());
		return token;
	}
	inline char GetSymbol() const
	{
		assert(IsSymbol());
		return _char;
	}
	inline int GetInt() const
	{
		assert(IsNumber());
		return _int;
	}
	inline uint GetUint() const
	{
		assert(IsNumber());
		return _uint;
	}
	inline float GetFloat() const
	{
		assert(IsNumber());
		return _float;
	}
	inline uint GetLine() const { return line; }
	inline uint GetCharPos() const { return charpos; }
	inline const string& GetKeyword() const
	{
		assert(IsKeyword());
		return token;
	}
	inline int GetKeywordId() const
	{
		assert(IsKeyword());
		return keywords[_int].id;
	}
	inline int GetKeywordGroup() const
	{
		assert(IsKeyword());
		return keywords[_int].group;
	}

	inline const string& MustGetItem() const
	{
		AssertItem();
		return GetItem();
	}
	inline const string& MustGetString() const
	{
		AssertString();
		return GetString();
	}
	inline char MustGetSymbol() const
	{
		AssertSymbol();
		return GetSymbol();
	}
	inline int MustGetNumberInt() const
	{
		AssertNumber();
		return GetInt();
	}
	inline int MustGetInt() const
	{
		AssertToken(T_INT);
		return GetInt();
	}
	inline uint MustGetUint() const
	{
		AssertToken(T_INT);
		return GetUint();
	}
	inline float MustGetNumberFloat() const
	{
		AssertNumber();
		return GetFloat();
	}
	inline float MustGetFloat() const
	{
		AssertToken(T_FLOAT);
		return GetFloat();
	}
	inline const string& MustGetKeyword() const
	{
		AssertToken(T_KEYWORD);
		return GetKeyword();
	}
	inline int MustGetKeywordId() const
	{
		AssertToken(T_KEYWORD);
		return GetKeywordId();
	}
	inline int MustGetKeywordId(int group) const
	{
		AssertKeywordGroup(group);
		return GetKeywordId();
	}
	inline const string& MustGetText() const
	{
		AssertText();
		return GetToken();
	}
	inline bool MustGetBool() const
	{
		AssertBool();
		return (_int == 1);
	}

private:
	uint FindFirstNotOf(cstring _str, uint _start);
	uint FindFirstOf(cstring _str, uint _start);
	uint FindFirstOfStr(cstring _str, uint _start);
	uint FindEndOfQuote(uint _start);

	uint pos, line, charpos;
	const string* str;
	string token;
	TOKEN_TYPE type;
	int _int, flags;
	float _float;
	char _char;
	uint _uint;
	vector<Keyword> keywords;
};
