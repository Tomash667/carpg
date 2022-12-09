// attributes & skill profiles
#include "Pch.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
vector<StatProfile*> StatProfile::profiles;
const float StatProfile::Subprofile::defaultPriorities[IT_MAX_WEARABLE] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };
const float StatProfile::Subprofile::defaultTagPriorities[TAG_MAX] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };

//=================================================================================================
StatProfile::Subprofile::Subprofile() : weaponChance(), weaponTotal(0), armorChance(), armorTotal(0), itemScript(nullptr)
{
	for(int i = 0; i < IT_MAX_WEARABLE; ++i)
		priorities[i] = defaultPriorities[i];
	for(int i = 0; i < TAG_MAX; ++i)
		tagPriorities[i] = defaultTagPriorities[i];
	for(int i = 0; i < StatProfile::MAX_TAGS; ++i)
		tagSkills[i] = SkillId::NONE;
	for(int i = 0; i < StatProfile::MAX_PERKS; ++i)
		perks[i].perk = nullptr;
}

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
SubprofileInfo StatProfile::GetRandomSubprofile(SubprofileInfo* prev)
{
	SubprofileInfo s;
	if(subprofiles.empty())
	{
		s.value = 0;
		return s;
	}
	s.index = byte(Rand() % subprofiles.size());
	s.level = 0;
	Subprofile& sub = *subprofiles[s.index];
	uint j = 0, k = Rand() % sub.weaponTotal;
	for(int i = 0; i < WT_MAX; ++i)
	{
		j += sub.weaponChance[i];
		if(k < j)
		{
			s.weapon = i;
			break;
		}
	}
	j = 0;
	k = Rand() % sub.armorTotal;
	for(int i = 0; i < AT_MAX; ++i)
	{
		j += sub.armorChance[i];
		if(k < j)
		{
			s.armor = i;
			break;
		}
	}
	if(prev && s.value == prev->value)
	{
		s.index = (s.index + 1) % subprofiles.size();
		Subprofile& sub = *subprofiles[s.index];
		byte start = s.weapon, i = s.weapon;
		while(true)
		{
			i = (i + 1) % WT_MAX;
			if(i == start)
				break;
			if(sub.weaponChance[i] > 0)
			{
				s.weapon = i;
				break;
			}
		}
		start = s.armor;
		i = s.armor;
		while(true)
		{
			i = (i + 1) % AT_MAX;
			if(i == start)
				break;
			if(sub.armorChance[i] > 0)
			{
				s.armor = i;
				break;
			}
		}
	}
	return s;
}

//=================================================================================================
SkillId SubprofileInfo::GetSkill(SkillId skill) const
{
	if(skill == SkillId::SPECIAL_WEAPON)
		return WeaponTypeInfo::info[weapon].skill;
	else if(skill == SkillId::SPECIAL_ARMOR)
		return GetArmorTypeSkill((ARMOR_TYPE)armor);
	else
		return skill;
}
