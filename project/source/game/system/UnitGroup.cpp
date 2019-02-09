#include "Pch.h"
#include "GameCore.h"
#include "UnitGroup.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
vector<UnitGroup*> UnitGroup::groups;
vector<UnitGroupList*> UnitGroupList::lists;

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
UnitGroupList* UnitGroupList::TryGet(Cstring id)
{
	for(UnitGroupList* list : lists)
	{
		if(list->id == id.s)
			return list;
	}
	return nullptr;
}

//=================================================================================================
void TmpUnitGroup::Fill(UnitGroup* group, int min_level, int max_level)
{
	assert(group && min_level <= max_level);
	this->min_level = min_level;
	this->max_level = max_level;
	total = 0;
	entries.clear();

	FillInternal(group);

	if(entries.empty())
	{
		// if level is too low pick lowest possible in this unit group
		// if level is too high pick highest possible in this unit group
		Int2 level_range(99, -99);
		for(UnitGroup::Entry& entry : group->entries)
		{
			if(entry.ud->level.x < level_range.x)
				level_range.x = entry.ud->level.x;
			if(entry.ud->level.y > level_range.y)
				level_range.y = entry.ud->level.y;
		}

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
void TmpUnitGroup::FillInternal(UnitGroup* group)
{
	for(UnitGroup::Entry& entry : group->entries)
	{
		if(entry.ud->level.y >= min_level && entry.ud->level.x <= max_level)
		{
			UnitGroup::Entry& new_entry = Add1(entries);
			new_entry.ud = entry.ud;
			new_entry.count = entry.count;
			total += new_entry.count;
		}
	}
}

//=================================================================================================
TmpUnitGroup::Spawn TmpUnitGroup::Get()
{
	int x = Rand() % total, y = 0;
	for(UnitGroup::Entry& entry : entries)
	{
		y += entry.count;
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
	int points = level * (count - 1) + Random(count / 2, count);
	while(points > 0 && spawn.size() < (uint)count *2)
	{
		int x = Rand() % total, y = 0;
		for(UnitGroup::Entry& entry : entries)
		{
			y += entry.count;
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
	return spawn;
}
