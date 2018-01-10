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

		bool Is(bool value) const
		{
			return type == Bool && _bool == value;
		}
		bool Is(int value) const
		{
			return type == Int && _int == value;
		}
		bool Is(float value) const
		{
			return type == Float && _float == value;
		}

		void Set(bool value)
		{
			type = Bool;
			_bool = value;
		}
		void Set(int value)
		{
			type = Int;
			_int = value;
		}
		void Set(float value)
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
