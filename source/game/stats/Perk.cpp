#include "Pch.h"
#include "Base.h"
#include "Perk.h"

PerkInfo g_perks[(int)Perk::Max] = {
	Perk::Weakness, "S³aboœæ", "Wada - Zmniejsza wybrany atrybut o 5 punktów. Pozwala wybraæ dwa dodatkowe atuty.", PerkInfo::Flaw | PerkInfo::History,
	Perk::Strength, "Si³a", "Zwiêksza wybrany atrybut o 5 punktów.", PerkInfo::History,
	Perk::Skilled, "Fachowiec", "Daje dwa dodatkowe punkty umiejêtnoœci.", PerkInfo::Multiple | PerkInfo::History,
	Perk::SkillFocus, "Skupienie na umiejêtnoœci", "Darmowy - Zmniejsza dwie wybrane umiejêtnoœci o 5, zwiêksza jedn¹ o 10.", PerkInfo::Free | PerkInfo::History,
	Perk::Talent, "Talent", "Zwiêksza wybran¹ umiejêtnoœæ o 5 punktów.", PerkInfo::History,
	Perk::CraftingTradition, "Tradycja rzemieœlnicza", "Zwiêksza umiejêtnoœæ Rzemios³o o 10 punktów.", PerkInfo::History,
};
