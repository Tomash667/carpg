#include "Pch.h"
#include "GameCore.h"
#include "UnitGroup.h"
#include "UnitData.h"
#include "ScriptException.h"

//-----------------------------------------------------------------------------
vector<UnitGroup*> UnitGroup::groups;
UnitGroup* UnitGroup::empty;
UnitGroup* UnitGroup::random;

//=================================================================================================
bool UnitGroup::HaveLeader() const
{
	for(const UnitGroup::Entry& entry : entries)
	{
		if(entry.is_leader)
			return true;
	}
	return false;
}

//=================================================================================================
UnitData* UnitGroup::GetLeader(int level) const
{
	int best = -1, best_dif, index = 0;
	for(const UnitGroup::Entry& entry : entries)
	{
		if(entry.is_leader)
		{
			int dif = entry.ud->GetLevelDif(level);
			if(best == -1 || best_dif > dif)
			{
				best = index;
				best_dif = dif;
			}
		}
		++index;
	}
	return best == -1 ? nullptr : entries[best].ud;
}

//=================================================================================================
Int2 UnitGroup::GetLevelRange() const
{
	Int2 level_range(99, -99);
	for(const Entry& entry : entries)
	{
		if(entry.ud->level.x < level_range.x)
			level_range.x = entry.ud->level.x;
		if(entry.ud->level.y > level_range.y)
			level_range.y = entry.ud->level.y;
	}
	return level_range;
}

//=================================================================================================
UnitGroup* UnitGroup::GetRandomGroup()
{
	assert(is_list);
	if(entries.size() == max_weight)
		return RandomItem(entries).group;
	else
	{
		int a = Rand() % max_weight, b = 0;
		for(const Entry& entry : entries)
		{
			b += entry.weight;
			if(a < b)
				return entry.group;
		}
		return entries[0].group;
	}
}

//=================================================================================================
UnitGroup* UnitGroup::TryGet(Cstring id)
{
	for(UnitGroup* group : groups)
	{
		if(group->id == id.s)
			return group;
	}
	return nullptr;
}


//=================================================================================================
void TmpUnitGroup::ReleaseS()
{
	if(--refs == 0)
		Free();
}

//=================================================================================================
void TmpUnitGroup::Fill(UnitGroup* group, int min_level, int max_level, bool required)
{
	assert(group && min_level <= max_level);
	this->min_level = min_level;
	this->max_level = max_level;
	total_weight = 0;
	entries.clear();

	FillInternal(group);

	if(entries.empty() && required)
	{
		// if level is too low pick lowest possible in this unit group
		// if level is too high pick highest possible in this unit group
		Int2 level_range = group->GetLevelRange();
		if(level_range.y > min_level)
		{
			// too low location level
			this->min_level = level_range.x;
			this->max_level = level_range.x + 1;
		}
		else
		{
			// too high location level
			this->min_level = level_range.y - 1;
			this->max_level = level_range.y;
		}

		FillInternal(group);
		assert(!entries.empty());
	}
}

//=================================================================================================
void TmpUnitGroup::FillS(const string& group_id, int count, int level)
{
	UnitGroup* group = UnitGroup::TryGet(group_id);
	if(!group)
		throw ScriptException("");
	Fill(group, level);
	Roll(level, count);
}

//=================================================================================================
void TmpUnitGroup::FillInternal(UnitGroup* group)
{
	for(UnitGroup::Entry& entry : group->entries)
	{
		if(entry.ud->level.y >= min_level && entry.ud->level.x <= max_level)
		{
			UnitGroup::Entry& new_entry = Add1(entries);
			new_entry.ud = entry.ud;
			new_entry.weight = entry.weight;
			total_weight += new_entry.weight;
		}
	}
}

//=================================================================================================
TmpUnitGroup::Spawn TmpUnitGroup::Get()
{
	int x = Rand() % total_weight, y = 0;
	for(UnitGroup::Entry& entry : entries)
	{
		y += entry.weight;
		if(x < y)
		{
			int unit_lvl = entry.ud->level.Random();
			if(unit_lvl < min_level)
				unit_lvl = min_level;
			else if(unit_lvl > max_level)
				unit_lvl = max_level;

			return std::make_pair(entry.ud, unit_lvl);
		}
	}
	return Spawn(nullptr, 0);
}

