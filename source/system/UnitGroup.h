#pragma once

//-----------------------------------------------------------------------------
struct UnitGroup
{
	enum Special
	{
		S_NONE,
		S_EMPTY,
		S_CHALLANGE
	};

	struct Entry
	{
		union
		{
			UnitData* ud;
			UnitGroup* group;
		};
		int weight;
		bool isLeader;

		Entry() {}
		Entry(UnitData* ud, int weight, bool isLeader) : ud(ud), weight(weight), isLeader(isLeader) {}
		Entry(UnitGroup* group, int weight) : group(group), weight(weight), isLeader(false) {}
	};

	string id, name, name2, name3, encounterText;
	vector<Entry> entries;
	Special special;
	int maxWeight, foodMod, encounterChance;
	bool isList, orcFood, haveCamps, gender;

	UnitGroup() : foodMod(0), encounterChance(0), maxWeight(0), isList(false), orcFood(false), haveCamps(false), special(S_NONE), gender(false) {}
	bool HaveLeader() const;
	bool IsEmpty() const { return special == S_EMPTY; }
	bool IsChallange() const { return special == S_CHALLANGE; }
	UnitData* GetLeader(int level) const;
	UnitData* GetRandomUnit() const;
	Int2 GetLevelRange() const;
	UnitGroup* GetRandomGroup();

	static vector<UnitGroup*> groups;
	static UnitGroup* empty;
	static UnitGroup* random;
	static UnitGroup* TryGet(Cstring id);
	static UnitGroup* Get(Cstring id)
	{
		auto group = TryGet(id);
		assert(group);
		return group;
	}
	static UnitGroup* GetS(const string& id) { return Get(id); }
};

//-----------------------------------------------------------------------------
struct TmpUnitGroup : ObjectPoolProxy<TmpUnitGroup>
{
	typedef pair<UnitData*, int> Spawn;

	vector<UnitGroup::Entry> entries;
	vector<Spawn> spawn;
	int totalWeight, minLevel, maxLevel, refs;

	void AddRefS() { ++refs; }
	void ReleaseS();
	void Fill(UnitGroup* group, int minLevel, int maxLevel, bool required = true);
	void Fill(UnitGroup* group, int level, bool required = true) { Fill(group, Max(level - 5, level / 2), level + 1, required); }
	void FillS(const string& groupId, int count, int level);
	void FillInternal(UnitGroup* group);
	Spawn Get();
	Spawn GetS(uint index) { return spawn[index]; }
	vector<Spawn>& Roll(int level, int count);
	inline uint GetCount() { return spawn.size(); }
	static TmpUnitGroup* GetInstanceS();
};

//-----------------------------------------------------------------------------
struct TmpUnitGroupList
{
	~TmpUnitGroupList();
	void Fill(UnitGroup* list, int level);
	vector<TmpUnitGroup::Spawn>& Roll(int level, int count);

private:
	vector<TmpUnitGroup*> groups;
};

//-----------------------------------------------------------------------------
// pre V_0_11 compatibility
namespace old
{
	enum SPAWN_GROUP
	{
		SG_RANDOM,
		SG_GOBLINS,
		SG_ORCS,
		SG_BANDITS,
		SG_UNDEAD,
		SG_NECROMANCERS,
		SG_MAGES,
		SG_GOLEMS,
		SG_MAGES_AND_GOLEMS,
		SG_EVIL,
		SG_NONE,
		SG_UNKNOWN,
		SG_CHALLANGE,
		SG_MAX
	};

	UnitGroup* OldToNew(SPAWN_GROUP spawn);
}

//-----------------------------------------------------------------------------
inline void operator << (GameWriter& f, UnitGroup* group)
{
	f << group->id;
}
inline void operator >> (GameReader& f, UnitGroup*& group)
{
	if(LOAD_VERSION >= V_0_11)
	{
		const string& id = f.ReadString1();
		group = UnitGroup::Get(id);
	}
	else
	{
		old::SPAWN_GROUP spawn;
		f.Read(spawn);
		group = old::OldToNew(spawn);
	}
}
