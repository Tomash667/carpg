// character class
#include "Pch.h"
#include "Base.h"
#include "Class.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
ClassInfo g_classes[(int)Class::MAX] = {
	ClassInfo(Class::BARBARIAN, "barbarian", "base_warrior", "icon_barbarian.png", false),
	ClassInfo(Class::BARD, "bard", "base_rogue", "icon_bard.png", false),
	ClassInfo(Class::CLERIC, "cleric", "base_warrior", "icon_cleric.png", false),
	ClassInfo(Class::DRUID, "druid", "base_hunter", "icon_druid.png", false),
	ClassInfo(Class::HUNTER, "hunter", "base_hunter", "icon_hunter.png", true),
	ClassInfo(Class::MAGE, "mage", "base_rogue", "icon_mage.png", false),
	ClassInfo(Class::MONK, "monk", "base_rogue", "icon_monk.png", false),
	ClassInfo(Class::PALADIN, "paladin", "base_warrior", "icon_paladin.png", false),
	ClassInfo(Class::ROGUE, "rogue", "base_rogue", "icon_rogue.png", true),
	ClassInfo(Class::WARRIOR, "warrior", "base_warrior", "icon_warrior.png", true)
};

// START EQUIPMENT
//barbarian - axe2, light armor, vodka, healing potions
//bard - weapon for picked skill or short blade

//=================================================================================================
ClassInfo* ClassInfo::Find(const string& id)
{
	for(ClassInfo& c : g_classes)
	{
		if(id == c.id)
			return &c;
	}

	return nullptr;
}

//=================================================================================================
void ClassInfo::Validate(uint& err)
{
	for(int i = 0; i<(int)Class::MAX; ++i)
	{
		ClassInfo& ci = g_classes[i];
		if(ci.class_id != (Class)i)
		{
			++err;
			WARN(Format("Test: Class %s: id mismatch.", ci.id));
		}
		if(ci.name.empty())
		{
			++err;
			WARN(Format("Test: Class %s: empty name.", ci.id));
		}
		if(ci.desc.empty())
		{
			++err;
			WARN(Format("Test: Class %s: empty desc.", ci.id));
		}
		if(ci.about.empty())
		{
			++err;
			WARN(Format("Test: Class %s: empty about.", ci.id));
		}
		if(!ci.icon)
		{
			++err;
			WARN(Format("Test: Class %s: missing icon file '%s'.", ci.id, ci.icon_file));
		}
		if(IsPickable(ci.class_id))
		{
			if(!ci.unit_data_id)
			{
				++err;
				WARN(Format("Test: Class %s: missing unit data.", ci.id));
			}
			else if(!ci.unit_data)
			{
				++err;
				WARN(Format("Test: Class %s: invalid unit data '%s'.", ci.id, ci.unit_data_id));
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
Class ClassInfo::GetRandom()
{
	// get random hero class, ignore new one for now
	//return (Class)(rand2() % (int)Class::MAX);
	switch(rand2()%7)
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
		for(ClassInfo& ci : g_classes)
		{
			if(ci.pickable)
				classes.push_back(ci.class_id);
		}
	}
	return random_item(classes);
}

//=================================================================================================
Class ClassInfo::GetRandomEvil()
{
	return GetRandom();
	/*switch(rand2() % 16)
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
