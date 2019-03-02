#pragma once

struct ScriptException
{
	ScriptException(cstring msg);

	template<typename... Args>
	ScriptException(cstring msg, const Args&... args) : ScriptException(Format(msg, args...)) {}
};
