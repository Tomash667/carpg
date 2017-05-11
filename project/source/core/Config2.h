#pragma once

template<typename T>
using Action = fastdelegate::FastDelegate1<const T&>;

class Config2
{
public:
	struct Var
	{
		Var& Readonly();
	};

	struct Section
	{
		Var& AddBool(cstring name, bool& value, Action<bool> f = nullptr);
		Var& AddInt(cstring name, int& value, Action<int>& f = nullptr);
		Var& AddINT2(cstring name, INT2& value, Action<INT2> f = nullptr);
	};

	Section* AddSection(cstring name);
};
