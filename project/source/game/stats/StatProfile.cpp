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
