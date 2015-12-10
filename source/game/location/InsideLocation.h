// podziemia
#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "InsideLocationLevel.h"

//-----------------------------------------------------------------------------
struct InsideLocation : public Location
{
	int target, special_room;
	bool from_portal;

	InsideLocation() : Location(false), special_room(-1), from_portal(false)
	{

	}

	virtual void SetActiveLevel(int _level) = 0;
	virtual bool HaveUpStairs() const = 0;
	virtual bool HaveDownStairs() const = 0;
	virtual InsideLocationLevel& GetLevelData() = 0;
	virtual bool IsMultilevel() const = 0;
	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);
	// return last level data if it was generated
	virtual InsideLocationLevel* GetLastLevelData() = 0;

	inline Room* FindChaseRoom(const VEC3& pos)
	{
		InsideLocationLevel& lvl = GetLevelData();
		if(lvl.rooms.size() < 2)
			return nullptr;
		else
			return lvl.GetNearestRoom(pos);
	}

	virtual Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) = 0;
	virtual Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr) = 0;
	bool RemoveItemFromChest(const Item* item, int& at_level);
	bool RemoveQuestItemFromChest(int quest_refid, int& at_level);
};
