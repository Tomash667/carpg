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
		T_SYMBOL,
		T_INT,
		T_FLOAT,
		T_KEYWORD,
		T_BROKEN_NUMBER,

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

		inline bool operator < (const Keyword& k)
		{
			return strcmp(name, k.name) < 0;
		}
	};

	struct KeywordGroup
	{
		cstring name;
		int id;

		inline bool operator == (const KeywordGroup& group) const
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
		static const int EMPTY_GROUP = -1;

		class Exception
		{
		public:
			int line, charpos;
			string* str, *filename;

			inline cstring ToString() const
			{
				return Format("%s(%d:%d) %s", filename ? filename->c_str() : "", line, charpos, str->c_str());
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

			inline Formatter& AddList(TOKEN token, std::initializer_list<int> const & items)
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

			inline __declspec(noreturn) void Throw()
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
				sd = &t->normal_seek;
			}

			inline void Prepare()
			{
				e.line = t->GetLine();
				e.charpos = t->GetCharPos();
				if(IS_SET(t->flags, Tokenizer::F_FILE_INFO))
					e.filename = &t->filename;
				else
					e.filename = nullptr;
			}

			inline void Start()
			{
				Prepare();
				e.str = &s;
				s = "Expecting ";
				count = 0;
			}

			inline void End()
			{
				s += Format(", found %s.", t->GetTokenValue(*sd));
			}

			inline __declspec(noreturn) void Throw(cstring msg)
			{
				assert(msg);
				s = msg;
				Prepare();
				throw e;
			}

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
		};

		explicit Tokenizer(int _flags = F_UNESCAPE) : need_sorting(false), formatter(this)
		{
			SetFlags(_flags);
			Reset();
		}

		void FromString(cstring _str);
		void FromString(const string& _str);
		bool FromFile(cstring path);
		void FromTokenizer(const Tokenizer& t);

		typedef bool(*SkipToFunc)(Tokenizer& t);

		inline bool Next(bool return_eol = false) { return DoNext(normal_seek, return_eol); }
		bool NextLine();
		bool SkipTo(SkipToFunc f);
		bool SkipToKeywordGroup(int group);
		bool PeekSymbol(char symbol);
		inline char PeekChar()
		{
			if(IsEof())
				return 0;
			else
				return str->at(normal_seek.pos);
		}
		inline void NextChar()
		{
			++normal_seek.pos;
		}
		cstring FormatToken(TOKEN token, int* what = nullptr, int* what2 = nullptr);

		const Keyword* FindKeyword(int id, int group = EMPTY_GROUP) const;
		const KeywordGroup* FindKeywordGroup(int group) const;
		inline void AddKeyword(cstring name, int id, int group = EMPTY_GROUP)
		{
			assert(name);
			Keyword& k = Add1(keywords);
			k.name = name;
			k.id = id;
			k.group = group;
			k.enabled = true;
			need_sorting = true;
		}
		inline void AddKeywordGroup(cstring name, int group)
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
		inline void AddEnums(int group, std::initializer_list<KeywordToRegisterEnum<T>> const & to_register, cstring group_name = nullptr)
		{
			AddKeywords(group, (std::initializer_list<KeywordToRegister> const&)to_register, group_name);
		}
		// Remove keyword (function with name is faster)
		bool RemoveKeyword(cstring name, int id, int group = EMPTY_GROUP);
		bool RemoveKeyword(int id, int group = EMPTY_GROUP);
		bool RemoveKeywordGroup(int group);
		void EnableKeywordGroup(int group);
		void DisableKeywordGroup(int group);

		inline Formatter& StartUnexpected() const { formatter.Start(); return formatter; }
		inline __declspec(noreturn) void Unexpected(const SeekData& seek_data) const
		{
			formatter.sd = &seek_data;
			formatter.Throw(Format("Unexpected %s.", GetTokenValue(seek_data)));
		}
		inline __declspec(noreturn) void Unexpected() const
		{
			Unexpected(normal_seek);
		}
		inline __declspec(noreturn) void SeekUnexpected() const
		{
			assert(seek);
			Unexpected(*seek);
		}
		inline __declspec(noreturn) void Unexpected(cstring err) const
		{
			assert(err);
			formatter.Throw(Format("Unexpected %s: %s", GetTokenValue(normal_seek), err));
		}
		inline __declspec(noreturn) void Unexpected(TOKEN expected_token, int* what = nullptr, int* what2 = nullptr) const
		{
			StartUnexpected().Add(expected_token, what, what2).Throw();
		}
		inline cstring FormatUnexpected(TOKEN expected_token, int* what = nullptr, int* what2 = nullptr) const
		{
			return StartUnexpected().Add(expected_token, what, what2).Get();
		}
		inline __declspec(noreturn) void Throw(cstring msg)
		{
			formatter.Throw(msg);
		}
		template<typename T>
		inline __declspec(noreturn) void Throw(cstring msg, T arg, ...)
		{
			cstring err = FormatList(msg, (va_list)&arg);
			formatter.Throw(err);
		}

		//===========================================================================================================================
		inline bool IsToken(TOKEN _tt) const { return normal_seek.token == _tt; }
		inline bool IsEof() const { return IsToken(T_EOF); }
		inline bool IsEol() const { return IsToken(T_EOL); }
		inline bool IsItem() const { return IsToken(T_ITEM); }
		inline bool IsItem(cstring _item) const { return IsItem() && GetItem() == _item; }
		inline bool IsString() const { return IsToken(T_STRING); }
		inline bool IsSymbol() const { return IsToken(T_SYMBOL); }
		inline bool IsSymbol(char c) const { return IsSymbol() && GetSymbol() == c; }
		bool IsSymbol(cstring s, char* c = nullptr) const;
		inline bool IsText() const { return IsItem() || IsString() || IsKeyword(); }
		inline bool IsInt() const { return IsToken(T_INT); }
		inline bool IsFloat() const { return IsToken(T_FLOAT); }
		inline bool IsNumber() const { return IsToken(T_INT) || IsToken(T_FLOAT); }
		inline bool IsKeyword() const { return IsToken(T_KEYWORD); }
		inline bool IsKeyword(int id) const
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
		inline bool IsKeyword(int id, int group) const
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
		inline bool IsKeywordGroup(int group) const
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
		int IsKeywordGroup(std::initializer_list<int> const & groups) const;
		inline bool IsBool() const { return IsInt() && (normal_seek._int == 0 || normal_seek._int == 1); }
		inline bool IsBrokenNumber() const { return IsToken(T_BROKEN_NUMBER); }

		//===========================================================================================================================
		static cstring GetTokenName(TOKEN _tt);
		// get formatted text of current token
		cstring GetTokenValue(const SeekData& s) const;

		//===========================================================================================================================
		inline void AssertToken(TOKEN _tt) const
		{
			if(normal_seek.token != _tt)
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
		template<typename T>
		inline void AssertKeywordGroup(std::initializer_list<T> const & groups)
		{
			int group = IsKeywordGroup(groups);
			if(group == MISSING_GROUP)
				StartUnexpected().AddList(T_KEYWORD_GROUP, groups);
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
		inline void AssertBrokenNumber() const { AssertToken(T_BROKEN_NUMBER); }

		//===========================================================================================================================
		inline TOKEN GetToken() const
		{
			return normal_seek.token;
		}
		inline cstring GetTokenName() const
		{
			return GetTokenName(GetToken());
		}
		inline const string& GetTokenString() const
		{
			return normal_seek.item;
		}
		inline const string& GetItem() const
		{
			assert(IsItem());
			return normal_seek.item;
		}
		inline const string& GetString() const
		{
			assert(IsString());
			return normal_seek.item;
		}
		inline char GetSymbol() const
		{
			assert(IsSymbol());
			return normal_seek._char;
		}
		inline int GetInt() const
		{
			assert(IsNumber());
			return normal_seek._int;
		}
		inline uint GetUint() const
		{
			assert(IsNumber());
			return normal_seek._uint;
		}
		inline float GetFloat() const
		{
			assert(IsNumber());
			return normal_seek._float;
		}
		inline uint GetLine() const { return normal_seek.line + 1; }
		inline uint GetCharPos() const { return normal_seek.charpos + 1; }
		inline const Keyword* GetKeyword() const
		{
			assert(IsKeyword());
			return normal_seek.keyword[0];
		}
		inline const Keyword* GetKeyword(int id) const
		{
			assert(IsKeyword(id));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->id == id)
					return k;
			}
			return nullptr;
		}
		inline const Keyword* GetKeyword(int id, int group) const
		{
			assert(IsKeyword(id, group));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->id == id && k->group == group)
					return k;
			}
			return nullptr;
		}
		inline const Keyword* GetKeywordByGroup(int group) const
		{
			assert(IsKeywordGroup(group));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->group == group)
					return k;
			}
			return nullptr;
		}
		inline int GetKeywordId() const
		{
			assert(IsKeyword());
			return normal_seek.keyword[0]->id;
		}
		inline int GetKeywordId(int group) const
		{
			assert(IsKeywordGroup(group));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->group == group)
					return k->id;
			}
			return EMPTY_GROUP;
		}
		inline int GetKeywordGroup() const
		{
			assert(IsKeyword());
			return normal_seek.keyword[0]->group;
		}
		inline int GetKeywordGroup(int id) const
		{
			assert(IsKeyword(id));
			for(Keyword* k : normal_seek.keyword)
			{
				if(k->id == id)
					return k->group;
			}
			return EMPTY_GROUP;
		}
		const string& GetBlock(char open = '{', char close = '}');

		//===========================================================================================================================
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
		inline const string& MustGetStringTrim()
		{
			AssertString();
			trim(normal_seek.item);
			if(normal_seek.item.empty())
				Throw("Expected not empty string.");
			return normal_seek.item;
		}
		inline char MustGetSymbol() const
		{
			AssertSymbol();
			return GetSymbol();
		}
		char MustGetSymbol(cstring symbols) const;
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
		template<typename T>
		inline T MustGetKeywordGroup(std::initializer_list<T> const & groups)
		{
			int group = IsKeywordGroup(groups);
			if(group == MISSING_GROUP)
				StartUnexpected().AddList(T_KEYWORD_GROUP, groups);
			return (T)group;
		}
		inline const string& MustGetText() const
		{
			AssertText();
			return GetTokenString();
		}
		inline bool MustGetBool() const
		{
			AssertBool();
			return (normal_seek._int == 1);
		}
		inline const string& MustGetItemKeyword() const
		{
			if(!IsItem() && !IsKeyword())
				StartUnexpected().Add(T_ITEM).Add(T_KEYWORD).Throw();
			return normal_seek.item;
		}

		//===========================================================================================================================
		bool SeekStart(bool return_eol = false);
		inline bool SeekNext(bool return_eol = false)
		{
			assert(seek);
			return DoNext(*seek, return_eol);
		}
		inline bool SeekToken(TOKEN _token) const
		{
			assert(seek);
			return _token == seek->token;
		}
		inline bool SeekItem() const
		{
			return SeekToken(T_ITEM);
		}
		inline bool SeekSymbol() const
		{
			return SeekToken(T_SYMBOL);
		}
		inline bool SeekSymbol(char c) const
		{
			return SeekSymbol() && seek->_char == c;
		}

		//===========================================================================================================================
		void Parse(INT2& i);
		void Parse(IBOX2D& b);
