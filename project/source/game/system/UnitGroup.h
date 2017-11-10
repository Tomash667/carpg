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
	static UnitGroup* Get(const AnyString& id)
	{
		auto group = TryGet(id);
		assert(group);
		return group;
	}
};

//-----------------------------------------------------------------------------
struct UnitGroupList
{
	string id;
	vector<UnitGroup*> groups;

	static vector<UnitGroupList*> lists;
	static UnitGroupList* TryGet(const AnyString& id);
	static UnitGroupList* Get(const AnyString& id)
	{
		auto list = TryGet(id);
		assert(list);
		return list;
	}
};

//-----------------------------------------------------------------------------
struct TmpUnitGroup
{
	UnitGroup* group;
	vector<UnitGroup::Entry> entries;
	int total, max_level;
};
