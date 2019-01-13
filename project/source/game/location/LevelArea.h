#pragma once

//-----------------------------------------------------------------------------
class LevelArea
{
public:
	vector<Unit*> units;
	vector<GroundItem*> items;
};

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
	GroundItem* FindQuestGroundItem(int quest_refid, LevelAreaContext::Entry** entry = nullptr, int* item_index = nullptr);
	Unit* FindUnitWithQuestItem(int quest_refid, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr, int* item_iindex = nullptr);
	bool FindUnit(Unit* unit, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(UnitData* data, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(delegate<bool(Unit*)> clbk, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	bool RemoveQuestGroundItem(int quest_refid);
	bool RemoveQuestItemFromUnit(int quest_refid);
	bool RemoveUnit(Unit* unit);
};

//-----------------------------------------------------------------------------
struct ForLocation
{
	ForLocation(int loc, int level = -1);
	ForLocation(const ForLocation& f) = delete;
	~ForLocation();
	ForLocation& operator = (const ForLocation& f) = delete;
	LevelAreaContext* operator -> () { return ctx; }

private:
	LevelAreaContext* ctx;
};
