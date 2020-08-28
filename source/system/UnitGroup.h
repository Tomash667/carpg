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
		bool is_leader;

		Entry() {}
		Entry(UnitData* ud, int weight, bool is_leader) : ud(ud), weight(weight), is_leader(is_leader) {}
		Entry(UnitGroup* group, int weight) : group(group), weight(weight), is_leader(false) {}
	};

	string id, name, name2, name3, encounter_text;
	vector<Entry> entries;
	Special special;
	int max_weight, food_mod, encounter_chance;
	bool is_list, orc_food, have_camps, gender;

	UnitGroup() : food_mod(0), encounter_chance(0), max_weight(0), is_list(false), orc_food(false), have_camps(false), special(S_NONE), gender(false) {}
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
	int total_weight, min_level, max_level, refs;

	void AddRefS() { ++refs; }
	void ReleaseS();
	void Fill(UnitGroup* group, int min_level, int max_level, bool required = true);
	void Fill(UnitGroup* group, int level, bool required = true) { Fill(group, Max(level - 5, level / 2), level + 1, required); }
	void FillS(const string& group_id, int count, int level);
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
