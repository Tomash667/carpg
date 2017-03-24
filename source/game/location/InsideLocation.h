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
	
	// from Location
	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;

	virtual void SetActiveLevel(int _level) = 0;
	virtual bool HaveUpStairs() const = 0;
	virtual bool HaveDownStairs() const = 0;
	virtual InsideLocationLevel& GetLevelData() = 0;
	virtual bool IsMultilevel() const = 0;
	// return last level data if it was generated
	virtual InsideLocationLevel* GetLastLevelData() = 0;
	virtual Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) = 0;
	virtual Chest* FindChestWithQuestItem(int quest_refid, int& at_level, int* index = nullptr) = 0;

	bool RemoveItemFromChest(const Item* item, int& at_level);
	bool RemoveQuestItemFromChest(int quest_refid, int& at_level);
	Room* FindChaseRoom(const VEC3& _pos)
	{
		InsideLocationLevel& lvl = GetLevelData();
		if(lvl.rooms.size() < 2)
			return nullptr;
		else
			return lvl.GetNearestRoom(_pos);
	}
};
