#pragma once

//-----------------------------------------------------------------------------
class NamespaceBuilder;

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

	TypeBuilder& Behaviour(asEBehaviours behaviour, cstring decl, const asSFuncPtr& funcPointer)
	{
		assert(decl);
		int flag = (funcPointer.flag == 3 ? asCALL_THISCALL : asCALL_CDECL_OBJFIRST);
		CHECKED(engine->RegisterObjectBehaviour(name, behaviour, decl, funcPointer, flag));
		return *this;
	}

	TypeBuilder& Factory(const asSFuncPtr& func)
	{
		CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_FACTORY, Format("%s@ f()", name), func, asCALL_CDECL));
		return *this;
	}

	TypeBuilder& ReferenceCounting(const asSFuncPtr& addRef, const asSFuncPtr& release)
	{
		int flag = (addRef.flag == 3 ? asCALL_THISCALL : asCALL_CDECL_OBJFIRST);
		CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void f()", addRef, flag));
		flag = (release.flag == 3 ? asCALL_THISCALL : asCALL_CDECL_OBJFIRST);
		CHECKED(engine->RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void f()", release, flag));
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

	NamespaceBuilder WithNamespace();

private:
	cstring name;
	asIScriptEngine* engine;
};

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
class GlobalFunctionBuilder
{
public:
	GlobalFunctionBuilder(asIScriptEngine* engine) : engine(engine) {}

	void AddFunction(cstring decl, const asSFuncPtr& funcPointer, void* auxiliary = nullptr)
	{
		if(auxiliary && funcPointer.flag == 3)
		{
			CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_THISCALL_ASGLOBAL, auxiliary));
		}
		else
		{
			CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_CDECL));
		}
	}

	void AddGenericFunction(cstring decl, const asSFuncPtr& funcPointer)
	{
		CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_GENERIC));
	}

	void AddEnum(cstring name, std::initializer_list<pair<cstring, int>> const& values)
	{
		CHECKED(engine->RegisterEnum(name));
		for(auto& pair : values)
		{
			CHECKED(engine->RegisterEnumValue(name, pair.first, pair.second));
		}
	}

	void AddProperty(cstring decl, void* ptr)
	{
		CHECKED(engine->RegisterGlobalProperty(decl, ptr));
	}

protected:
	asIScriptEngine* engine;
};

//-----------------------------------------------------------------------------
class NamespaceBuilder : public GlobalFunctionBuilder
{
public:
	NamespaceBuilder(asIScriptEngine* engine, cstring ns, void* auxiliary) : GlobalFunctionBuilder(engine), auxiliary(auxiliary) { SetNamespace(ns); }
	~NamespaceBuilder() { SetNamespace(""); }

	NamespaceBuilder& AddFunction(cstring decl, const asSFuncPtr& funcPointer)
	{
		GlobalFunctionBuilder::AddFunction(decl, funcPointer, auxiliary);
		return *this;
	}

	NamespaceBuilder& AddProperty(cstring decl, void* ptr)
	{
		GlobalFunctionBuilder::AddProperty(decl, ptr);
		return *this;
	}

private:
	void SetNamespace(cstring ns) { CHECKED(engine->SetDefaultNamespace(ns)); }
	void* auxiliary;
};

//-----------------------------------------------------------------------------
class ScriptBuilder : public GlobalFunctionBuilder
{
public:
	ScriptBuilder(asIScriptEngine* engine) : GlobalFunctionBuilder(engine) {}

	ScriptBuilder& AddFunction(cstring decl, const asSFuncPtr& funcPointer)
	{
		GlobalFunctionBuilder::AddFunction(decl, funcPointer, nullptr);
		return *this;
	}

	TypeBuilder AddType(cstring name)
	{
		CHECKED(engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT));
		return TypeBuilder(name, engine);
	}

	template<typename T>
	SpecificTypeBuilder<T> AddStruct(cstring name, int extra_flags = 0)
	{
		CHECKED(engine->RegisterObjectType(name, sizeof(T), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<T>() | extra_flags));
		return SpecificTypeBuilder<T>(name, engine);
	}

	TypeBuilder ForType(cstring name)
	{
		return TypeBuilder(name, engine);
	}

	NamespaceBuilder BeginNamespace(cstring ns, void* auxiliary = nullptr)
	{
		return NamespaceBuilder(engine, ns, auxiliary);
	}
};

//-----------------------------------------------------------------------------
inline NamespaceBuilder TypeBuilder::WithNamespace()
{
	return NamespaceBuilder(engine, name, nullptr);
}
