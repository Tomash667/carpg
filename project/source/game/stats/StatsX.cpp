#include "Pch.h"
#include "Core.h"
#include "StatsX.h"
#include "Const.h"
#include "BitStreamFunc.h"
#include "UnitData.h"

static std::set<StatsX::Entry, StatsX::Entry> statsx_entries;
const float StatsX::attrib_apt_mod[] = {
	0.f,
	0.5f,
	1.f,
	2.f,
	2.15f,
	2.3f,
	2.35f,
	2.4f
};

const float StatsX::player_apt_mod[] = {
	0,25, // -4
	0.4f, // -3
	0.55f, // -2
	0.75f, // -1
	1.f, // 0
	1.25f, // +1
	1.45f, // +2
	1.6f, // +3
	1.7f, // +4
	1.75f // +5
};

const Vec2 StatsX::skill_apt_mod[] = {
	Vec2(0.f, 0.f),
	Vec2(0.2f, 1.5f),
	Vec2(0.4f, 2.5f),
	Vec2(0.6f, 3.5f),
	Vec2(0.8f, 4.f),
	Vec2(1.f, 4.5f),
	Vec2(1.2f, 4.75f),
	Vec2(1.4f, 5.f)
};

//=================================================================================================
void StatsX::Upgrade()
{
	// get best skills
	TopN<Skill, 3> top(Skill::NONE, -1);
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(skill[i] >= 0)
			top.Add((Skill)i, skill[i]);
	}

	// set base values, increase tag skills
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		attrib_base[i] = profile->attrib[i];
		attrib_apt[i] = AttributeToAptitude(attrib_base[i]);
	}
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		skill_base[i] = profile->skill[i];
		if(top.Is((Skill)i))
		{
			int value = (profile->ShouldDoubleSkill((Skill)i)) ? 10 : 5;
			skill_base[i] += value;
		}
		skill_apt[i] = SkillToAptitude(skill_base[i]);
	}

	// calculate level
	if(seed.level == (byte)-1)
		seed.level = (int)CalculateLevel();
	int level = seed.level;

	// fill base values
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		if(attrib[i] != -1)
			continue;
		int apt = max(0, attrib_apt[i]);
		attrib[i] = attrib_base[i] + (int)(attrib_apt_mod[apt] * level);
	}
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(skill[i] != -1)
			continue;
		int apt = max(0, skill_apt[i]);
		skill[i] = skill_base[i] + (int)(skill_apt_mod[apt].y * level);
	}

	unique = true;
	subprofile = nullptr;
	seed.subprofile = 0;
	seed.variant = 0;
}

//=================================================================================================
float StatsX::CalculateLevel()
{
	// attributes
	float attrib_level = 0.f;
	int attrib_count = 0;
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		if(attrib[i] != -1 && attrib_apt[i] > 0)
		{
			attrib_count += attrib_apt[i];
			attrib_level += float(attrib[i] - attrib_base[i]) / attrib_apt_mod[attrib_apt[i]] * attrib_apt[i];
		}
	}
	if(attrib_count > 0)
		attrib_level /= attrib_count;

	// get best 5 combat skills
	TopN<Skill, 5> top(Skill::NONE, -1);
	for(int i = 0; i <= (int)Skill::HEAVY_ARMOR; ++i)
		top.Add((Skill)i, skill[i]);

	// skill
	float skill_level = 0.f;
	float skill_count = 0.f;
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(skill[i] != -1 && skill_apt[i] > 0)
		{
			float weight;
			if(Any((Skill)i, top.best[0], top.best[1], top.best[2]))
				weight = 1.f;
			else if((Skill)i == top.best[3])
				weight = 0.75f;
			else if((Skill)i == top.best[4])
				weight = 0.5f;
			else if(Any((Skill)i, Skill::HAGGLE, Skill::LITERACY))
				weight = 0.1f;
			else if(Any((Skill)i, Skill::ATHLETICS, Skill::ACROBATICS))
				weight = 0.2f;
			else
				weight = 0;
			if(weight > 0)
			{
				float v = (skill[i] - skill_base[i]) / skill_apt_mod[skill_apt[i]].y;
				v *= weight;
				skill_level += v;
				skill_count += weight;
			}
		}
	}
	if(skill_count > 0)
		skill_level /= skill_count;

	float level = (attrib_level + skill_level) / 2;
	level = floor(level * 10) / 10;
	return min(level, (float)MAX_LEVEL);
}

//=================================================================================================
void StatsX::Save(FileWriter& f)
{
	if(profile->id.empty())
		f << Format("!%s", profile->unit_data->id.c_str());
	else
		f << profile->id;
	f << unique;
	if(unique)
	{
		f << attrib;
		f << attrib_base;
		f << attrib_apt;
		f << skill;
		f << skill_base;
		f << skill_apt;
	}
	else
		f << seed;
}

//=================================================================================================
StatsX* StatsX::Load(FileReader& f)
{
	f.ReadStringBUF();
	StatProfile* profile;
	if(BUF[0] == '!')
	{
		auto unit_data = UnitData::Get(BUF + 1);
		profile = unit_data->stat_profile;
	}
	else
		profile = StatProfile::Get(BUF);
	bool unique;
	f >> unique;
	if(unique)
	{
		auto stats = new StatsX;
		stats->unique = true;
		stats->profile = profile;
		f >> stats->attrib;
		f >> stats->attrib_base;
		f >> stats->attrib_apt;
		f >> stats->skill;
		f >> stats->skill_base;
		f >> stats->skill_apt;
		return stats;
	}
	else
	{
		Entry e;
		e.profile = profile;
		f >> e.seed;
		return Get(e);
	}
}

