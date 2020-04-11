#pragma once

//-----------------------------------------------------------------------------
#include "Var.h"
#include "ScriptException.h"
#include "Event.h"

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
struct ScriptEvent
{
	EventType type;
	union
	{
		struct OnCleared
		{
			Location* location;
		} on_cleared;
		struct OnDie
		{
			Unit* unit;
		} on_die;
		struct OnEncounter
		{
		} on_encounter;
		struct OnEnter
		{
			Location* location;
		} on_enter;
		struct OnGenerate
		{
			Location* location;
			MapSettings* map_settings;
			int stage;
		} on_generate;
		struct OnPickup
		{
			Unit* unit;
			GroundItem* item;
		} on_pickup;
		struct OnTimeout
		{
		} on_timeout;
		struct OnUpdate
		{
			Unit* unit;
		} on_update;
	};
	bool cancel;

	ScriptEvent(EventType type) : type(type), cancel(false) {}
};

//-----------------------------------------------------------------------------
struct ScriptContext
{
	ScriptContext() : pc(nullptr), target(nullptr), stock(nullptr), quest(nullptr) {}

	PlayerController* pc;
	Unit* target;
	vector<ItemSlot>* stock;
	Quest_Scripted* quest;

	void Clear()
	{
		pc = nullptr;
		target = nullptr;
		stock = nullptr;
		quest = nullptr;
	}
	Quest_Scripted* GetQuest()
	{
		if(!quest)
			throw ScriptException("Method must be called from quest.");
		return quest;
	}
};

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
	void SetException(cstring ex) { last_exception = ex; }
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
	void Save(FileWriter& f);
	void Load(FileReader& f);
	enum RegisterResult
	{
		Ok,
		InvalidType,
		AlreadyExists
	};
	RegisterResult RegisterGlobalVar(const string& type, bool is_ref, const string& name);
	ScriptContext& GetContext() { return ctx; }
	asIScriptEngine* GetEngine() { return engine; }
	void AddVarType(Var::Type type, cstring name, bool is_ref);
	Var::Type GetVarType(int type_id);
	bool CheckVarType(int type_id, bool is_ref);
	void ScriptSleep(float time);
	void UpdateScripts(float dt);
	bool ExecuteScript(asIScriptContext* ctx);
	void StopAllScripts();
	asIScriptContext* SuspendScript();
	void ResumeScript(asIScriptContext* ctx);

private:
	struct ScriptTypeInfo
	{
		ScriptTypeInfo() {}
		ScriptTypeInfo(Var::Type type, bool require_ref) : type(type), require_ref(require_ref) {}

		Var::Type type;
		bool require_ref;
	};

	asIScriptEngine* engine;
	asIScriptModule* module;
	string output;
	cstring last_exception;
	bool gather_output;
	std::map<int, ScriptTypeInfo> script_type_infos;
	std::map<string, int> var_type_map;
	std::unordered_map<int, Vars*> unit_vars;
	ScriptContext ctx;
	vector<SuspendedScript> suspended_scripts;
};
