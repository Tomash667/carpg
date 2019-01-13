#pragma once

//-----------------------------------------------------------------------------
struct UnitGroup
{
	struct Entry
	{
		UnitData* ud;
		int count;
		bool is_leader;

		Entry() {}
		Entry(UnitData* ud, int count, bool is_leader) : ud(ud), count(count), is_leader(is_leader) {}
	};

	string id;
	vector<Entry> entries;
	int total;

	bool HaveLeader() const;
	UnitData* GetLeader(int level) const;

	static vector<UnitGroup*> groups;
	static UnitGroup* TryGet(Cstring id);
	static UnitGroup* Get(Cstring id)
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
	static UnitGroupList* TryGet(Cstring id);
	static UnitGroupList* Get(Cstring id)
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

	void Fill(int level);
};
