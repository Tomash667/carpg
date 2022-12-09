#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "InsideLocationLevel.h"

//-----------------------------------------------------------------------------
struct InsideLocation : public Location
{
	int specialRoom;
	bool fromPortal;

	InsideLocation() : Location(false), specialRoom(-1), fromPortal(false)
	{
	}

	// from Location
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	void Write(BitStreamWriter& f) override;
	bool Read(BitStreamReader& f) override;

	virtual void SetActiveLevel(int level) = 0;
	virtual bool HavePrevEntry() const = 0;
	virtual bool HaveNextEntry() const = 0;
	virtual InsideLocationLevel& GetLevelData() = 0;
	virtual bool IsMultilevel() const = 0;
	// return last level data if it was generated
	virtual InsideLocationLevel* GetLastLevelData() = 0;
	virtual Chest* FindChestWithItem(const Item* item, int& atLevel, int* index = nullptr) = 0;
	virtual Chest* FindChestWithQuestItem(int questId, int& atLevel, int* index = nullptr) = 0;

	bool RemoveQuestItemFromChest(int questId, int& atLevel);
	Room* FindChaseRoom(const Vec3& pos)
	{
		InsideLocationLevel& lvl = GetLevelData();
		if(lvl.rooms.size() < 2)
			return nullptr;
		else
			return lvl.GetNearestRoom(pos);
	}
};
