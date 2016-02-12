// attributes & skill profiles
#include "Pch.h"
#include "Base.h"
#include "StatProfile.h"

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