#ifndef NO_DIRECT_X
		void Parse(VEC2& v);
#endif

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
						ERROR(FormatUnexpected(T_KEYWORD_GROUP, &group));
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
				ERROR(Format("Failed to parse top group: %s", e.ToString()));
				++errors;
			}

			return errors;
		}

		void SetFlags(int _flags);
		inline void CheckItemOrKeyword(const string& item) { CheckItemOrKeyword(normal_seek, item); }
		Pos GetPos();
		void MoveTo(const Pos& pos);
		bool MoveToClosingSymbol(char start, char end);

	private:
		bool DoNext(SeekData& s, bool return_eol);
		void CheckItemOrKeyword(SeekData& s, const string& item);
		void ParseNumber(SeekData& s, uint pos2, bool negative);
		uint FindFirstNotOf(SeekData& s, cstring _str, uint _start);
		uint FindFirstOf(SeekData& s, cstring _str, uint _start);
		uint FindFirstOfStr(SeekData& s, cstring _str, uint _start);
		uint FindEndOfQuote(SeekData& s, uint _start);
		void CheckSorting();
		bool CheckMultiKeywords();
		inline void Reset()
		{
			normal_seek.token = T_NONE;
			normal_seek.pos = 0;
			normal_seek.line = 0;
			normal_seek.charpos = 0;
		}

		const string* str;
		int flags;
		string filename;
		vector<Keyword> keywords;
		vector<KeywordGroup> groups;
		SeekData normal_seek;
		SeekData* seek;
		bool need_sorting;
		mutable Formatter formatter;
	};
}

using tokenizer::Tokenizer;

//-----------------------------------------------------------------------------
struct FlagGroup
{
	int* flags;
	int group;
};

//-----------------------------------------------------------------------------
int ReadFlags(Tokenizer& t, int group);
void ReadFlags(Tokenizer& t, std::initializer_list<FlagGroup> const & flags, bool clear);
