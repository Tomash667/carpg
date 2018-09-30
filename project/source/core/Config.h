// configuration reader/writer
#pragma once

#include "Tokenizer.h"

//-----------------------------------------------------------------------------
enum class AnyVarType
{
	Bool
};

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
union AnyVar
{
	bool _bool;
};

//-----------------------------------------------------------------------------
struct ConfigVar
{
	cstring name;
	AnyVarType type;
	AnyVar* ptr;
	AnyVar new_value;
	bool have_new_value, need_save;

	ConfigVar(cstring name, bool& _bool) : name(name), type(AnyVarType::Bool), ptr((AnyVar*)&_bool), have_new_value(false), need_save(false) {}
};

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
	void Add(cstring name, bool value) { Add(name, value ? "1" : "0"); }
	void AddVar(ConfigVar& var) { config_vars.push_back(var); }
	Result Load(cstring filename);
	Result Save(cstring filename);
	void Remove(cstring name);
	void ParseConfigVar(cstring var);
	void LoadConfigVars();

	int GetVersion() const { return version; }
	const string& GetError() const { return error; }
	Entry* GetEntry(cstring name);
	bool GetBool(cstring name, bool def = false);
	Bool3 GetBool3(cstring name, Bool3 def = None);
	const string& GetString(cstring name);
	const string& GetString(cstring name, const string& def);
	int GetInt(cstring name, int def = -1);
	uint GetUint(cstring name, uint def = 0);
	__int64 GetInt64(cstring name, int def = 0);
	float GetFloat(cstring name, float def = 0.f);
	Int2 GetInt2(cstring name, Int2 def = Int2(0, 0));

	GetResult TryGetInt(cstring name, int& value);
	template<typename T>
	GetResult TryGetEnum(cstring name, T& value, std::initializer_list<EnumDef<T>> const & enum_defs)
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

private:
	vector<Entry> entries;
	string tmpstr, error;
	int version;
	Tokenizer t;
	vector<ConfigVar> config_vars;
};
