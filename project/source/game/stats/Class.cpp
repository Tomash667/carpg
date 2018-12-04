// character class
#include "Pch.h"
#include "GameCore.h"
#include "Class.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
ClassInfo ClassInfo::classes[(int)Class::MAX] = {
	ClassInfo(Class::BARBARIAN, "barbarian", "base_warrior", "icon_barbarian.png", false, nullptr),
	ClassInfo(Class::BARD, "bard", "base_rogue", "icon_bard.png", false, nullptr),
	ClassInfo(Class::CLERIC, "cleric", "base_warrior", "icon_cleric.png", false, nullptr),
	ClassInfo(Class::DRUID, "druid", "base_hunter", "icon_druid.png", false, nullptr),
	ClassInfo(Class::HUNTER, "hunter", "base_hunter", "icon_hunter.png", true, "summon_wolf"),
	ClassInfo(Class::MAGE, "mage", "base_rogue", "icon_mage.png", false, nullptr),
	ClassInfo(Class::MONK, "monk", "base_rogue", "icon_monk.png", false, nullptr),
	ClassInfo(Class::PALADIN, "paladin", "base_warrior", "icon_paladin.png", false, nullptr),
	ClassInfo(Class::ROGUE, "rogue", "base_rogue", "icon_rogue.png", true, "dash"),
	ClassInfo(Class::WARRIOR, "warrior", "base_warrior", "icon_warrior.png", true, "bull_charge")
};

// START EQUIPMENT
//barbarian - axe2, light armor, vodka, healing potions
//bard - weapon for picked skill or short blade

//=================================================================================================
ClassInfo* ClassInfo::Find(const string& id)
{
	for(ClassInfo& c : ClassInfo::classes)
	{
		if(id == c.id)
			return &c;
	}

	return nullptr;
}

//=================================================================================================
void ClassInfo::Validate(uint& err)
{
	for(int i = 0; i < (int)Class::MAX; ++i)
	{
		ClassInfo& ci = ClassInfo::classes[i];
		if(ci.class_id != (Class)i)
		{
			++err;
			Warn("Test: Class %s: id mismatch.", ci.id);
		}
		if(ci.name.empty())
		{
			++err;
			Warn("Test: Class %s: empty name.", ci.id);
		}
		if(ci.desc.empty())
		{
			++err;
			Warn("Test: Class %s: empty desc.", ci.id);
		}
		if(ci.about.empty())
		{
			++err;
			Warn("Test: Class %s: empty about.", ci.id);
		}
		if(!ci.icon)
		{
			++err;
			Warn("Test: Class %s: missing icon file '%s'.", ci.id, ci.icon_file);
		}
		if(IsPickable(ci.class_id))
		{
			if(!ci.unit_data_id)
			{
				++err;
				Warn("Test: Class %s: missing unit data.", ci.id);
			}
			else if(!ci.unit_data)
			{
				++err;
				Warn("Test: Class %s: invalid unit data '%s'.", ci.id, ci.unit_data_id);
			}
		}
	}
}

//=================================================================================================
Class ClassInfo::OldToNew(Class c)
{
	switch((int)c)
	{
	case 0:
	default:
		return Class::WARRIOR;
	case 1:
		return Class::HUNTER;
	case 2:
		return Class::ROGUE;
	case 3:
		return Class::MAGE;
	case 4:
		return Class::CLERIC;
	}
}

//=================================================================================================
UnitData& ClassInfo::GetUnitData(Class clas, bool crazy)
{
	cstring id = nullptr;

	switch(clas)
	{
	default:
		assert(0);
	case Class::WARRIOR:
		id = (crazy ? "crazy_warrior" : "hero_warrior");
		break;
	case Class::HUNTER:
		id = (crazy ? "crazy_hunter" : "hero_hunter");
		break;
	case Class::ROGUE:
		id = (crazy ? "crazy_rogue" : "hero_rogue");
		break;
	case Class::MAGE:
		id = (crazy ? "crazy_mage" : "hero_mage");
		break;
	}

	return *UnitData::Get(id);
}

//=================================================================================================
Class ClassInfo::GetRandom()
{
	// get Random hero class, ignore new one for now
	//return (Class)(Rand() % (int)Class::MAX);
	switch(Rand() % 7)
	{
	default:
	case 0:
	case 1:
		return Class::WARRIOR;
	case 2:
	case 3:
		return Class::HUNTER;
	case 4:
	case 5:
		return Class::ROGUE;
	case 6:
		return Class::MAGE;
	}
}

//=================================================================================================
Class ClassInfo::GetRandomPlayer()
{
	static vector<Class> classes;
	if(classes.empty())
	{
		for(ClassInfo& ci : ClassInfo::classes)
		{
			if(ci.pickable)
				classes.push_back(ci.class_id);
		}
	}
	return RandomItem(classes);
}

//=================================================================================================
Class ClassInfo::GetRandomEvil()
{
	return GetRandom();
	/*switch(Rand() % 16)
	{
	case 0:
	case 1:
	case 2:
		return Class::BARBARIAN;
	case 3:
		return Class::BARD;
	case 4:
		return Class::CLERIC;
	case 5:
		return Class::DRUID;
	case 6:
	case 7:
		return Class::HUNTER;
	case 8:
		return Class::MAGE;
	case 9:
		return Class::MONK;
	case 10:
	case 11:
	case 12:
		return Class::ROGUE;
	case 13:
	case 14:
	case 15:
	default:
		return Class::WARRIOR;
	}*/
}
