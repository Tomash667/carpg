#pragma once

//-----------------------------------------------------------------------------
class LevelAreaContext
{
	friend struct ForLocation;

	struct Entry
	{
		LevelArea* area;
		int loc, level;
		bool active;
	};

	vector<Entry> entries;

public:
	GroundItem* FindQuestGroundItem(int quest_id, LevelAreaContext::Entry** entry = nullptr, int* item_index = nullptr);
	bool FindUnit(Unit* unit, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(UnitData* data, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(delegate<bool(Unit*)> clbk, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
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
	LevelAreaContext* operator -> () { return ctx; }

private:
	void Setup(Location* loc, int level);

	LevelAreaContext* ctx;
};
