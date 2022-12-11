#pragma once

//-----------------------------------------------------------------------------
#include "Event.h"
#include "ScriptContext.h"
#include "ScriptException.h"
#include "Var.h"

//-----------------------------------------------------------------------------
#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

//-----------------------------------------------------------------------------
class TypeBuilder;
class NamespaceBuilder;
struct asSFuncPtr;

//-----------------------------------------------------------------------------
struct SuspendedScript
{
	asIScriptContext* ctx;
	ScriptContext sctx;
	float time;
};

//-----------------------------------------------------------------------------
class ScriptManager
{
public:
	ScriptManager();
	~ScriptManager();
	void Init();
	void RegisterCommon();
	void RegisterGame();
	void SetException(cstring ex) { lastException = ex; }
	void RunScript(cstring code);
	asIScriptFunction* PrepareScript(cstring name, cstring code);
	void RunScript(asIScriptFunction* func, void* instance = nullptr, delegate<void(asIScriptContext*, int)> clbk = nullptr);
	string& OpenOutput();
	void CloseOutput();
	void Log(Logger::Level level, cstring msg, cstring code = nullptr);
	void AddFunction(cstring decl, const asSFuncPtr& funcPointer);
	void AddFunction(cstring decl, const asSFuncPtr& funcPointer, void* obj);
	// add enum with values {name, value}
	void AddEnum(cstring name, std::initializer_list<pair<cstring, int>> const& values);
	template<typename T>
	void AddEnum(cstring name, std::initializer_list<pair<cstring, T>> const& values)
	{
		AddEnum(name, (std::initializer_list<pair<cstring, int>> const&)values);
	}
	TypeBuilder AddType(cstring name, bool refcount = false);
	TypeBuilder ForType(cstring name);
	Vars* GetVars(Unit* unit);
	NamespaceBuilder WithNamespace(cstring name, void* auxiliary = nullptr);
	Var& GetVar(cstring name);
	void Reset();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	enum RegisterResult
	{
		Ok,
		InvalidType,
		AlreadyExists
	};
	RegisterResult RegisterGlobalVar(const string& type, bool isRef, const string& name);
	ScriptContext& GetContext() { return ctx; }
	asIScriptEngine* GetEngine() { return engine; }
	void AddVarType(Var::Type type, cstring name, bool isRef);
	Var::Type GetVarType(int typeId);
	bool CheckVarType(int typeId, bool isRef);
	void ScriptSleep(float time);
	void UpdateScripts(float dt);
	bool ExecuteScript(asIScriptContext* ctx);
	void StopAllScripts();
	asIScriptContext* SuspendScript();
	void ResumeScript(asIScriptContext* ctx);
	void RegisterSharedInstance(QuestScheme* scheme, asIScriptObject* instance)
	{
		assert(scheme && instance);
		sharedInstances.push_back(std::make_pair(scheme, instance));
	}
	asIScriptObject* GetSharedInstance(QuestScheme* scheme);

private:
	struct ScriptTypeInfo
	{
		ScriptTypeInfo() {}
		ScriptTypeInfo(Var::Type type, bool requireRef) : type(type), requireRef(requireRef) {}

		Var::Type type;
		bool requireRef;
	};

	asIScriptEngine* engine;
	asIScriptModule* module;
	string output;
	cstring lastException;
	std::map<int, ScriptTypeInfo> scriptTypeInfos;
	std::map<string, int> varTypeMap;
	std::unordered_map<int, Vars*> unitVars;
	ScriptContext ctx;
	vector<SuspendedScript> suspended_scripts;
	vector<pair<QuestScheme*, asIScriptObject*>> sharedInstances;
	bool gatherOutput;
};
