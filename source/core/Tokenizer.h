#pragma once

//-----------------------------------------------------------------------------
// Tokenizer
// todo:
// - 'a', ' b'
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

		inline bool operator < (const Keyword& k)
		{
			return strcmp(name, k.name) < 0;
		}
	};

	struct KeywordToRegister
	{
		cstring name;
		int id;
	};

	template<typename T>
	struct KeywordToRegisterEnum
	{
		cstring name;
		T id;
	};

	class Exception
	{
	public:
		int line, charpos;
		string* str;

		inline cstring ToString() const
		{
			return Format("(%d:%d) %s", line, charpos, str->c_str());
		}
	};

	class Formatter
	{
		friend class Tokenizer;

	public:
		inline Formatter& Add(TOKEN token, int* what = nullptr, int* what2 = nullptr)
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
			throw e;
		}

		inline cstring Get()
		{
			End();
			return Format("(%d:%d) %s", e.line, e.charpos, s.c_str());
		}

	private:
		explicit inline Formatter(Tokenizer* t) : t(t)
		{
			e.str = &s;
		}

		inline void Start()
		{
			e.line = t->line + 1;
			e.charpos = t->charpos + 1;
			e.str = &s;
			s = "Expecting ";
			count = 0;
		}

		inline void End()
		{
			s += Format(", found %s.", t->GetTokenValue());
		}

		inline void Throw(cstring msg)
		{
			assert(msg);
			s = msg;
			e.line = t->line + 1;
			e.charpos = t->charpos + 1;
			throw e;
		}

		Exception e;
		Tokenizer* t;
		string s;
		int count;
	};

	enum FLAGS
	{
		F_JOIN_MINUS = 1<<0, // join minus with number (otherwise it's symbol minus and number)
		F_JOIN_DOT = 1<<1, // join dot after text (log.txt is one item, otherwise log dot txt - 2 items and symbol)
		F_UNESCAPE = 1 << 2, // unescape strings
		F_MULTI_KEYWORDS = 1 << 3, // allows multiple keywords
	};

	explicit Tokenizer(int _flags = Tokenizer::F_UNESCAPE) : flags(_flags), need_sorting(false), formatter(this)
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
	bool SkipToKeywordGroup(int group);
	bool PeekSymbol(char symbol);
	inline char PeekChar()
	{
		if(IsEof())
			return 0;
		else
			return str->at(pos);
	}
	inline void NextChar()
	{
		++pos;
	}
	cstring FormatToken(TOKEN token, int* what = nullptr, int* what2 = nullptr);

	const Keyword* FindKeyword(int id, int group = EMPTY_GROUP) const;
	inline void AddKeyword(cstring name, int id, int group = EMPTY_GROUP)
	{
		assert(name);
		Keyword& k = Add1(keywords);
		k.name = name;
		k.id = id;
		k.group = group;
		need_sorting = true;
	}
	// Add keyword for group in format {name, id}
	void AddKeywords(int group, std::initializer_list<KeywordToRegister> const & to_register);
	template<typename T>
	inline void AddEnums(int group, std::initializer_list<KeywordToRegisterEnum<T>> const & to_register)
	{
		for(const KeywordToRegisterEnum<T>& k : to_register)
			AddKeyword(k.name, (int)k.id, group);

		if(to_register.size() > 0)
			need_sorting = true;
	}

	inline Formatter& StartUnexpected() const { formatter.Start();  return formatter; }
	inline void Unexpected()
	{
		formatter.Throw(Format("Unexpected %s.", GetTokenValue()));
	}
	inline void Unexpected(TOKEN token, int* what = nullptr, int* what2 = nullptr) const
	{
		StartUnexpected().Add(token, what, what2).Throw();
	}
	inline cstring FormatUnexpected(TOKEN token, int* what = nullptr, int* what2 = nullptr) const
	{
		return StartUnexpected().Add(token, what, what2).Get();
	}
	inline void Throw(cstring msg)
	{
		formatter.Throw(msg);
	}
	template<typename T>
	inline void Throw(cstring msg, T arg, ...)
	{
		va_list list;
		va_start(list, msg);
		cstring err = FormatList(msg, list);
		va_end(list);
		formatter.Throw(err);
	}

	//===========================================================================================================================
	inline bool IsToken(TOKEN _tt) const { return token == _tt; }
	inline bool IsEof() const { return IsToken(T_EOF); }
	inline bool IsEol() const { return IsToken(T_EOL); }
	inline bool IsItem() const { return IsToken(T_ITEM); }
	inline bool IsItem(cstring item) const { return IsItem() && GetItem() == item; }
	inline bool IsString() const { return IsToken(T_STRING); }
	inline bool IsSymbol() const { return IsToken(T_SYMBOL); }
	inline bool IsSymbol(char c) const { return IsSymbol() && GetSymbol() == c; }
	inline bool IsText() const { return IsItem() || IsString() || IsKeyword(); }
	inline bool IsInt() const { return IsToken(T_INT); }
	inline bool IsFloat() const { return IsToken(T_FLOAT); }
	inline bool IsNumber() const { return IsToken(T_INT) || IsToken(T_FLOAT); }
	inline bool IsKeyword() const { return IsToken(T_KEYWORD); }
	inline bool IsKeyword(int id) const
	{
		if(IsKeyword())
		{
			for(Keyword* k : keyword)
			{
				if(k->id == id)
					return true;
			}
		}
		return false;
	}
	inline bool IsKeyword(int id, int group) const
	{
		if(IsKeyword())
		{
			for(Keyword* k : keyword)
			{
				if(k->id == id && k->group == group)
					return true;
			}
		}
		return false;
	}
	inline bool IsKeywordGroup(int group) const
	{
		if(IsKeyword())
		{
			for(Keyword* k : keyword)
			{
				if(k->group == group)
					return true;
			}
		}
		return false;
	}
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
			if(keyword.size() == 1)
				return Format("%s (%d,%d:%s)", GetTokenName(token), keyword[0]->id, keyword[0]->group, item.c_str());
			else
			{
				LocalString s = Format("multiple keywords (%s) [", item.c_str());
				bool first = true;
				for(Keyword* k : keyword)
				{
					if(first)
						first = false;
					else
						s += ", ";
					s += Format("(%d,%d)", k->id, k->group);
				}
				s += "]";
				return s.c_str();
			}
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
		if(!IsNumber())
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
		return keyword[0];
	}
	inline const Keyword* GetKeyword(int id) const
	{
		assert(IsKeyword(id));
		for(Keyword* k : keyword)
		{
			if(k->id == id)
				return k;
		}
		return nullptr;
	}
	inline const Keyword* GetKeyword(int id, int group) const
	{
		assert(IsKeyword(id, group));
		for(Keyword* k : keyword)
		{
			if(k->id == id && k->group == group)
				return k;
		}
		return nullptr;
	}
	inline const Keyword* GetKeywordByGroup(int group) const
	{
		assert(IsKeywordGroup(group));
		for(Keyword* k : keyword)
		{
			if(k->group == group)
				return k;
		}
		return nullptr;
	}
	inline int GetKeywordId() const
	{
		assert(IsKeyword());
		return keyword[0]->id;
	}
	inline int GetKeywordId(int group) const
	{
		assert(IsKeywordGroup(group));
		for(Keyword* k : keyword)
		{
			if(k->group == group)
				return k->id;
		}
		return EMPTY_GROUP;
	}
	inline int GetKeywordGroup() const
	{
		assert(IsKeyword());
		return keyword[0]->group;
	}
	inline int GetKeywordGroup(int id) const
	{
		assert(IsKeyword(id));
		for(Keyword* k : keyword)
		{
			if(k->id == id)
				return k->group;
		}
		return EMPTY_GROUP;
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
	inline const Keyword* MustGetKeyword(int id) const
	{
		AssertKeyword(id);
		return GetKeyword(id);
	}
	inline const Keyword* MustGetKeyword(int id, int group) const
	{
		AssertKeyword(id, group);
		return GetKeyword(id, group);
	}
	inline const Keyword* MustGetKeywordByGroup(int group) const
	{
		AssertKeywordGroup(group);
		return GetKeywordByGroup(group);
	}
	inline int MustGetKeywordId() const
	{
		AssertKeyword();
		return GetKeywordId();
	}
	inline int MustGetKeywordId(int group) const
	{
		AssertKeywordGroup(group);
		return GetKeywordId(group);
	}
	inline int MustGetKeywordGroup() const
	{
		AssertKeyword();
		return GetKeywordGroup();
	}
	inline int MustGetKeywordGroup(int id) const
	{
		AssertKeyword(id);
		return GetKeywordGroup(id);
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
	inline const string& MustGetItemKeyword() const
	{
		if(!IsItem() && !IsKeyword())
			StartUnexpected().Add(T_ITEM).Add(T_KEYWORD).Throw();
		return item;
	}

	void Parse(INT2& i);
	void Parse(IBOX2D& b);
#ifndef NO_DIRECT_X
	void Parse(VEC2& v);
#endif

	inline void SetFlags(int _flags)
	{
		flags = _flags;
	}

private:
	uint FindFirstNotOf(cstring _str, uint _start);
	uint FindFirstOf(cstring _str, uint _start);
	uint FindFirstOfStr(cstring _str, uint _start);
	uint FindEndOfQuote(uint _start);
	void CheckSorting();
	bool CheckMultiKeywords();

	uint pos, line, charpos;
	const string* str;
	string item;
	TOKEN token;
	int _int, flags;
	float _float;
	char _char;
	uint _uint;
	vector<Keyword> keywords;
	vector<Keyword*> keyword;
	bool need_sorting;
	mutable Formatter formatter;
};

//-----------------------------------------------------------------------------
struct FlagGroup
{
	int* flags;
	int group;
};

//-----------------------------------------------------------------------------
int ReadFlags(Tokenizer& t, int group);
void ReadFlags(Tokenizer& t, std::initializer_list<FlagGroup> const & flags, bool clear);
