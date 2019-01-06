// attributes & skill profiles
#include "Pch.h"
#include "GameCore.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
vector<StatProfile*> StatProfile::profiles;

//=================================================================================================
bool StatProfile::operator != (const StatProfile& p) const
{
	bool result = false;
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		if(attrib[i] != p.attrib[i])
		{
			Info("AttributeId %s: %d and %d.", Attribute::attributes[i].id, attrib[i], p.attrib[i]);
			result = true;
		}
	}
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(skill[i] != p.skill[i])
		{
			Info("SkillId %s: %d and %d.", Skill::skills[i].id, skill[i], p.skill[i]);
			result = true;
		}
	}
	return result;
}

//=================================================================================================
void StatProfile::Set(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);
	assert(level == -1 || level >= 1);

	if(level == -1)
	{
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attribs[i] = attrib[i];
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skills[i] = skill[i];
	}
	else
	{
		for(int i = 0; i < (int)AttributeId::MAX; ++i)
			attribs[i] = 25 + int(Attribute::GetModifier(attrib[i]) * level);
		for(int i = 0; i < (int)SkillId::MAX; ++i)
			skills[i] = int(Skill::GetModifier(skill[i]) * level);
	}
}

//=================================================================================================
void StatProfile::SetForNew(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);
	assert(level >= 1);

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		if(attribs[i] == -2)
			attribs[i] = 25 + int(Attribute::GetModifier(attrib[i]) * level);
	}
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(skills[i] == -2)
			skills[i] = int(Skill::GetModifier(skill[i]) * level);
	}
}

//=================================================================================================
StatProfile* StatProfile::TryGet(Cstring id)
{
	for(auto profile : profiles)
	{
		if(profile->id == id)
			return profile;
	}

	return nullptr;
}
