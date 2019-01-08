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
StatProfile::Subprofile* StatProfile::GetSubprofile(const string& id)
{
	for(Subprofile* subprofile : subprofiles)
	{
		if(subprofile->id == id)
			return subprofile;
	}
	return nullptr;
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

//=================================================================================================
SubprofileInfo StatProfile::GetRandomSubprofile()
{
	SubprofileInfo s;
	if(subprofiles.empty())
	{
		s.value = 0;
		return s;
	}
	s.index = byte(Rand() % subprofiles.size());
	Subprofile& sub = *subprofiles[s.index];
	uint j = 0, k = Rand() % sub.weapon_total;
	for(int i = 0; i < WT_MAX; ++i)
	{
		if(k < j)
		{
			s.weapon = i;
			break;
		}
		j += sub.weapon_chance[i];
	}
	j = 0;
	k = Rand() % sub.armor_total;
	for(int i = 0; i < AT_MAX; ++i)
	{
		if(k < j)
		{
			s.armor = i;
			break;
		}
		j += sub.armor_chance[i];
	}
	return s;
}
