// attributes & skill profiles
#include "Pch.h"
#include "Core.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
vector<StatProfile*> StatProfile::profiles;

//=================================================================================================
bool StatProfile::operator != (const StatProfile& p) const
{
	bool result = false;
	if(flags != p.flags)
		result = true;
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		if(attrib[i] != p.attrib[i])
		{
			Info("Attribute %s: %d and %d.", AttributeInfo::attributes[i].id, attrib[i], p.attrib[i]);
			result = true;
		}
	}
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		if(skill[i] != p.skill[i])
		{
			Info("Skill %s: %d and %d.", SkillInfo::skills[i].id, skill[i], p.skill[i]);
			result = true;
		}
	}
	return result;
}

//=================================================================================================
void StatProfile::Set(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || IS_SET(flags, F_FIXED))
	{
		for(int i = 0; i < (int)Attribute::MAX; ++i)
			attribs[i] = attrib[i];
		for(int i = 0; i < (int)Skill::MAX; ++i)
			skills[i] = max(0, skill[i]);
	}
	else
	{
		int unused;
		float lvl = float(level) / 5;
		for(int i = 0; i < (int)Attribute::MAX; ++i)
			attribs[i] = attrib[i] + int(AttributeInfo::GetModifier(attrib[i], unused) * lvl);
		for(int i = 0; i < (int)Skill::MAX; ++i)
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

	if(level == 0 || IS_SET(flags, F_FIXED))
	{
		for(int i = 0; i < (int)Attribute::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i];
		}
		for(int i = 0; i < (int)Skill::MAX; ++i)
		{
			if(skills[i] == -1)
				skills[i] = max(0, skill[i]);
		}
	}
	else
	{
		int unused;
		float lvl = float(level) / 5;
		for(int i = 0; i < (int)Attribute::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i] + int(AttributeInfo::GetModifier(attrib[i], unused) * lvl);
		}
		for(int i = 0; i < (int)Skill::MAX; ++i)
		{
			if(skills[i] == -1)
			{
				int val = max(0, skill[i]);
				skills[i] = val + int(SkillInfo::GetModifier(val, unused) * lvl);
			}
		}
	}
}

//=================================================================================================
SubProfile* StatProfile::TryGetSubprofile(const AnyString& id)
{
	for(auto sub : subprofiles)
	{
		if(sub->id == id)
			return sub;
	}

	return nullptr;
}

//=================================================================================================
StatProfile* StatProfile::TryGet(const AnyString& id)
{
	for(auto profile : profiles)
	{
		if(profile->id == id)
			return profile;
	}

	return nullptr;
}
