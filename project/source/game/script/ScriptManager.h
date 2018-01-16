#pragma once

#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

class asIScriptEngine;
class asIScriptModule;
class TypeBuilder;
struct PlayerController;

class ScriptManager
{
public:
	ScriptManager();
	~ScriptManager();
	void Init();
	void Register();
	void SetContext(PlayerController* pc);
	bool RunScript(cstring code);
	string& OpenOutput();
	void CloseOutput();
	void Log(Logger::Level level, cstring msg);
	TypeBuilder AddType(cstring name);
	TypeBuilder ForType(cstring name);

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
	string output;
	bool gather_output;

	PlayerController* pc;
};
