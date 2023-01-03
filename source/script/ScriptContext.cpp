#include "Pch.h"
#include "ScriptContext.h"

#include "ScriptException.h"

void ScriptContext::Clear()
{
	pc = nullptr;
	target = nullptr;
	stock = nullptr;
	quest = nullptr;
}

Quest2* ScriptContext::GetQuest()
{
	if(!quest)
		throw ScriptException("Method must be called from quest.");
	return quest;
}
