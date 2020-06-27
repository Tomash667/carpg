#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "InsideLocationLevel.h"

//-----------------------------------------------------------------------------
struct InsideLocation : public Location
{
	int special_room;
	bool from_portal;

	InsideLocation() : Location(false), special_room(-1), from_portal(false)
	{
	}

	// from Location
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	void Write(BitStreamWriter& f) override;
	bool Read(BitStreamReader& f) override;

	virtual void SetActiveLevel(int _level) = 0;
	virtual bool HavePrevEntry() const = 0;
	virtual bool HaveNextEntry() const = 0;
	virtual InsideLocationLevel& GetLevelData() = 0;
	virtual bool IsMultilevel() const = 0;
	// return last level data if it was generated
	virtual InsideLocationLevel* GetLastLevelData() = 0;
	virtual Chest* FindChestWithItem(const Item* item, int& at_level, int* index = nullptr) = 0;
	virtual Chest* FindChestWithQuestItem(int quest_id, int& at_level, int* index = nullptr) = 0;

	bool RemoveQuestItemFromChest(int quest_id, int& at_level);
	Room* FindChaseRoom(const Vec3& _pos)
	{
		InsideLocationLevel& lvl = GetLevelData();
		if(lvl.rooms.size() < 2)
			return nullptr;
		else
			return lvl.GetNearestRoom(_pos);
	}
};
