#pragma once

class VarsContainer : public ObjectPoolProxy<VarsContainer>
{
public:
	enum VarType
	{
		None,
		Bool,
		Int,
		Float,
		//String
	};

	struct Var : public ObjectPoolProxy<Var>
	{
		VarType type;
		string name;
		union
		{
			bool _bool;
			int _int;
			float _float;
			//string* _string;
		};

		bool IsNone() const
		{
			return type == None;
		}
		bool IsBool() const
		{
			return type == Bool;
		}
		bool IsInt() const
		{
			return type == Int;
		}
		bool IsFloat() const
		{
			return type == Float;
		}
		bool IsBool(bool value) const
		{
			return type == Bool && _bool == value;
		}
		bool IsInt(int value) const
		{
			return type == Int && _int == value;
		}
		bool IsFloat(float value) const
		{
			return type == Float && _float == value;
		}

		void SetNone()
		{
			type = None;
		}
		void SetBool(bool value)
		{
			type = Bool;
			_bool = value;
		}
		void SetInt(int value)
		{
			type = Int;
			_int = value;
		}
		void SetFloat(float value)
		{
			type = Float;
			_float = value;
		}
	};

	void OnFree();
	void Write(FileWriter& f);
	void Read(FileReader& f);
	Var* GetVar(AnyString name);

private:
	struct VarsComparer
	{
		bool operator () (const Var* v1, const Var* v2) const
		{
			return v1->name > v2->name;
		}
	};

	typedef std::set<Var*, VarsComparer> Container;
	Container vars;
};
