#pragma once

//-----------------------------------------------------------------------------
struct ScriptContext
{
	ScriptContext() : pc(nullptr), target(nullptr), stock(nullptr), quest(nullptr), prevQuest(nullptr) {}
	void Clear();
	Quest2* GetQuest();
	void SetQuest(Quest2* quest);

	PlayerController* pc;
	Unit* target;
	vector<ItemSlot>* stock;
	Quest2* quest, *prevQuest;
};
