#pragma once

struct Var
{
	enum class Type
	{
		None,
		Bool,
		Int,
		Float
	};
	Type type;
	union
	{
		bool _bool;
		int _int;
		float _float;
	};

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
	bool IsVar(Var* var) const
	{
		return type == var->type && _int == var->_int;
	}

	void SetNone()
	{
		type = Type::None;
	}
	Var* SetBool(bool value)
	{
		type = Type::Bool;
		_bool = value;
		return this;
	}
	Var* SetInt(int value)
	{
		type = Type::Int;
		_int = value;
		return this;
	}
	Var* SetFloat(float value)
	{
		type = Type::Float;
		_float = value;
		return this;
	}
	Var* SetVar(Var* var)
	{
		type = var->type;
		_int = var->_int;
		return this;
	}

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
};

struct VarsContainer
{
	Var* Get(const string& id)
	{
		auto it = vars.lower_bound(id);
		if(it != vars.end() && !(vars.key_comp()(id, it->first)))
			return it->second;
		else
		{
			Var* var = new Var;
			var->type = Var::Type::None;
			vars.insert(it, std::map<string, Var*>::value_type(id, var));
			return var;
		}
	}

	std::map<string, Var*> vars;
};
