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
struct TmpUnitGroup : ObjectPoolProxy<TmpUnitGroup>
{
	typedef std::pair<UnitData*, int> Spawn;

	vector<UnitGroup::Entry> entries;
	vector<Spawn> spawn;
	int total, min_level, max_level;

	void Fill(UnitGroup* group, int min_level, int max_level);
	void Fill(UnitGroup* group, int level) { Fill(group, Max(level - 5, level / 2), level); }
	Spawn Get();
	vector<Spawn>& Roll(int points);
};

//-----------------------------------------------------------------------------
template<int N>
struct TmpUnitGroupList
{
	void Fill(UnitGroup*(&groups)[N], int level)
	{
		for(int i = 0; i < N; ++i)
			e[i]->Fill(groups[i], level);
	}
	vector<TmpUnitGroup::Spawn>& Roll(int points)
	{
		int index = Rand() % N,
			start = index;
		while(true)
		{
			TmpUnitGroup& group = e[index];
			if(group.total != 0)
				return group.Roll(points);
			index = (index + 1) % N;
			if(index == start)
				return group.spawn;
		}
	}
	Pooled<TmpUnitGroup> e[N];
};
