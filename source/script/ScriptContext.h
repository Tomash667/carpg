#pragma once

//-----------------------------------------------------------------------------
struct ScriptContext
{
	ScriptContext() : pc(nullptr), target(nullptr), stock(nullptr), quest(nullptr) {}
	void Clear();
	Quest2* GetQuest();

	PlayerController* pc;
	Unit* target;
	vector<ItemSlot>* stock;
	Quest2* quest;
};
