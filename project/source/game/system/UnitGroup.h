#pragma once

//-----------------------------------------------------------------------------
struct UnitData;

//-----------------------------------------------------------------------------
struct UnitGroup
{
	struct Entry
	{
		UnitData* ud;
		int count;

		Entry() {}
		Entry(UnitData* ud, int count) : ud(ud), count(count) {}
	};

	string id;
	vector<Entry> entries;
	UnitData* leader;
	int total;

	static vector<UnitGroup*> groups;
	static UnitGroup* TryGet(const AnyString& id);
};

//-----------------------------------------------------------------------------
struct TmpUnitGroup
{
	UnitGroup* group;
	vector<UnitGroup::Entry> entries;
	int total, max_level;
};
