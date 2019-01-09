#include "Pch.h"
#include "GameCore.h"
#include "UnitStats.h"
#include "BitStreamFunc.h"

std::map<std::pair<StatProfile*, SubprofileInfo>, UnitStats*> UnitStats::shared_stats;

//=================================================================================================
void UnitStats::Set(StatProfile& profile)
{
	int level = subprofile.level;
	assert(level >= 0);
	if(level == 0)
	{
		// used only by player at start
		assert(!fixed);
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attrib[i] = profile.attrib[i];
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skill[i] = profile.skill[i];
	}
	else if(profile.subprofiles.empty())
	{
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attrib[i] = 25 + int(Attribute::GetModifier(profile.attrib[i]) * level);
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skill[i] = int(Skill::GetModifier(profile.skill[i]) * level);
	}
	else
	{
		// set attribute
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attrib[i] = 25 + int(Attribute::GetModifier(profile.attrib[i]) * level);
		// set base skill
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skill[i] = profile.skill[i];
		// apply tag skills
		if(!profile.subprofiles.empty())
		{
			bool single_weapon = false;
			StatProfile::Subprofile& sub = *profile.subprofiles[subprofile.index];
			for(int i = 0; i < StatProfile::Subprofile::MAX_TAGS; ++i)
			{
				if(sub.tag_skills[i] == SkillId::NONE)
					break;
				SkillId sk = subprofile.GetSkill(sub.tag_skills[i]);
				if(Skill::skills[(int)sk].type == SkillType::WEAPON && !single_weapon)
				{
					single_weapon = true;
					skill[(int)SkillId::ONE_HANDED_WEAPON] += Skill::TAG_BONUS;
				}
				skill[(int)sk] += Skill::TAG_BONUS;
			}
		}
		// calculate skill
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skill[i] = int(Skill::GetModifier(skill[i]) * level);
	}
}

//=================================================================================================
void UnitStats::SetForNew(StatProfile& profile)
{
	// only used by player
	int level = subprofile.level;
	assert(!fixed && level >= 1);
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		if(attrib[i] == -2)
			attrib[i] = 25 + int(Attribute::GetModifier(profile.attrib[i]) * level);
	}
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(skill[i] == -2)
			skill[i] = int(Skill::GetModifier(profile.skill[i]) * level);
	}
}

//=================================================================================================
void UnitStats::Write(BitStreamWriter& f) const
{
	f << attrib;
	f << skill;
}

//=================================================================================================
void UnitStats::Read(BitStreamReader& f)
{
	f >> attrib;
	f >> skill;
}
