#include "Pch.h"
#include "Ability.h"

vector<Ability*> Ability::abilities;
std::map<int, Ability*> Ability::hash_abilities;

//=================================================================================================
Ability* Ability::Get(int hash)
{
	auto it = hash_abilities.find(hash);
	if(it != hash_abilities.end())
		return it->second;
	return nullptr;
}
