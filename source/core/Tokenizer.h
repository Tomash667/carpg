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
class Tokenizer
{
	friend class Formatter;
public:
	enum TOKEN
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

		T_KEYWORD_GROUP,
		T_NUMBER,
		T_TEXT,
		T_BOOL
	};

	static const int EMPTY_GROUP = -1;

	struct Keyword
	{
		cstring name;
		int id, group;
	};

	struct KeywordToRegister
	{
		cstring name;
		int id;
	};

	class Formatter
	{
		friend class Tokenizer;

	public:
		inline Formatter& Add(TOKEN token, int* what = NULL, int* what2 = NULL)
		{
			if(count > 0)
				s += ", ";
			s += t->FormatToken(token, what, what2);
			++count;
			return *this;
		}

		inline void Throw()
		{
			End();
			throw s.c_str();
		}

		inline cstring Get()
		{
			End();
			return s.c_str();
		}

	private:
		inline Formatter(Tokenizer* t) : t(t), count(0)
		{
			s = Format("(%d:%d) Expecting ", t->line + 1, t->charpos + 1);
		}

		inline void End()
		{
			s += Format(", found %s!", t->GetTokenValue());
		}

		Tokenizer* t;
		string s;
		int count;
	};

	enum FLAGS
	{
		// bez tego jest symbol -, potem liczba
		// z ujemna liczba
		F_JOIN_MINUS = 1<<0, // nie zrobione
		F_JOIN_DOT = 1<<1, // ³¹czy . po tekœcie (np log.txt - zwraca jeden item)
		F_UNESCAPE = 1 << 2, // unescape strings
	};

	Tokenizer(int _flags = Tokenizer::F_UNESCAPE) : flags(_flags)
	{
		Reset();
	}

	inline void Reset()
	{
		token = T_NONE;
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

	typedef bool(*SkipToFunc)(Tokenizer& t);

	bool Next(bool return_eol=false);
	bool NextLine();
	bool SkipTo(SkipToFunc f);
	bool PeekSymbol(char symbol);
	cstring FormatToken(TOKEN token, int* what = NULL, int* what2 = NULL);

	const Keyword* FindKeyword(int id, int group = EMPTY_GROUP) const;
	inline void AddKeyword(cstring name, int id, int group = EMPTY_GROUP)
	{
		assert(name);
		Keyword& k = Add1(keywords);
		k.name = name;
		k.id = id;
		k.group = group;
	}
	void AddKeywords(int group, std::initializer_list<KeywordToRegister> const & to_register);

	inline Formatter StartUnexpected() const { return Formatter((Tokenizer*)this); }
	inline void Unexpected()
	{
		throw Format("(%d:%d) Unexpected %s!", line + 1, charpos + 1, GetTokenValue());
	}
	inline void Unexpected(TOKEN token, int* what = NULL, int* what2 = NULL) const
	{
		StartUnexpected().Add(token, what, what2).Throw();
	}
	inline cstring FormatUnexpected(TOKEN token, int* what = NULL, int* what2 = NULL) const
	{
		return StartUnexpected().Add(token, what, what2).Get();
	}
	inline void Throw(cstring msg)
	{
		throw Format("(%d:%d) %s", line + 1, charpos + 1, msg);
	}

	//===========================================================================================================================
	inline bool IsToken(TOKEN _tt) const { return token == _tt; }
	inline bool IsEof() const { return IsToken(T_EOF); }
	inline bool IsEol() const { return IsToken(T_EOL); }
	inline bool IsItem() const { return IsToken(T_ITEM); }
	inline bool IsString() const { return IsToken(T_STRING); }
	inline bool IsSymbol() const { return IsToken(T_SYMBOL); }
	inline bool IsSymbol(char c) const { return IsSymbol() && GetSymbol() == c; }
	inline bool IsText() const { return IsItem() || IsString() || IsKeyword(); }
	inline bool IsInt() const { return IsToken(T_INT); }
	inline bool IsFloat() const { return IsToken(T_FLOAT); }
	inline bool IsNumber() const { return IsToken(T_INT) || IsToken(T_FLOAT); }
	inline bool IsKeyword() const { return IsToken(T_KEYWORD); }
	inline bool IsKeyword(int id) const { return IsKeyword() && GetKeywordId() == id; }
	inline bool IsKeyword(int id, int group) const { return IsKeyword(id) && GetKeywordGroup() == group; }
	inline bool IsKeywordGroup(int group) const { return IsKeyword() && GetKeywordGroup() == group; }
	inline bool IsBool() const { return IsInt() && (_int == 0 || _int == 1); }

	//===========================================================================================================================
	inline cstring GetTokenName(TOKEN _tt) const
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
		case T_KEYWORD_GROUP:
			return "keyword group";
		case T_NUMBER:
			return "number";
		case T_TEXT:
			return "text";
		case T_BOOL:
			return "bool";
		default:
			assert(0);
			return "unknown";
		}
	}

	// get formatted text of current token
	inline cstring GetTokenValue() const
	{
		switch(token)
		{
		case T_ITEM:
		case T_STRING:
			return Format("%s (%s)", GetTokenName(token), item.c_str());
		case T_SYMBOL:
			return Format("%s '%c'", GetTokenName(token), _char);
		case T_INT:
			return Format("%s %d", GetTokenName(token), _int);
		case T_FLOAT:
			return Format("%s %g", GetTokenName(token), _float);
		case T_KEYWORD:
			return Format("%s (%d,%d:%s)", GetTokenName(token), keyword->id, keyword->group, item.c_str());
		case T_NONE:
		case T_EOF:
		case T_EOL:
		default:
			return GetTokenName(token);
		}
	}

	//===========================================================================================================================
	inline void AssertToken(TOKEN _tt) const
	{
		if(token != _tt)
			Unexpected(_tt);
	}
	inline void AssertEof() const { AssertToken(T_EOF); }
	inline void AssertItem() const { AssertToken(T_ITEM); }
	inline void AssertString() const { AssertToken(T_STRING); }
	inline void AssertSymbol() const { AssertToken(T_SYMBOL); }
	inline void AssertInt() const { AssertToken(T_INT); }
	inline void AssertFloat() const { AssertToken(T_FLOAT); }
	inline void AssertNumber() const
	{
		if(IsNumber())
			Unexpected(T_NUMBER);
	}
	inline void AssertKeyword() const { AssertToken(T_KEYWORD); }
	inline void AssertKeyword(int id) const
	{
		if(!IsKeyword(id))
			Unexpected(T_KEYWORD, &id);
	}
	inline void AssertKeyword(int id, int group) const
	{
		if(!IsKeyword(id, group))
			Unexpected(T_KEYWORD, &id, &group);
	}
	inline void AssertKeywordGroup(int group) const
	{
		if(!IsKeywordGroup(group))
			Unexpected(T_KEYWORD_GROUP, &group);
	}
	inline void AssertSymbol(char c) const
	{
		if(!IsSymbol(c))
			Unexpected(T_SYMBOL, (int*)&c);
	}
	inline void AssertText() const
	{
		if(!IsText())
			Unexpected(T_TEXT);
	}
	inline void AssertBool() const
	{
		if(!IsBool())
			Unexpected(T_BOOL);
	}

	//===========================================================================================================================
	inline TOKEN GetToken() const
	{
		return token;
	}
	inline cstring GetTokenName() const
	{
		return GetTokenName(GetToken());
	}
	inline const string& GetTokenString() const
	{
		return item;
	}
	inline const string& GetItem() const
	{
		assert(IsItem());
		return item;
	}
	inline const string& GetString() const
	{
		assert(IsString());
		return item;
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
	inline const Keyword* GetKeyword() const
	{
		assert(IsKeyword());
		return keyword;
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
	inline const Keyword* MustGetKeyword() const
	{
		AssertKeyword();
		return GetKeyword();
	}
	inline int MustGetKeywordId() const
	{
		AssertKeyword();
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
		return GetTokenString();
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
	string item;
	TOKEN token;
	int _int, flags;
	float _float;
	char _char;
	uint _uint;
	vector<Keyword> keywords;
	Keyword* keyword;
};
