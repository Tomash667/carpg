#pragma once

#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

class asIScriptEngine;
class asIScriptModule;
class TypeBuilder;
struct asSFuncPtr;

struct ScriptException
{
	ScriptException(cstring msg);

	template<typename... Args>
	ScriptException(cstring msg, const Args&... args) : ScriptException(Format(msg, args...)) {}
};

class ScriptManager
{
public:
	ScriptManager();
	void Init();
	void Cleanup();
	void RegisterCommon();
	void RegisterGame();
	void SetContext(PlayerController* pc, Unit* target);
	void SetException(cstring ex) { last_exception = ex; }
	bool RunScript(cstring code, bool validate = false);
	bool RunIfScript(cstring code, bool validate = false);
	string& OpenOutput();
	void CloseOutput();
	void Log(Logger::Level level, cstring msg, cstring code = nullptr);
	void AddFunction(cstring decl, const asSFuncPtr& funcPointer);
	TypeBuilder AddType(cstring name);
	TypeBuilder ForType(cstring name);
	VarsContainer* GetVars(Unit* unit);
	Var& GetVar(cstring name);
	void Reset();
	void Save(FileWriter& f);
	void Load(FileReader& f);

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
	string output;
	cstring last_exception;
	bool gather_output;
	std::unordered_map<Unit*, VarsContainer*> unit_vars;

	// context
	PlayerController* pc;
	Unit* target;
};

extern ScriptManager SM;
