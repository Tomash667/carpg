#include "Pch.h"
#include "Ability.h"

vector<Ability*> Ability::abilities;
std::map<int, Ability*> Ability::hash_abilities;

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
Ability* Ability::Get(int hash)
{
	auto it = hash_abilities.find(hash);
	if(it != hash_abilities.end())
		return it->second;
	return nullptr;
}
