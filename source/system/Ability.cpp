#include "Pch.h"
#include "Ability.h"

vector<Ability*> Ability::abilities;
std::map<int, Ability*> Ability::hashAbilities;
Ptr<Ability> Ability::bullCharge, Ability::dash, Ability::fireball, Ability::magicBolt, Ability::thunderBolt;

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
	auto it = hashAbilities.find(hash);
	if(it != hashAbilities.end())
		return it->second;
	return nullptr;
}
