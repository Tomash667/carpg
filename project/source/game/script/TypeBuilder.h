#pragma once

namespace internal
{
	template<typename E>
	using is_scoped_enum = std::integral_constant<
		bool,
		std::is_enum<E>::value && !std::is_convertible<E, int>::value>;

	template<typename T>
	void Constructor(void* adr)
	{
		new(adr)T();
	}

	template<typename T, typename... Args>
	void ConstructorArgs(void* adr, Args... args)
	{
		new(adr) T(args...);
	}

	template<typename T>
	void Destructor(void* adr)
	{
		((T*)adr)->~T();
	}
}

class TypeBuilder
{
public:
	TypeBuilder(cstring name, asIScriptEngine* engine) : name(name), engine(engine)
	{

	}

	TypeBuilder& Constructor(cstring decl, const asSFuncPtr& funcPointer)
	{
		assert(decl);
		int flag = (funcPointer.flag == 3 ? asCALL_THISCALL : asCALL_CDECL_OBJFIRST);
		CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, decl, funcPointer, flag));
		return *this;
	}

	template<typename T, typename... Args>
	TypeBuilder& Constructor(cstring decl)
	{
		assert(decl);
		CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, decl, asFUNCTION((internal::ConstructorArgs<T, Args...>)), asCALL_CDECL_OBJFIRST));
		return *this;
	}

	TypeBuilder& Method(cstring decl, const asSFuncPtr& funcPointer)
	{
		assert(decl);
		int flag = (funcPointer.flag == 3 ? asCALL_THISCALL : asCALL_CDECL_OBJFIRST);
		CHECKED(engine->RegisterObjectMethod(name, decl, funcPointer, flag));
		return *this;
	}

	TypeBuilder& Member(cstring decl, int offset)
	{
		assert(decl);
		CHECKED(engine->RegisterObjectProperty(name, decl, offset));
		return *this;
	}

	TypeBuilder& WithInstance(cstring decl, void* ptr)
	{
		assert(decl && ptr);
		CHECKED(engine->RegisterGlobalProperty(decl, ptr));
		return *this;
	}

	TypeBuilder& WithSingleton(cstring decl)
	{
		assert(decl);
		void* ptr = (void*)0xC0DEFAB0;
		CHECKED(engine->RegisterGlobalProperty(decl, ptr));
		return *this;
	}

private:
	cstring name;
	asIScriptEngine* engine;
};

template<typename T>
class SpecificTypeBuilder : public TypeBuilder
{
public:
	SpecificTypeBuilder(cstring name, asIScriptEngine* engine) : TypeBuilder(name, engine)
	{

	}

	SpecificTypeBuilder<T>& Constructor(cstring decl, const asSFuncPtr& funcPointer)
	{
		TypeBuilder::Constructor(decl, funcPointer);
		return *this;
	}

	template<typename... Args>
	SpecificTypeBuilder<T>& Constructor(cstring decl)
	{
		TypeBuilder::Constructor<T, Args...>(decl);
		return *this;
	}

	SpecificTypeBuilder<T>& Method(cstring decl, const asSFuncPtr& funcPointer)
	{
		TypeBuilder::Method(decl, funcPointer);
		return *this;
	}

	SpecificTypeBuilder<T>& Member(cstring decl, int offset)
	{
		TypeBuilder::Member(decl, offset);
		return *this;
	}

	SpecificTypeBuilder<T>& WithInstance(cstring decl, void* ptr)
	{
		TypeBuilder::WithInstance(decl, ptr);
		return *this;
	}
};
