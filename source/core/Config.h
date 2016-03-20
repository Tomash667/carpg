// configuration reader/writer
#pragma once

//-----------------------------------------------------------------------------
enum Bool3
{
	False,
	True,
	None
};

//-----------------------------------------------------------------------------
inline bool ToBool(Bool3 b)
{
	return (b == True);
}

//-----------------------------------------------------------------------------
class Config
{
public:
	struct Entry
	{
		string name, value;
	};

	template<typename T>
	struct EnumDef
	{
		cstring name;
		T value;
	};

	enum Result
	{
		NO_FILE,
		PARSE_ERROR,
		OK,
		CANT_SAVE
	};

	enum GetResult
	{
		GET_OK,
		GET_MISSING,
		GET_INVALID
	};

	void Add(cstring name, cstring value);
	Result Load(cstring filename);
	Result Save(cstring filename);
	void Remove(cstring name);

	bool GetBool(cstring name, bool def = false);
	Bool3 GetBool3(cstring name, Bool3 def = None);
	const string& GetString(cstring name);
	const string& GetString(cstring name, const string& def);
	int GetInt(cstring name, int def = -1);
	uint GetUint(cstring name, uint def = 0);
	__int64 GetInt64(cstring name, int def = 0);
	float GetFloat(cstring name, float def = 0.f);
	INT2 GetInt2(cstring name, INT2 def = INT2(0, 0));

	GetResult TryGetInt(cstring name, int& value);
	template<typename T>
	inline GetResult TryGetEnum(cstring name, T& value, std::initializer_list<EnumDef<T>> const & enum_defs)
	{
		Entry* e = GetEntry(name);
		if(!e)
			return GET_MISSING;
		else
		{
			for(const EnumDef<T>& en : enum_defs)
			{
				if(e->value == en.name)
				{
					value = en.value;
					return GET_OK;
				}
			}
			return GET_MISSING;
		}
	}

	inline int GetVersion() const { return version; }
	inline const string& GetError() const { return error; }

	inline Entry* GetEntry(cstring name)
	{
		assert(name);
		for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
		{
			if(it->name == name)
				return &*it;
		}
		return nullptr;
	}

private:
	vector<Entry> entries;
	string tmpstr, error;
	int version;
	static Tokenizer t;
};
