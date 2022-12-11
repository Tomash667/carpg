#include "Pch.h"
#include "ScriptContext.h"

#include "ScriptException.h"

void ScriptContext::Clear()
{
	pc = nullptr;
	target = nullptr;
	stock = nullptr;
	quest = nullptr;
	prevQuest = nullptr;
}

Quest2* ScriptContext::GetQuest()
{
	if(!quest)
		throw ScriptException("Method must be called from quest.");
	return quest;
}

void ScriptContext::SetQuest(Quest2* newQuest)
{
	if(newQuest)
	{
		// not implemented, it is currently unsafe to call from quest A -> B -> A because callDepth will ignore setting
		// quest again to A, for now only 2 level call "stack" allowed
		assert(!prevQuest);

		prevQuest = quest;
		quest = newQuest;
	}
	else
	{
		assert(quest);
		quest = prevQuest;
		prevQuest = nullptr;
	}
}