//=================================================================================================
vector<TmpUnitGroup::Spawn>& TmpUnitGroup::Roll(int level, int count)
{
	spawn.clear();
	if(entries.empty())
		return spawn;

	int points = level * count + Random(-level / 2, level / 2);
	while(points > 0 && spawn.size() < (uint)count *2)
	{
		int x = Rand() % total_weight, y = 0;
		for(UnitGroup::Entry& entry : entries)
		{
			y += entry.weight;
			if(x < y)
			{
				// chose unit level
				int unit_lvl = entry.ud->level.Random();
				if(unit_lvl < min_level)
					unit_lvl = min_level;
				else
				{
					if(unit_lvl > max_level)
						unit_lvl = max_level;
					if(unit_lvl > points)
						unit_lvl = Max(points, entry.ud->level.x);
				}

				if(unit_lvl >= points)
				{
					// lower other units level if possible to spawn last unit
					int points_required = unit_lvl - points;
					int points_available = 0;
					for(Spawn& s : spawn)
						points_available += s.second - Max(min_level, s.first->level.x);
					if(points_available < points_required)
					{
						points = 0;
						break;
					}
					for(Spawn& s : spawn)
					{
						int p = s.second - Max(min_level, s.first->level.x);
						if(p > 0)
						{
							int r = Min(p, points_required);
							s.second -= r;
							points_required -= r;
							if(points_required == 0)
								break;
						}
					}
				}

				spawn.push_back(std::make_pair(entry.ud, unit_lvl));
				points -= unit_lvl;
				break;
			}
		}
	}

	if(spawn.empty())
	{
		// add anything with lowest level
		int min_level = 99, min_index = -1, index = 0;
		for(UnitGroup::Entry& entry : entries)
		{
			if(entry.ud->level.x < min_level)
			{
				min_level = entry.ud->level.x;
				min_index = index;
			}
			++index;
		}
		UnitGroup::Entry& entry = entries[min_index];
		spawn.push_back(std::make_pair(entry.ud, Max(level, entry.ud->level.x)));
	}

	return spawn;
}

//=================================================================================================
TmpUnitGroup* TmpUnitGroup::GetInstanceS()
{
	TmpUnitGroup* group = ObjectPoolProxy<TmpUnitGroup>::Get();
	group->refs = 1;
	return group;
}


//=================================================================================================
TmpUnitGroupList::~TmpUnitGroupList()
{
	TmpUnitGroup::Free(groups);
}

//=================================================================================================
void TmpUnitGroupList::Fill(UnitGroup* group, int level)
{
	assert(group->is_list);
	TmpUnitGroup* tmp = nullptr;
	for(UnitGroup::Entry& entry : group->entries)
	{
		if(!tmp)
			tmp = ObjectPoolProxy<TmpUnitGroup>::Get();
		tmp->Fill(entry.group, level, false);
		if(!tmp->entries.empty())
		{
			groups.push_back(tmp);
			tmp = nullptr;
		}
	}

	if(groups.empty())
	{
		UnitGroup* best = nullptr;
		int best_diff = -1;
		for(UnitGroup::Entry& entry : group->entries)
		{
			Int2 level_range = entry.group->GetLevelRange();
			int diff;
			if(level < level_range.x)
				diff = level_range.x - level;
			else
				diff = level - level_range.y;
			if(best_diff == -1 || diff < best_diff)
			{
				best_diff = diff;
				best = entry.group;
			}
		}

		if(!tmp)
			tmp = ObjectPoolProxy<TmpUnitGroup>::Get();
		tmp->Fill(best, level);
		groups.push_back(tmp);
	}
	else if(tmp)
		tmp->Free();
}

//=================================================================================================
vector<TmpUnitGroup::Spawn>& TmpUnitGroupList::Roll(int level, int count)
{
	return RandomItem(groups)->Roll(level, count);
}


//=================================================================================================
UnitGroup* old::OldToNew(SPAWN_GROUP spawn)
{
	cstring id;
	switch(spawn)
	{
	case SG_NONE:
		return UnitGroup::empty;
	default:
	case SG_GOBLINS:
		id = "goblins";
		break;
	case SG_ORCS:
		id = "orcs";
		break;
	case SG_BANDITS:
		id = "bandits";
		break;
	case SG_UNDEAD:
		id = "undead";
		break;
	case SG_NECROMANCERS:
		id = "necromancers";
		break;
	case SG_MAGES:
		id = "mages";
		break;
	case SG_GOLEMS:
		id = "golems";
		break;
	case SG_MAGES_AND_GOLEMS:
		id = "mages_and_golems";
		break;
	case SG_EVIL:
		id = "evil";
		break;
	case SG_UNKNOWN:
		id = "unk";
		break;
	case SG_CHALLANGE:
		id = "challange";
		break;
	}
	return UnitGroup::Get(id);
}
