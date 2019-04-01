#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
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
	Location* location;
	Unit* unit;
	GroundItem* item;

	ScriptEvent(EventType type) : type(type), location(nullptr), unit(nullptr), item(nullptr) {}
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
class ScriptManager : public GameComponent
{
public:
	ScriptManager();
	void InitOnce() override;
	void Cleanup() override;
	void RegisterCommon();
	void RegisterGame();
	void SetException(cstring ex) { last_exception = ex; }
	bool RunScript(cstring code, bool validate = false);
	bool RunIfScript(cstring code, bool validate = false);
	asIScriptFunction* PrepareScript(cstring name, cstring code);
	bool RunScript(asIScriptFunction* func, void* instance = nullptr, delegate<void(asIScriptContext*, int)> clbk = nullptr);
	string& OpenOutput();
	void CloseOutput();
	void Log(Logger::Level level, cstring msg, cstring code = nullptr);
	void AddFunction(cstring decl, const asSFuncPtr& funcPointer);
	// add enum with values {name, value}
	void AddEnum(cstring name, std::initializer_list<pair<cstring, int>> const& values);
	TypeBuilder AddType(cstring name, bool refcount = false);
	TypeBuilder ForType(cstring name);
	VarsContainer* GetVars(Unit* unit);
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
	std::unordered_map<Unit*, VarsContainer*> unit_vars;
	ScriptContext ctx;
};
extern ScriptManager SM;
