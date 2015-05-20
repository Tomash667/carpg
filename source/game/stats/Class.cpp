// klasy gracza i npc
#include "Pch.h"
#include "Base.h"
#include "Class.h"

//-----------------------------------------------------------------------------
ClassInfo g_classes[(int)Class::MAX] = {
	ClassInfo(Class::WARRIOR, "warrior", "base_warrior", "icon_warrior.png"),
	ClassInfo(Class::HUNTER, "hunter", "base_hunter", "icon_hunter.png"),
	ClassInfo(Class::ROGUE, "rogue", "base_rogue", "icon_rogue.png"),
	ClassInfo(Class::MAGE, "mage", NULL, "icon_mage.png"),
	ClassInfo(Class::CLERIC, "cleric", NULL, "icon_cleric.png")
};


/*

// START EQUIPMENT
//barbarian - axe2, light armor, vodka, healing potions
//bard - weapon for picked skill or short blade

//-----------------------------------------------------------------------------
ClassInfo g_classes[(int)Class::MAX] = {
	ClassInfo(Class::BARBARIAN, "barbarian", "icon_barbarian.png"),
	ClassInfo(Class::BARD, "bard", "icon_bard.png"),
	ClassInfo(Class::CLERIC, "cleric", "icon_cleric.png"),
	ClassInfo(Class::DRUID, "druid", "icon_druid.png"),
	ClassInfo(Class::HUNTER, "hunter", "icon_hunter.png"),
	ClassInfo(Class::MAGE, "mage", "icon_mage.png"),
	ClassInfo(Class::MONK, "monk", "icon_monk.png"),
	ClassInfo(Class::PALADIN, "paladin", "icon_paladin.png"),
	ClassInfo(Class::ROGUE, "rogue", "icon_rogue.png"),
	ClassInfo(Class::WARRIOR, "warrior", "icon_warrior.png")
};
*/

//=================================================================================================
ClassInfo* ClassInfo::Find(const string& id)
{
	for(ClassInfo& c : g_classes)
	{
		if(id == c.id)
			return &c;
	}

	return NULL;
}

//=================================================================================================
void ClassInfo::Validate(int& err)
{
	for(int i = 0; i<(int)Class::MAX; ++i)
	{
		ClassInfo& ci = g_classes[i];
		if(ci.class_id != (Class)i)
		{
			++err;
			WARN(Format("class %s: id mismatch.", ci.id));
		}
		if(ci.name.empty())
		{
			++err;
			WARN(Format("class %s: empty name.", ci.id));
		}
		if(ci.desc.empty())
		{
			++err;
			WARN(Format("class %s: empty desc.", ci.id));
		}
		if(!ci.icon)
		{
			++err;
			WARN(Format("class %s: missing icon file '%s'.", ci.id, ci.icon_file));
		}
	}
}
