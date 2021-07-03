#pragma once

//-----------------------------------------------------------------------------
struct LocationContext
{
	friend struct ForLocation;

	struct Entry
	{
		LocationPart* locPart;
		int loc, level;
		bool active;
	};

	vector<Entry> entries;

public:
	GroundItem* FindQuestGroundItem(int quest_id, LocationContext::Entry** entry = nullptr, int* item_index = nullptr);
	bool FindUnit(Unit* unit, LocationContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(UnitData* data, LocationContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(delegate<bool(Unit*)> clbk, LocationContext::Entry** entry = nullptr, int* unit_index = nullptr);
	bool RemoveQuestGroundItem(int quest_id);
	bool RemoveUnit(Unit* unit);
};

//-----------------------------------------------------------------------------
struct ForLocation
{
	ForLocation(int loc, int level = -1);
	ForLocation(Location* loc, int level = -1);
	ForLocation(const ForLocation& f) = delete;
	~ForLocation();
	ForLocation& operator = (const ForLocation& f) = delete;
	LocationContext* operator -> () { return ctx; }

private:
	void Setup(Location* loc, int level);

	LocationContext* ctx;
};
