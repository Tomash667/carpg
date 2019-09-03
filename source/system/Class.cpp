#include "Pch.h"
#include "GameCore.h"
#include "Class.h"
#include "UnitGroup.h"
#include "UnitData.h"

vector<Class*> Class::classes;
static vector<Class*> player_classes;
static UnitGroup* heroes, *evil_heroes, *crazies;

//=================================================================================================
const Class::PotionEntry& Class::GetPotionEntry(int level) const
{
	for(const PotionEntry& pe : potions)
	{
		if(level >= pe.level)
			return pe;
	}
	return potions.back();
}

//=================================================================================================
Class* Class::TryGet(Cstring id)
{
	for(Class* clas : classes)
	{
		if(clas->id == id)
			return clas;
	}
	return nullptr;
}

//=================================================================================================
Class* Class::GetRandomPlayer()
{
	return RandomItem(player_classes);
}

//=================================================================================================
Class* Class::GetRandomHero(bool evil)
{
	return GetRandomHeroData(evil)->clas;
}

//=================================================================================================
UnitData* Class::GetRandomHeroData(bool evil)
{
	return evil ? evil_heroes->GetRandomUnit() : heroes->GetRandomUnit();
}

//=================================================================================================
Class* Class::GetRandomCrazy()
{
	return GetRandomCrazyData()->clas;
}

//=================================================================================================
UnitData* Class::GetRandomCrazyData()
{
	return crazies->GetRandomUnit();
}

//=================================================================================================
int Class::InitLists()
{
	for(Class* clas : classes)
	{
		if(clas->player)
			player_classes.push_back(clas);
	}

	int errors = 0;

	heroes = UnitGroup::Get("heroes");
	errors += VerifyGroup(heroes, false);

	evil_heroes = UnitGroup::Get("evil_heroes");
	errors += VerifyGroup(evil_heroes, false);

	crazies = UnitGroup::Get("crazies");
	errors += VerifyGroup(crazies, true);

	return errors;
}

//=================================================================================================
int Class::VerifyGroup(UnitGroup* group, bool crazy)
{
	if(!group)
		return 0;

	int errors = 0;
	for(UnitGroup::Entry& entry : group->entries)
	{
		if(!entry.ud->clas)
		{
			Error("Group '%s' unit '%s' don't have class.", group->id.c_str(), entry.ud->id.c_str());
			++errors;
		}
		else if(crazy)
		{
			if(entry.ud->clas->crazy != entry.ud)
			{
				Error("Group '%s' unit '%s' crazy mismatch.", group->id.c_str(), entry.ud->id.c_str());
				++errors;
			}
		}
		else
		{
			if(entry.ud->clas->hero != entry.ud)
			{
				Error("Group '%s' unit '%s' hero mismatch.", group->id.c_str(), entry.ud->id.c_str());
				++errors;
			}
		}
	}

	return errors;
}
