// skrzynia
#pragma once

//-----------------------------------------------------------------------------
#include "ItemSlot.h"
#include "Animesh.h"
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
	VEC3 pos;
	float rot;
	AnimeshInstance* ani;
	ChestEventHandler* handler;
	int netid;
	bool looted; // czy skrzynia jest ograbiana - nie trzeba zapisywaæ

	static const int MIN_SIZE = 20;

	Chest() : ani(nullptr)
	{

	}
	~Chest()
	{
		delete ani;
	}

	inline bool AddItem(const Item* item, uint count, uint team_count)
	{
		return InsertItem(items, item, count, team_count);
	}
	inline bool AddItem(const Item* item, uint count=1)
	{
		return AddItem(item, count, count);
	}
	inline void RemoveItem(int index)
	{
		items.erase(items.begin() + index);
	}

	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	inline int FindItem(const Item* item) const
	{
		assert(item);
		int index = 0;
		for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
		{
			if(it->item == item)
				return index;
		}
		return -1;
	}
	inline int FindQuestItem(int quest_refid) const
	{
		int index = 0;
		for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
		{
			if(it->item->IsQuest(quest_refid))
				return index;
		}
		return -1;
	}
};
