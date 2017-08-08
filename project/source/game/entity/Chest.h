// skrzynia
#pragma once

//-----------------------------------------------------------------------------
#include "ItemSlot.h"
#include "MeshInstance.h"
#include "Item.h"

//-----------------------------------------------------------------------------
struct ChestEventHandler
{
	enum Event
	{
		Opened
	};

	virtual void HandleChestEvent(Event event) = 0;
	virtual int GetChestEventHandlerQuestRefid() = 0;
};

//-----------------------------------------------------------------------------
struct Chest
{
	vector<ItemSlot> items;
	Vec3 pos;
	float rot;
	MeshInstance* mesh_inst;
	ChestEventHandler* handler;
	int netid;
	bool looted; // czy skrzynia jest ograbiana - nie trzeba zapisywa�

	static const int MIN_SIZE = 20;

	Chest() : mesh_inst(nullptr)
	{
	}
	~Chest()
	{
		delete mesh_inst;
	}

	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	int FindItem(const Item* item) const;
	int FindQuestItem(int quest_refid) const;

	bool AddItem(const Item* item, uint count, uint team_count)
	{
		return InsertItem(items, item, count, team_count);
	}
	bool AddItem(const Item* item, uint count = 1)
	{
		return AddItem(item, count, count);
	}
	void RemoveItem(int index)
	{
		items.erase(items.begin() + index);
	}
};
