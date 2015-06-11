#include "Pch.h"
#include "Base.h"
#include "Perk.h"

//-----------------------------------------------------------------------------
PerkInfo g_perks[(int)Perk::Max] = {
	PerkInfo(Perk::Weakness, "weakness", PerkInfo::Free | PerkInfo::Flaw | PerkInfo::History),
	PerkInfo(Perk::Strength, "strength", PerkInfo::History),
	PerkInfo(Perk::Skilled, "skilled", PerkInfo::History),
	PerkInfo(Perk::SkillFocus, "skill_focus", PerkInfo::Free | PerkInfo::History),
	PerkInfo(Perk::Talent, "talent", PerkInfo::Multiple | PerkInfo::History),
	PerkInfo(Perk::CraftingTradition, "crafting_tradition", PerkInfo::History | PerkInfo::Validate),
};

//=================================================================================================
PerkInfo* PerkInfo::Find(const string& id)
{
	for(PerkInfo& pi : g_perks)
	{
		if(id == pi.id)
			return &pi;
	}

	return NULL;
}
