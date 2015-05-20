// klasy gracza i npc
#include "Pch.h"
#include "Base.h"
#include "Class.h"

//-----------------------------------------------------------------------------
Class g_classes[] = {
	NULL, "base_warrior", NULL,
	NULL, "base_hunter", NULL,
	NULL, "base_rogue", NULL
};
const uint n_classes = countof(g_classes);

/*
// klasy gracza i npc
#include "Pch.h"
#include "Base.h"
#include "Class.h"

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
void Validateclasses()
{
	for(int i = 0; i<(int)Class::MAX; ++i)
	{
		ClassInfo& ci = g_classes[i];
		if(ci.class_id != (Class)i)
			WARN(Format("class %s: id mismatch.", ci.id));
		if(ci.name.empty())
			WARN(Format("class %s: empty name.", ci.id));
		if(ci.desc.empty())
			WARN(Format("class %s: empty desc.", ci.id));
		if(!ci.icon)
			WARN(Format("class %s: missing icon file '%s'.", ci.id, ci.icon_file));
	}
}
*/
