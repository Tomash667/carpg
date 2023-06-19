#include "Pch.h"
#include "Ability.h"

vector<Ability*> Ability::abilities;
std::map<uint, Ability*> Ability::hashAbilities;

//=================================================================================================
SkillId Ability::GetSkill() const
{
	if(type == RangedAttack)
		return SkillId::BOW;
	else if(IsSet(flags, Mage))
		return SkillId::MYSTIC_MAGIC;
	else
		return SkillId::NONE;
}

//=================================================================================================
Ability* Ability::Get(uint hash)
{
	auto it = hashAbilities.find(hash);
	if(it != hashAbilities.end())
		return it->second;
	return nullptr;
}
