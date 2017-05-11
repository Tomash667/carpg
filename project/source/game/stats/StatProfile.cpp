// attributes & skill profiles
#include "Pch.h"
#include "Base.h"
#include "StatProfile.h"

//=================================================================================================
bool StatProfile::operator != (const StatProfile& p) const
{
	bool result = false;
	if(fixed != p.fixed)
		result = true;
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		if(attrib[i] != p.attrib[i])
		{
			LOG(Format("Attribute %s: %d and %d.", g_attributes[i].id, attrib[i], p.attrib[i]));
			result = true;
		}
	}
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(skill[i] != p.skill[i])
		{
			LOG(Format("Skill %s: %d and %d.", g_skills[i].id, skill[i], p.skill[i]));
			result = true;
		}
	}
	return result;
}

//=================================================================================================
void StatProfile::Set(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || fixed)
	{
		for(int i = 0; i<(int)Attribute::MAX; ++i)
			attribs[i] = attrib[i];
		for(int i = 0; i<(int)Skill::MAX; ++i)
			skills[i] = max(0, skill[i]);
	}
	else
	{
		int unused;
		float lvl = float(level)/5;
		for(int i = 0; i<(int)Attribute::MAX; ++i)
			attribs[i] = attrib[i] + int(AttributeInfo::GetModifier(attrib[i], unused) * lvl);
		for(int i = 0; i<(int)Skill::MAX; ++i)
		{
			int val = max(0, skill[i]);
			skills[i] = val + int(SkillInfo::GetModifier(val, unused) * lvl);
		}
	}
}

//=================================================================================================
void StatProfile::SetForNew(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || fixed)
	{
		for(int i = 0; i<(int)Attribute::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i];
		}
		for(int i = 0; i<(int)Skill::MAX; ++i)
		{
			if(skills[i] == -1)
				skills[i] = max(0, skill[i]);
		}
	}
	else
	{
		int unused;
		float lvl = float(level)/5;
		for(int i = 0; i<(int)Attribute::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i] + int(AttributeInfo::GetModifier(attrib[i], unused) * lvl);
		}
		for(int i = 0; i<(int)Skill::MAX; ++i)
		{
			if(skills[i] == -1)
			{
				int val = max(0, skill[i]);
				skills[i] = val + int(SkillInfo::GetModifier(val, unused) * lvl);
			}
		}
	}
}
