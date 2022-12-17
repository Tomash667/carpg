#include "Pch.h"
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
		if(entry.isLeader)
			return true;
	}
	return false;
}

//=================================================================================================
UnitData* UnitGroup::GetLeader(int level) const
{
	int best = -1, bestDif, index = 0;
	for(const UnitGroup::Entry& entry : entries)
	{
		if(entry.isLeader)
		{
			int dif = entry.ud->GetLevelDif(level);
			if(best == -1 || bestDif > dif)
			{
				best = index;
				bestDif = dif;
			}
		}
		++index;
	}
	return best == -1 ? nullptr : entries[best].ud;
}

//=================================================================================================
UnitData* UnitGroup::GetRandomUnit() const
{
	int a = Rand() % maxWeight, b = 0;
	for(const Entry& entry : entries)
	{
		b += entry.weight;
		if(a < b)
			return entry.ud;
	}
	return entries[0].ud;
}

//=================================================================================================
Int2 UnitGroup::GetLevelRange() const
{
	Int2 levelRange(99, -99);
	for(const Entry& entry : entries)
	{
		if(entry.ud->level.x < levelRange.x)
			levelRange.x = entry.ud->level.x;
		if(entry.ud->level.y > levelRange.y)
			levelRange.y = entry.ud->level.y;
	}
	return levelRange;
}

//=================================================================================================
UnitGroup* UnitGroup::GetRandomGroup()
{
	assert(isList);
	if(entries.size() == maxWeight)
		return RandomItem(entries).group;
	else
	{
		int a = Rand() % maxWeight, b = 0;
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
void TmpUnitGroup::Fill(UnitGroup* group, int minLevel, int maxLevel, bool required)
{
	assert(group && minLevel <= maxLevel);
	this->minLevel = minLevel;
	this->maxLevel = maxLevel;
	totalWeight = 0;
	entries.clear();

	FillInternal(group);

	if(entries.empty() && required)
	{
		// if level is too low pick lowest possible in this unit group
		// if level is too high pick highest possible in this unit group
		Int2 levelRange = group->GetLevelRange();
		if(levelRange.y > minLevel)
		{
			// too low location level
			this->minLevel = levelRange.x;
			this->maxLevel = levelRange.x + 1;
		}
		else
		{
			// too high location level
			this->minLevel = levelRange.y - 1;
			this->maxLevel = levelRange.y;
		}

		FillInternal(group);
		assert(!entries.empty());
	}
}

//=================================================================================================
void TmpUnitGroup::FillS(const string& groupId, int count, int level)
{
	UnitGroup* group = UnitGroup::TryGet(groupId);
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
		if(entry.ud->level.y >= minLevel && entry.ud->level.x <= maxLevel)
		{
			UnitGroup::Entry& newEntry = Add1(entries);
			newEntry.ud = entry.ud;
			newEntry.weight = entry.weight;
			totalWeight += newEntry.weight;
		}
	}
}

//=================================================================================================
TmpUnitGroup::Spawn TmpUnitGroup::Get()
{
	int x = Rand() % totalWeight, y = 0;
	for(UnitGroup::Entry& entry : entries)
	{
		y += entry.weight;
		if(x < y)
		{
			int unitLvl = entry.ud->level.Random();
			if(unitLvl < minLevel)
				unitLvl = minLevel;
			else if(unitLvl > maxLevel)
				unitLvl = maxLevel;

			return std::make_pair(entry.ud, unitLvl);
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
	while(points > 0 && spawn.size() < (uint)count * 2)
	{
		int x = Rand() % totalWeight, y = 0;
		for(UnitGroup::Entry& entry : entries)
		{
			y += entry.weight;
			if(x < y)
			{
				// chose unit level
				int unitLvl = entry.ud->level.Random();
				if(unitLvl < minLevel)
					unitLvl = minLevel;
				else
				{
					if(unitLvl > maxLevel)
						unitLvl = maxLevel;
					if(unitLvl > points)
						unitLvl = Max(points, entry.ud->level.x);
				}

				if(unitLvl >= points)
				{
					// lower other units level if possible to spawn last unit
					int pointsRequired = unitLvl - points;
					int pointAvailable = 0;
					for(Spawn& s : spawn)
						pointAvailable += s.second - Max(minLevel, s.first->level.x);
					if(pointAvailable < pointsRequired)
					{
						points = 0;
						break;
					}
					for(Spawn& s : spawn)
					{
						int p = s.second - Max(minLevel, s.first->level.x);
						if(p > 0)
						{
							int r = Min(p, pointsRequired);
							s.second -= r;
							pointsRequired -= r;
							if(pointsRequired == 0)
								break;
						}
					}
				}

				spawn.push_back(std::make_pair(entry.ud, unitLvl));
				points -= unitLvl;
				break;
			}
		}
	}

	if(spawn.empty())
	{
		// add anything with lowest level
		int minLevel = 99, minIndex = -1, index = 0;
		for(UnitGroup::Entry& entry : entries)
		{
			if(entry.ud->level.x < minLevel)
			{
				minLevel = entry.ud->level.x;
				minIndex = index;
			}
			++index;
		}
		UnitGroup::Entry& entry = entries[minIndex];
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
	assert(group->isList);
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
		int bestDif = -1;
		for(UnitGroup::Entry& entry : group->entries)
		{
			Int2 levelRange = entry.group->GetLevelRange();
			int dif;
			if(level < levelRange.x)
				dif = levelRange.x - level;
			else
				dif = level - levelRange.y;
			if(bestDif == -1 || dif < bestDif)
			{
				bestDif = dif;
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
