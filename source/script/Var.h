#pragma once

//-----------------------------------------------------------------------------
struct Var
{
	enum class Type
	{
		None,
		Bool,
		Int,
		Float,
		Int2,
		Vec2,
		Vec3,
		Vec4,
		Item,
		Location,
		Encounter,
		GroundItem,
		String,
		Unit,
		UnitGroup,
		Array,
		Class
	};
	Type type;
	union
	{
		bool _bool;
		int _int;
		float _float;
		string* str;
		Int2 int2;
		Vec2 vec2;
		Vec3 vec3;
		Vec4 vec4;
		const Item* item;
		Location* location;
		Encounter* encounter;
		GroundItem* groundItem;
		Unit* unit;
		void* ptr;
	};
	bool registered;

	Var() : type(Type::None) {}
	~Var()
	{
		if(type == Type::String)
			StringPool.Free(str);
	}

	bool IsNone() const
	{
		return type == Type::None;
	}
	bool IsBool() const
	{
		return type == Type::Bool;
	}
	bool IsBool(bool value) const
	{
		return IsBool() && _bool == value;
	}
	bool IsInt() const
	{
		return type == Type::Int;
	}
	bool IsInt(int value) const
	{
		return IsInt() && _int == value;
	}
	bool IsFloat() const
	{
		return type == Type::Float;
	}
	bool IsFloat(float value) const
	{
		return IsFloat() && _float == value;
	}
	bool IsGeneric(void* ptr, int type);
	bool IsVar(Var* var) const
	{
		return type == var->type && _int == var->_int;
	}

	void SetNone();
	Var* SetBool(bool value);
	Var* SetInt(int value);
	Var* SetFloat(float value);
	Var* SetString(const string& str);
	Var* SetGeneric(void* ptr, int type);
	Var* SetVar(Var* var);
	void SetPtr(void* ptr, Type type);

	bool GetBool() const
	{
		return _bool;
	}
	int GetInt() const
	{
		return _int;
	}
	float GetFloat() const
	{
		return _float;
	}

	bool operator == (bool value) const
	{
		return IsBool(value);
	}
	bool operator != (bool value) const
	{
		return !IsBool(value);
	}

	void operator = (bool value)
	{
		SetBool(value);
	}

	void GetGeneric(void* ptr, int type);
};

//-----------------------------------------------------------------------------
struct Vars
{
	Vars() : refs(1) {}
	~Vars();
	void AddRef() { ++refs; }
	void Release() { if(--refs == 0) delete this; }
	Var* Add(Var::Type type, const string& name, bool registered);
	Var* Get(const string& name);
	Var* TryGet(const string& name) const;
	bool IsEmpty() const { return vars.empty(); }
	bool IsSet(const string& name) const { return TryGet(name) != nullptr; }
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Clear();

	static Vars* Create() { return new Vars; }

private:
	std::map<string, Var*> vars;
	int refs;
	static string tmpStr;
};
