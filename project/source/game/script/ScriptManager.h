#pragma once

class asIScriptEngine;
class asIScriptModule;
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

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
	string output;
	bool gather_output;

	PlayerController* pc;
};
