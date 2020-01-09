#include "Pch.h"
#include "GameCore.h"
#include "Ability.h"

vector<Ability*> Ability::abilities;
std::map<uint, Ability*> Ability::hash_abilities;

//=================================================================================================
Ability* Ability::Get(uint hash)
{
	auto it = hash_abilities.find(hash);
	if(it != hash_abilities.end())
		return it->second;
	return nullptr;
}
