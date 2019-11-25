#include "Pch.h"
#include "GameCore.h"
#include "Ability.h"

vector<Ability*> Ability::abilities;
vector<pair<string, Ability*>> Ability::aliases;

//=================================================================================================
Ability* Ability::TryGet(Cstring id)
{
	for(Ability* a : abilities)
	{
		if(a->id == id)
			return a;
	}

	for(auto& alias : aliases)
	{
		if(alias.first == id)
			return alias.second;
	}

	return nullptr;
}
