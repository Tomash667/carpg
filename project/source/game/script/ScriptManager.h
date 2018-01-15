#pragma once

class asIScriptEngine;
class asIScriptModule;

class ScriptManager
{
public:
	ScriptManager();
	void Init();
	void Release();
	void SetContext(PlayerController* pc);
	bool RunScript(cstring code);

private:
	asIScriptEngine* engine;
	asIScriptModule* module;
};
