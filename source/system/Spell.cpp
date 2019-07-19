#include "Pch.h"
#include "GameCore.h"
#include "Spell.h"

vector<Spell*> Spell::spells;
vector<pair<string, Spell*>> Spell::aliases;

//=================================================================================================
Spell* Spell::TryGet(Cstring id)
{
	for(Spell* s : spells)
	{
		if(s->id == id)
			return s;
	}

	for(auto& alias : aliases)
	{
		if(alias.first == id)
			return alias.second;
	}

	return nullptr;
}
