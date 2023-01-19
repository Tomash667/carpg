#include "Pch.h"
#include "Class.h"

#include "UnitData.h"
#include "UnitGroup.h"

vector<Class*> Class::classes;
static vector<Class*> playerClasses;
static UnitGroup* heroes, *evilHeroes, *crazies;

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
	return RandomItem(playerClasses);
}

//=================================================================================================
Class* Class::GetRandomHero(bool evil)
{
	return GetRandomHeroData(evil)->clas;
}

//=================================================================================================
UnitData* Class::GetRandomHeroData(bool evil)
{
	return evil ? evilHeroes->GetRandomUnit() : heroes->GetRandomUnit();
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
			playerClasses.push_back(clas);
	}

	int errors = 0;

	heroes = UnitGroup::Get("heroes");
	errors += VerifyGroup(heroes, false);

	evilHeroes = UnitGroup::Get("evilHeroes");
	errors += VerifyGroup(evilHeroes, false);

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

//=================================================================================================
::Class* old::ConvertOldClass(int clas)
{
	cstring id;
	switch((old::Class)clas)
	{
	case old::Class::BARBARIAN:
		id = "barbarian";
		break;
	case old::Class::BARD:
		id = "bard";
		break;
	case old::Class::CLERIC:
		id = "cleric";
		break;
	case old::Class::DRUID:
		id = "druid";
		break;
	case old::Class::HUNTER:
		id = "hunter";
		break;
	case old::Class::MAGE:
		id = "mage";
		break;
	case old::Class::MONK:
		id = "monk";
		break;
	case old::Class::PALADIN:
		id = "paladin";
		break;
	case old::Class::ROGUE:
		id = "rogue";
		break;
	default:
		assert(0);
	case old::Class::WARRIOR:
		id = "warrior";
		break;
	}

	return ::Class::TryGet(id);
}
