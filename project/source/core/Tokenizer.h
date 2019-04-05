#pragma once

namespace tokenizer
{
	enum TOKEN
	{
		T_NONE,
		T_EOF,
		T_EOL,
		T_ITEM,
		T_STRING,
		T_CHAR,
		T_SYMBOL,
		T_INT,
		T_FLOAT,
		T_KEYWORD,

		T_KEYWORD_GROUP,
		T_NUMBER,
		T_TEXT,
		T_BOOL,
		T_SYMBOLS_LIST,
		T_COMPOUND_SYMBOL
	};

	static const int EMPTY_GROUP = -1;
	static const int MISSING_GROUP = -2;

	struct Keyword
	{
		cstring name;
		int id, group;
		bool enabled;

		bool operator < (const Keyword& k)
		{
			return strcmp(name, k.name) < 0;
		}
	};

	struct KeywordGroup
	{
		cstring name;
		int id;

		bool operator == (const KeywordGroup& group) const
		{
			return id == group.id;
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

	struct SeekData
	{
		uint pos, start_pos, line, charpos;
		string item;
		TOKEN token;
		int _int;
		float _float;
		char _char;
		uint _uint;
		vector<Keyword*> keyword;
	};

	struct Pos
	{
		uint pos, line, charpos;
	};

	struct FlagGroup
	{
		int* flags;
		int group;
	};

	//-----------------------------------------------------------------------------
	// Tokenizer
	//-----------------------------------------------------------------------------
	class Tokenizer
	{
		friend class Formatter;

	public:
		static const int EMPTY_GROUP = -1;

		class Exception
		{
		public:
			uint line, charpos;
			string* str, *filename;

			cstring ToString() const
			{
				return Format("%s(%d:%d) %s", filename ? filename->c_str() : "", line, charpos, str->c_str());
			}
		};

		class Formatter
		{
			friend class Tokenizer;

		public:
			Formatter& Add(TOKEN token, int* what = nullptr, int* what2 = nullptr)
			{
				if(count > 0)
					s += ", ";
				s += t->FormatToken(token, what, what2);
				++count;
				return *this;
			}

			Formatter& AddList(TOKEN token, std::initializer_list<int> const & items)
			{
				if(count > 0)
					s += ", ";
				s += t->FormatToken(token);
				s += "{";
				bool first = true;
				for(int item : items)
				{
					if(first)
						first = false;
					s += ", ";
					s += Format("%d", item);
				}
				s += "}";
				++count;
				return *this;
			}

			__declspec(noreturn) void Throw()
			{
				End();
				OnThrow();
				throw e;
			}

			cstring Get()
			{
				End();
				return Format("(%d:%d) %s", e.line, e.charpos, s.c_str());
			}

			const string& GetNoLineNumber()
			{
				End();
				return s;
			}

		private:
			explicit Formatter(Tokenizer* t) : t(t)
			{
				e.str = &s;
				sd = &t->normal_seek;
			}

			void SetFilename()
			{
				if(IS_SET(t->flags, Tokenizer::F_FILE_INFO))
					e.filename = &t->filename;
				else
					e.filename = nullptr;
			}

			void Prepare()
			{
				e.line = t->GetLine();
				e.charpos = t->GetCharPos();
				SetFilename();
			}

			void Start()
			{
				Prepare();
				e.str = &s;
				s = "Expecting ";
				count = 0;
			}

			void End()
			{
				s += Format(", found %s.", t->GetTokenValue(*sd));
			}

			__declspec(noreturn) void Throw(cstring msg)
			{
				assert(msg);
				s = msg;
				Prepare();
				OnThrow();
				throw e;
			}

			__declspec(noreturn) void ThrowAt(uint line, uint charpos, cstring msg)
			{
				assert(msg);
				s = msg;
				e.line = line;
				e.charpos = charpos;
				SetFilename();
				OnThrow();
				throw e;
			}

			void OnThrow() {}

			Exception e;
			Tokenizer* t;
			const SeekData* sd;
			string s;
			int count;
		};

		enum FLAGS
		{
			F_JOIN_MINUS = 1 << 0, // join minus with number (otherwise it's symbol minus and number)
			F_JOIN_DOT = 1 << 1, // join dot after text (log.txt is one item, otherwise log dot txt - 2 items and symbol)
			F_UNESCAPE = 1 << 2, // unescape strings
			F_MULTI_KEYWORDS = 1 << 3, // allows multiple keywords
			F_SEEK = 1 << 4, // allows to use seek operations
			F_FILE_INFO = 1 << 5, // add filename in errors
			F_CHAR = 1 << 6, // handle 'c' as char type (otherwise it's symbol ', identifier c, symbol ')
			F_HIDE_ID = 1 << 7, // in exceptions don't write keyword/group id, only name
		};

		explicit Tokenizer(int flags = F_UNESCAPE);
		~Tokenizer();

		void FromString(cstring str);
		void FromString(const string& str);
		bool FromFile(cstring path);
		bool FromFile(const string& path) { return FromFile(path.c_str()); }
		void FromTokenizer(const Tokenizer& t);

		typedef bool(*SkipToFunc)(Tokenizer& t);

		bool Next(bool return_eol = false) { return DoNext(normal_seek, return_eol); }
		bool NextLine();
		bool SkipTo(delegate<bool(Tokenizer&)> f)
		{
			while(true)
			{
				if(f(*this))
					return true;

				if(!Next())
					return false;
			}
		}
		bool SkipToSymbol(char symbol)
		{
			return SkipTo([symbol](Tokenizer& t) { return t.IsSymbol(symbol); });
		}
		bool SkipToSymbol(cstring symbols)
		{
			return SkipTo([symbols](Tokenizer& t) { return t.IsSymbol(symbols); });
		}
		bool SkipToKeywordGroup(int group)
		{
			return SkipTo([group](Tokenizer& t) { return t.IsKeywordGroup(group); });
		}
		bool SkipToKeyword(int keyword, int group = EMPTY_GROUP)
		{
			return SkipTo([keyword, group](Tokenizer& t) { return t.IsKeyword(keyword, group); });
		}
		bool PeekSymbol(char symbol);
		char PeekChar()
		{
			if(IsEof())
				return 0;
			else
				return str->at(normal_seek.pos);
		}
		void NextChar()
		{
			++normal_seek.pos;
		}
		cstring FormatToken(TOKEN token, int* what = nullptr, int* what2 = nullptr);

		const Keyword* FindKeyword(int id, int group = EMPTY_GROUP) const;
		const KeywordGroup* FindKeywordGroup(int group) const;
		void AddKeyword(cstring name, int id, int group = EMPTY_GROUP)
		{
			assert(name);
			Keyword& k = Add1(keywords);
			k.name = name;
			k.id = id;
			k.group = group;
			k.enabled = true;
			need_sorting = true;
		}
		void AddKeywordGroup(cstring name, int group)
		{
			assert(name);
			assert(group != EMPTY_GROUP);
			KeywordGroup new_group = { name, group };
			assert(std::find(groups.begin(), groups.end(), new_group) == groups.end());
			groups.push_back(new_group);
		}
		// Add keyword for group in format {name, id}
		void AddKeywords(int group, std::initializer_list<KeywordToRegister> const & to_register, cstring group_name = nullptr);
		template<typename T>
		void AddEnums(int group, std::initializer_list<KeywordToRegisterEnum<T>> const & to_register, cstring group_name = nullptr)
		{
			AddKeywords(group, (std::initializer_list<KeywordToRegister> const&)to_register, group_name);
		}
		// Remove keyword (function with name is faster)
		bool RemoveKeyword(cstring name, int id, int group = EMPTY_GROUP);
		bool RemoveKeyword(int id, int group = EMPTY_GROUP);
		bool RemoveKeywordGroup(int group);
		void EnableKeywordGroup(int group);
		void DisableKeywordGroup(int group);

		Formatter& StartUnexpected() const { formatter.Start(); return formatter; }
		Formatter& SeekStartUnexpected() const
		{
			formatter.sd = seek;
			formatter.Start();
			return formatter;
		}
		__declspec(noreturn) void Unexpected(const SeekData& seek_data) const
		{
			formatter.sd = &seek_data;
			formatter.Throw(Format("Unexpected %s.", GetTokenValue(seek_data)));
		}
		__declspec(noreturn) void Unexpected() const
		{
			Unexpected(normal_seek);
		}
		__declspec(noreturn) void SeekUnexpected() const
		{
			assert(seek);
			Unexpected(*seek);
		}
		__declspec(noreturn) void Unexpected(cstring err) const
		{
			assert(err);
			formatter.Throw(Format("Unexpected %s: %s", GetTokenValue(normal_seek), err));
		}
		__declspec(noreturn) void Unexpected(TOKEN expected_token, int* what = nullptr, int* what2 = nullptr) const
		{
			StartUnexpected().Add(expected_token, what, what2).Throw();
		}
		cstring FormatUnexpected(TOKEN expected_token, int* what = nullptr, int* what2 = nullptr) const
		{
			return StartUnexpected().Add(expected_token, what, what2).Get();
		}
		__declspec(noreturn) void Throw(cstring msg)
		{
			formatter.Throw(msg);
		}
		template<typename T>
		__declspec(noreturn) void Throw(cstring msg, T arg, ...)
		{
			cstring err = FormatList(msg, (va_list)&arg);
			formatter.Throw(err);
		}
		__declspec(noreturn) void ThrowAt(uint line, uint charpos, cstring msg)
		{
			formatter.ThrowAt(line, charpos, msg);
		}
		template<typename T>
		__declspec(noreturn) void ThrowAt(uint line, uint charpos, cstring msg, T arg, ...)
		{
			cstring err = FormatList(msg, (va_list)&arg);
			formatter.ThrowAt(line, charpos, err);
		}
		cstring Expecting(cstring what)
		{
			return Format("Expecting %s, found %s.", what, GetTokenValue(normal_seek));
		}
		__declspec(noreturn) void ThrowExpecting(cstring what)
		{
			formatter.Throw(Expecting(what));
		}

		//===========================================================================================================================
		bool IsToken(TOKEN _tt) const { return normal_seek.token == _tt; }
		bool IsEof() const { return IsToken(T_EOF); }
		bool IsEol() const { return IsToken(T_EOL); }
		bool IsItem() const { return IsToken(T_ITEM); }
		bool IsItem(cstring _item) const { return IsItem() && GetItem() == _item; }
		bool IsString() const { return IsToken(T_STRING); }
		bool IsChar() const { return IsToken(T_CHAR); }
		bool IsChar(char c) const { return IsChar() && GetChar() == c; }
		bool IsSymbol() const { return IsToken(T_SYMBOL); }
		bool IsSymbol(char c) const { return IsSymbol() && GetSymbol() == c; }
		bool IsSymbol(cstring s, char* c = nullptr) const;
		bool IsText() const { return IsItem() || IsString() || IsKeyword(); }
		bool IsInt() const { return IsToken(T_INT); }
		bool IsFloat() const { return IsToken(T_FLOAT); }
		bool IsNumber() const { return IsToken(T_INT) || IsToken(T_FLOAT); }
		bool IsKeyword() const { return IsToken(T_KEYWORD); }
		bool IsKeyword(int id) const
		{
			if(IsKeyword())
			{
				for(Keyword* k : normal_seek.keyword)
				{
					if(k->id == id)
						return true;
				}
			}
			return false;
		}
		bool IsKeyword(int id, int group) const
		{
			if(IsKeyword())
			{
				for(Keyword* k : normal_seek.keyword)
				{
					if(k->id == id && k->group == group)
						return true;
				}
			}
			return false;
		}
		bool IsKeywordGroup(int group) const
		{
			if(IsKeyword())
			{
				for(Keyword* k : normal_seek.keyword)
				{
					if(k->group == group)
						return true;
				}
			}
			return false;
		}
		int IsAnyKeyword(int group, std::initializer_list<int> const& keyword_ids)
		{
			for(auto keyword_id : keyword_ids)
			{
				if(IsKeyword(keyword_id, group))
					return keyword_id;
			}
			return -1;
		}
		int IsKeywordGroup(std::initializer_list<int> const & groups) const;
		bool IsBool() const { return IsInt() && (normal_seek._int == 0 || normal_seek._int == 1); }
		bool IsItemOrString() const { return IsItem() || IsString(); }

		//===========================================================================================================================
		static cstring GetTokenName(TOKEN _tt);
		// get formatted text of current token
		cstring GetTokenValue(const SeekData& s) const;
		cstring GetTokenValue() const
		{
			return GetTokenValue(normal_seek);
		}

		//===========================================================================================================================
		void AssertToken(TOKEN _tt) const
		{
			if(normal_seek.token != _tt)
				Unexpected(_tt);
		}
		void AssertEof() const { AssertToken(T_EOF); }
		void AssertItem() const { AssertToken(T_ITEM); }
		void AssertItem(cstring item)
		{
			if(!IsItem(item))
				Unexpected(T_ITEM, (int*)item);
		}
		void AssertString() const { AssertToken(T_STRING); }
		void AssertChar() const { AssertToken(T_CHAR); }
		void AssertChar(char c) const
		{
			if(!IsChar(c))
				Unexpected(T_CHAR, (int*)&c);
		}
		void AssertSymbol() const { AssertToken(T_SYMBOL); }
		void AssertSymbol(char c) const
		{
			if(!IsSymbol(c))
				Unexpected(T_SYMBOL, (int*)&c);
		}
		void AssertInt() const { AssertToken(T_INT); }
		void AssertFloat() const { AssertToken(T_FLOAT); }
		void AssertNumber() const
		{
			if(!IsNumber())
				Unexpected(T_NUMBER);
		}
		void AssertKeyword() const { AssertToken(T_KEYWORD); }
		void AssertKeyword(int id) const
		{
			if(!IsKeyword(id))
				Unexpected(T_KEYWORD, &id);
		}
		void AssertKeyword(int id, int group) const
		{
			if(!IsKeyword(id, group))
				Unexpected(T_KEYWORD, &id, &group);
		}
		void AssertKeywordGroup(int group) const
		{
			if(!IsKeywordGroup(group))
				Unexpected(T_KEYWORD_GROUP, &group);
		}
		void AssertKeywordGroup(std::initializer_list<int> const & groups)
		{
			int group = IsKeywordGroup(groups);
			if(group == MISSING_GROUP)
				StartUnexpected().AddList(T_KEYWORD_GROUP, groups).Throw();
		}
		void AssertText() const
		{
			if(!IsText())
				Unexpected(T_TEXT);
		}
		void AssertBool() const
		{
			if(!IsBool())
				Unexpected(T_BOOL);
		}

		//===========================================================================================================================
		TOKEN GetToken() const
		{
			return normal_seek.token;
		}
		cstring GetTokenName() const
		{
			return GetTokenName(GetToken());
		}
		const string& GetTokenString() const
		{
			return normal_seek.item;
		}
		const string& GetItem() const
		{
			assert(IsItem());
			return normal_seek.item;
		}
		const string& GetString() const
		{
			assert(IsString());
			return normal_seek.item;
		}
		char GetChar() const
		{
			assert(IsChar());
			return normal_seek._char;
		}
		char GetSymbol() const
		{
			assert(IsSymbol());
			return normal_seek._char;
		}
		int GetInt() const
		{
			assert(IsNumber());
			return normal_seek._int;
		}
		uint GetUint() const
		{
			assert(IsNumber());
			return normal_seek._uint;
		}
		float GetFloat() const
		{
			assert(IsNumber());
			return normal_seek._float;
		}
		uint GetLine() const { return normal_seek.line + 1; }
		uint GetCharPos() const { return normal_seek.charpos + 1; }
		const Keyword* GetKeyword() const
		{
			assert(IsKeyword());
			return normal_seek.keyword[0];
		}
		const Keyword* GetKeyword(int id) const
		{
			assert(IsKeyword(id));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->id == id)
					return k;
			}
			return nullptr;
		}
		const Keyword* GetKeyword(int id, int group) const
		{
			assert(IsKeyword(id, group));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->id == id && k->group == group)
					return k;
			}
			return nullptr;
		}
		const Keyword* GetKeywordByGroup(int group) const
		{
			assert(IsKeywordGroup(group));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->group == group)
					return k;
			}
			return nullptr;
		}
		int GetKeywordId() const
		{
			assert(IsKeyword());
			return normal_seek.keyword[0]->id;
		}
		int GetKeywordId(int group) const
		{
			assert(IsKeywordGroup(group));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->group == group)
					return k->id;
			}
			return EMPTY_GROUP;
		}
		int GetKeywordGroup() const
		{
			assert(IsKeyword());
			return normal_seek.keyword[0]->group;
		}
		int GetKeywordGroup(int id) const
		{
			assert(IsKeyword(id));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->id == id)
					return k->group;
			}
			return EMPTY_GROUP;
		}
		const string& GetBlock(char open = '{', char close = '}', bool include_symbol = true);
		const string& GetItemOrString() const
		{
			assert(IsItemOrString());
			return normal_seek.item;
		}
		const string& GetText() const
		{
			assert(IsText());
			return normal_seek.item;
		}

		//===========================================================================================================================
		const string& MustGetItem() const
		{
			AssertItem();
			return GetItem();
		}
		const string& MustGetString() const
		{
			AssertString();
			return GetString();
		}
		const string& MustGetStringTrim()
		{
			AssertString();
			Trim(normal_seek.item);
			if(normal_seek.item.empty())
				Throw("Expected not empty string.");
			return normal_seek.item;
		}
		char MustGetChar() const
		{
			AssertChar();
			return GetChar();
		}
		char MustGetSymbol() const
		{
			AssertSymbol();
			return GetSymbol();
		}
		char MustGetSymbol(cstring symbols) const;
		int MustGetNumberInt() const
		{
			AssertNumber();
			return GetInt();
		}
		int MustGetInt() const
		{
			AssertToken(T_INT);
			return GetInt();
		}
		uint MustGetUint() const
		{
			AssertToken(T_INT);
			return GetUint();
		}
		float MustGetNumberFloat() const
		{
			AssertNumber();
			return GetFloat();
		}
		float MustGetFloat() const
		{
			AssertToken(T_FLOAT);
			return GetFloat();
		}
		const Keyword* MustGetKeyword() const
		{
			AssertKeyword();
			return GetKeyword();
		}
		const Keyword* MustGetKeyword(int id) const
		{
			AssertKeyword(id);
			return GetKeyword(id);
		}
		const Keyword* MustGetKeyword(int id, int group) const
		{
			AssertKeyword(id, group);
			return GetKeyword(id, group);
		}
		const Keyword* MustGetKeywordByGroup(int group) const
		{
			AssertKeywordGroup(group);
			return GetKeywordByGroup(group);
		}
		int MustGetKeywordId() const
		{
			AssertKeyword();
			return GetKeywordId();
		}
		int MustGetKeywordId(int group) const
		{
			AssertKeywordGroup(group);
			return GetKeywordId(group);
		}
		int MustGetKeywordGroup() const
		{
			AssertKeyword();
			return GetKeywordGroup();
		}
		int MustGetKeywordGroup(int id) const
		{
			AssertKeyword(id);
			return GetKeywordGroup(id);
		}
		template<typename T>
		T MustGetKeywordGroup(std::initializer_list<T> const & groups)
		{
			int group = IsKeywordGroup(groups);
			if(group == MISSING_GROUP)
				StartUnexpected().AddList(T_KEYWORD_GROUP, groups).Throw();
			return (T)group;
		}
		const string& MustGetText() const
		{
			AssertText();
			return GetTokenString();
		}
		bool MustGetBool() const
		{
			AssertBool();
			return (normal_seek._int == 1);
		}
		const string& MustGetItemKeyword() const
		{
			if(!IsItem() && !IsKeyword())
				StartUnexpected().Add(T_ITEM).Add(T_KEYWORD).Throw();
			return normal_seek.item;
		}

		//===========================================================================================================================
		bool SeekStart(bool return_eol = false);
		bool SeekNext(bool return_eol = false)
		{
			assert(seek);
			return DoNext(*seek, return_eol);
		}
		bool SeekToken(TOKEN _token) const
		{
			assert(seek);
			return _token == seek->token;
		}
		bool SeekItem() const
		{
			return SeekToken(T_ITEM);
		}
		bool SeekSymbol() const
		{
			return SeekToken(T_SYMBOL);
		}
		bool SeekSymbol(char c) const
		{
			return SeekSymbol() && seek->_char == c;
		}

		//===========================================================================================================================
		SeekData& Query();
		bool QuerySymbol(char c) { SeekData& sd = Query(); return sd.token == T_SYMBOL && sd._char == c; }

		//===========================================================================================================================
		void Parse(Int2& i);
		void Parse(Rect& b);
		void Parse(Vec2& v);
		void ParseFlags(int group, int& flags);
		void ParseFlags(std::initializer_list<FlagGroup> const& flags);

		template<typename Top, typename Action>
		int ParseTop(int group, Action action)
		{
			int errors = 0;

			try
			{
				Next();

				while(!IsEof())
				{
					bool skip = false;

					if(IsKeywordGroup(group))
					{
						Top top = (Top)GetKeywordId(group);
						Next();
						if(!action(top))
							skip = true;
					}
					else
					{
						Error(FormatUnexpected(T_KEYWORD_GROUP, &group));
						skip = true;
					}

					if(skip)
					{
						SkipToKeywordGroup(group);
						++errors;
					}
					else
						Next();
				}
			}
			catch(const Exception& e)
			{
				Error("Failed to parse top group: %s", e.ToString());
				++errors;
			}

			return errors;
		}

		void SetFlags(int _flags);
		void CheckItemOrKeyword(const string& item)
		{
			CheckSorting();
			CheckItemOrKeyword(normal_seek, item);
		}
		Pos GetPos();
		void MoveTo(const Pos& pos);
		char GetClosingSymbol(char start);
		bool MoveToClosingSymbol(char start, char end = 0);
		void ForceMoveToClosingSymbol(char start, char end = 0);
		void Reset()
		{
			normal_seek.token = T_NONE;
			normal_seek.pos = 0;
			normal_seek.line = 0;
			normal_seek.charpos = 0;
		}
		cstring GetTextRest();

	private:
		bool DoNext(SeekData& s, bool return_eol);
		void CheckItemOrKeyword(SeekData& s, const string& item);
		bool ParseNumber(SeekData& s, uint pos2, bool negative);
		uint FindFirstNotOf(SeekData& s, cstring _str, uint _start);
		uint FindFirstOf(SeekData& s, cstring _str, uint _start);
		uint FindFirstOfStr(SeekData& s, cstring _str, uint _start);
		uint FindEndOfQuote(SeekData& s, uint _start);
		void CheckSorting();
		bool CheckMultiKeywords() const;

		const string* str;
		int flags;
		string filename;
		vector<Keyword> keywords;
		vector<KeywordGroup> groups;
		SeekData normal_seek;
		SeekData* seek;
		bool need_sorting, own_string;
		mutable Formatter formatter;
	};
}