//=================================================================================================
void StatsX::Write(BitStream& stream)
{
	stream.Write(attrib);
	stream.Write(skill);
}

//=================================================================================================
bool StatsX::Read(BitStream& stream)
{
	return stream.Read(attrib)
		&& stream.Read(skill);
}

//=================================================================================================
bool StatsX::Skip(BitStream& stream)
{
	return ::Skip(stream, sizeof(attrib) + sizeof(skill));
}

//=================================================================================================
StatsX* StatsX::GetLevelUp()
{
	assert(seed.level != MAX_LEVEL);

	// build entry
	Entry e;
	e.profile = profile;
	e.seed = seed;
	e.seed.level++;

	return Get(e);
}

//=================================================================================================
int StatsX::AttributeToAptitude(int value)
{
	return Clamp((value - 50) / 5, -4, (int)countof(attrib_apt_mod) - 1);
}

//=================================================================================================
int StatsX::SkillToAptitude(int value)
{
	return Clamp(value / 5, -4, (int)countof(skill_apt_mod) - 1);
}

//=================================================================================================
StatsX* StatsX::GetRandom(StatProfile* profile, int level)
{
	assert(profile);
	assert(InRange(level, 0, MAX_LEVEL));

	// build entry
	Entry e;
	e.profile = profile;
	e.seed.value = 0u;
	e.seed.level = level;
	if(!profile->subprofiles.empty())
	{
		e.seed.subprofile = (byte)(Rand() % profile->subprofiles.size());
		auto& sub = *profile->subprofiles[e.seed.subprofile];
		if(sub.variants > 1u)
			e.seed.variant = Rand() % sub.variants;
	}

	return Get(e);
}

//=================================================================================================
StatsX* StatsX::Get(Entry& e)
{
	// search for existing stats
	auto it = statsx_entries.lower_bound(e);
	if(it != statsx_entries.end() && !(statsx_entries.key_comp()(e, *it)))
		return it->stats;

	// build stats
	e.stats = new StatsX;
	e.stats->unique = false;
	e.stats->profile = e.profile;
	e.stats->seed = e.seed;
	e.stats->perk_flags = 0;
	if(e.profile->subprofiles.empty())
	{
		e.stats->subprofile = nullptr;
		e.stats->weapon = Skill::WEAPON_PROFILE;
		e.stats->armor = Skill::ARMOR_PROFILE;
	}
	else
	{
		e.stats->subprofile = e.profile->subprofiles[e.seed.subprofile];
		if(e.stats->subprofile->weapons.empty())
			e.stats->weapon = Skill::WEAPON_PROFILE;
		else
			e.stats->weapon = e.stats->subprofile->weapons[e.seed.variant % e.stats->subprofile->weapons.size()];
		if(e.stats->subprofile->armors.empty())
			e.stats->armor = Skill::ARMOR_PROFILE;
		else
			e.stats->armor = e.stats->subprofile->armors[e.seed.variant % e.stats->subprofile->armors.size()];
	}

	e.stats->ApplyBase(e.profile);

	// perks / tag skills
	if(e.stats->subprofile)
	{
		for(auto& se : e.stats->subprofile->skills)
		{
			Skill skill = se.skill;
			if(skill == Skill::WEAPON_PROFILE)
				skill = e.stats->weapon;
			else if(skill == Skill::ARMOR_PROFILE)
				skill = e.stats->armor;
			if(se.tag)
			{
				int value = (IS_SET(e.profile->flags, StatProfile::F_DOUBLE_WEAPON_TAG_SKILL) ? 10 : 5);
				e.stats->skill[(int)skill] += value;
				e.stats->skill_apt[(int)skill] = SkillToAptitude(e.stats->skill[(int)skill]);
			}
			e.stats->skill_tag[(int)skill] = se.value;
		}
	}

	// calculate stats
	int level = e.seed.level;
	if(level != 0)
	{
		for(int i = 0; i < (int)Attribute::MAX; ++i)
		{
			int apt = max(0, e.stats->attrib_apt[i]);
			e.stats->attrib[i] += (int)(attrib_apt_mod[apt] * level);
		}
		for(int i = 0; i < (int)Skill::MAX; ++i)
		{
			int apt = max(0, e.stats->skill_apt[i]);
			e.stats->skill[i] += (int)(skill_apt_mod[apt].Lerp(e.stats->skill_tag[i]) * level);
		}
	}

	// insert and return
	statsx_entries.insert(it, e);
	return e.stats;
}

//=================================================================================================
void StatsX::Cleanup()
{
	for(auto& e : statsx_entries)
		delete e.stats;
}

//=================================================================================================
void StatsX::ApplyBase(StatProfile* profile)
{
	// base value / aptitude
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		attrib[i] = profile->attrib[i];
		attrib_base[i] = attrib[i];
		attrib_apt[i] = AttributeToAptitude(profile->attrib[i]);
	}
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		skill[i] = profile->skill[i];
		skill_base[i] = skill[i];
		skill_apt[i] = SkillToAptitude(profile->skill[i]);
		skill_tag[i] = 0.f;
	}
}
