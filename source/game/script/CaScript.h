#pragma once

struct FunctionInfo
{
	FunctionInfo(void* ptr, bool is_method = false) : ptr(ptr), is_method(is_method) {}

	void* ptr;
	bool is_method;
};

template<int N>
struct MethodHelper
{
	template<class M>
	FunctionInfo Convert(M mthd)
	{
		assert(N == 1);
		void* ptr = &mthd;
		int adr = ((int*)ptr)[0];
		return FunctionInfo((void*)adr, true);
	}
};

#define FUNCTION(f) FunctionInfo(f)
#define FUNCTION_EX(f,p,r) FunctionInfo((void (*)())((r (*)p)(f)))
#define METHOD(c,m) MethodHelper<sizeof(void (c::*)())>().Convert((void (c::*)())(&c::m))
#define METHOD_EX(c,m,p,r) MethodHelper<sizeof(void (c::*)())>().Convert((r (c::*)p)(&c::m))

namespace cas
{
	class Enum
	{
	public:
		void Add(cstring id, int value);

	private:
		string id;
	};

	class Type
	{
	public:
		void RegisterFunction(cstring def, const FunctionInfo& f);
	};

	class Engine
	{
	public:
		template<typename T>
		struct StringPair
		{
			cstring id;
			T value;
		};

		template<typename T>
		void RegisterEnum(cstring id, std::initializer_list<StringPair<T>> const & values);

		Enum* RegisterEnum(cstring id);

		Type* RegisterType(cstring id);

		void RegisterFunction(cstring id, const FunctionInfo& f);
	};
};
