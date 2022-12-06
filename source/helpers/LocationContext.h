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
	bool FindUnit(Unit* unit, LocationContext::Entry** entry = nullptr, int* unitIndex = nullptr);
	Unit* FindUnit(UnitData* data, LocationContext::Entry** entry = nullptr, int* unitIndex = nullptr);
	Unit* FindUnit(delegate<bool(Unit*)> clbk, LocationContext::Entry** entry = nullptr, int* unitIndex = nullptr);
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
