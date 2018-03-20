#include "Pch.h"
#include "GameCore.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Language.h"
#include "PlayerController.h"
#include "CreatedCharacter.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
PerkInfo PerkInfo::perks[(int)Perk::Max] = {
	PerkInfo(Perk::Weakness, "weakness", PerkInfo::Free | PerkInfo::Flaw | PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Strength, "strength", PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Skilled, "skilled", PerkInfo::History),
	PerkInfo(Perk::SkillFocus, "skill_focus", PerkInfo::Free | PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Talent, "talent", PerkInfo::Multiple | PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::AlchemistApprentice, "alchemist", PerkInfo::History),
	PerkInfo(Perk::Wealthy, "wealthy", PerkInfo::History),
	PerkInfo(Perk::VeryWealthy, "very_wealthy", PerkInfo::History | PerkInfo::Check, Perk::Wealthy),
	PerkInfo(Perk::FamilyHeirloom, "heirloom", PerkInfo::History),
	PerkInfo(Perk::Leader, "leader", PerkInfo::History),
};

//-----------------------------------------------------------------------------
cstring TakenPerk::txIncreasedAttrib, TakenPerk::txIncreasedSkill, TakenPerk::txDecreasedAttrib, TakenPerk::txDecreasedSkill, TakenPerk::txDecreasedSkills;

//=================================================================================================
PerkInfo* PerkInfo::Find(const string& id)
{
	for(PerkInfo& pi : PerkInfo::perks)
	{
		if(id == pi.id)
			return &pi;
	}

	return nullptr;
}

//=================================================================================================
void PerkInfo::Validate(uint& err)
{
	int index = 0;
	for(PerkInfo& pi : PerkInfo::perks)
	{
		if(pi.perk_id != (Perk)index)
		{
			Warn("Test: Perk %s: id mismatch.", pi.id);
			++err;
		}
		if(pi.name.empty())
		{
			Warn("Test: Perk %s: empty name.", pi.id);
			++err;
		}
		if(pi.desc.empty())
		{
			Warn("Test: Perk %s: empty desc.", pi.id);
			++err;
		}
		++index;
	}
}

//=================================================================================================
void TakenPerk::LoadText()
{
	txIncreasedAttrib = Str("increasedAttrib");
	txIncreasedSkill = Str("increasedSkill");
	txDecreasedAttrib = Str("decreasedAttrib");
	txDecreasedSkill = Str("decreasedSkill");
	txDecreasedSkills = Str("decreasedSkills");
}

//=================================================================================================
void TakenPerk::GetDesc(string& s) const
{
	switch(perk)
	{
	case Perk::Skilled:
	case Perk::AlchemistApprentice:
	case Perk::Wealthy:
	case Perk::VeryWealthy:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		s.clear();
		break;
	case Perk::Weakness:
		s = Format("%s: %s", txDecreasedAttrib, Attribute::attributes[value].name.c_str());
		break;
	case Perk::Strength:
		s = Format("%s: %s", txIncreasedAttrib, Attribute::attributes[value].name.c_str());
		break;
	case Perk::SkillFocus:
		{
			int skill_p = (value & 0xFF),
				skill_m1 = ((value & 0xFF00) >> 8),
				skill_m2 = ((value & 0xFF0000) >> 16);
			s = Format("%s: %s\n%s: %s, %s", txIncreasedSkill, Skill::skills[skill_p].name.c_str(), txDecreasedSkills,
				Skill::skills[skill_m1].name.c_str(), Skill::skills[skill_m2].name.c_str());
		}
		break;
	case Perk::Talent:
		s = Format("%s: %s", txIncreasedSkill, Skill::skills[value].name.c_str());
		break;
	default:
		assert(0);
		s.clear();
		break;
	}
}

//=================================================================================================
int TakenPerk::Apply(CreatedCharacter& cc, bool validate) const
{
	PerkInfo& info = PerkInfo::perks[(int)perk];

	switch(perk)
	{
	case Perk::Strength:
		if(validate)
		{
			if(value < 0 || value >= (int)AttributeId::MAX)
			{
				Error("Perk 'strength', invalid attribute %d.", value);
				return 2;
			}
			if(cc.a[value].mod)
			{
				Error("Perk 'strength', attribute %d is already modified.", value);
				return 3;
			}
		}
		cc.a[value].Mod(5, true);
		break;
	case Perk::Weakness:
		if(validate)
		{
			if(value < 0 || value >= (int)AttributeId::MAX)
			{
				Error("Perk 'weakness', invalid attribute %d.", value);
				return 2;
			}
			if(cc.a[value].mod)
			{
				Error("Perk 'weakness', attribute %d is already modified.", value);
				return 3;
			}
		}
		cc.a[value].Mod(-5, true);
		break;
	case Perk::Skilled:
		cc.update_skills = true;
		cc.sp += 2;
		cc.sp_max += 2;
		break;
	case Perk::SkillFocus:
		{
			int v[3];
			Split3(value, v[0], v[1], v[2]);
			if(validate)
			{
				for(int i = 0; i < 3; ++i)
				{
					if(v[i] < 0 || v[i] >= (int)SkillId::MAX)
					{
						Error("Perk 'skill_focus', invalid skill %d (%d).", v[i], i);
						return 2;
					}
					if(cc.a[v[i]].mod)
					{
						Error("Perk 'skill_focus', skill %d is already modified (%d).", v[i], i);
						return 3;
					}
				}
			}
			cc.to_update.push_back((SkillId)v[0]);
			cc.to_update.push_back((SkillId)v[1]);
			cc.to_update.push_back((SkillId)v[2]);
			cc.s[v[0]].Mod(10, true);
			cc.s[v[1]].Mod(-5, true);
			cc.s[v[2]].Mod(-5, true);
		}
		break;
	case Perk::Talent:
		if(validate)
		{
			if(value < 0 || value >= (int)SkillId::MAX)
			{
				Error("Perk 'talent', invalid skill %d.", value);
				return 2;
			}
			if(cc.s[value].mod)
			{
				Error("Perk 'talent', skill %d is already modified.", value);
				return 3;
			}
		}
		cc.s[value].Mod(5, true);
		cc.to_update.push_back((SkillId)value);
		break;
	case Perk::VeryWealthy:
		{
			bool found = false;
			for(uint i = 0; i < cc.taken_perks.size(); ++i)
			{
				if(cc.taken_perks[i].perk == Perk::Wealthy)
				{
					found = true;
					cc.taken_perks.erase(cc.taken_perks.begin() + i);
					break;
				}
			}
			if(!found)
				--cc.perks;
		}
		break;
	case Perk::Wealthy:
	case Perk::AlchemistApprentice:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		break;
	default:
		assert(0);
		break;
	}

	if(!IS_SET(info.flags, PerkInfo::Free))
		--cc.perks;
	if(IS_SET(info.flags, PerkInfo::Flaw))
	{
		++cc.perks;
		++cc.perks_max;
	}

	return 0;
}

//=================================================================================================
void TakenPerk::Apply(PlayerController& pc) const
{
	switch(perk)
	{
	case Perk::Strength:
		pc.base_stats.attrib[value] += 5;
		break;
	case Perk::Weakness:
		pc.base_stats.attrib[value] -= 5;
		break;
	case Perk::Skilled:
		// nothing to do here, only affects character creation
		break;
	case Perk::SkillFocus:
		{
			int plus, minus, minus2;
			Split3(value, plus, minus, minus2);
			pc.base_stats.skill[plus] += 10;
			pc.base_stats.skill[minus] -= 5;
			pc.base_stats.skill[minus2] -= 5;
		}
		break;
	case Perk::Talent:
		pc.base_stats.skill[value] += 5;
		break;
	case Perk::AlchemistApprentice:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		// effects in CreatedCharacter::Apply
		break;
	case Perk::Wealthy:
		pc.unit->gold += 500;
		break;
	case Perk::VeryWealthy:
		pc.unit->gold += 2000;
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void TakenPerk::Remove(CreatedCharacter& cc, int index) const
{
	PerkInfo& info = PerkInfo::perks[(int)perk];
	bool add = false;

	switch(perk)
	{
	case Perk::Strength:
		cc.a[value].Mod(-5, false);
		break;
	case Perk::Weakness:
		cc.a[value].Mod(5, false);
		break;
	case Perk::Skilled:
		cc.update_skills = true;
		cc.sp -= 2;
		cc.sp_max -= 2;
		break;
	case Perk::SkillFocus:
		{
			int plus, minus, minus2;
			Split3(value, plus, minus, minus2);
			cc.to_update.push_back((SkillId)plus);
			cc.to_update.push_back((SkillId)minus);
			cc.to_update.push_back((SkillId)minus2);
			cc.s[plus].Mod(-10, false);
			cc.s[minus].Mod(5, false);
			cc.s[minus2].Mod(5, false);
		}
		break;
	case Perk::Talent:
		cc.s[value].Mod(-5, false);
		cc.to_update.push_back((SkillId)value);
		break;
	case Perk::AlchemistApprentice:
	case Perk::Wealthy:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		break;
	case Perk::VeryWealthy:
		add = true;
		break;
	default:
		assert(0);
		break;
	}

	if(!IS_SET(info.flags, PerkInfo::Free))
		++cc.perks;
	if(IS_SET(info.flags, PerkInfo::Flaw))
	{
		--cc.perks;
		--cc.perks_max;
	}

	cc.taken_perks.erase(cc.taken_perks.begin() + index);

	if(add)
		cc.taken_perks.push_back(TakenPerk(Perk::Wealthy));
}

//=================================================================================================
cstring TakenPerk::FormatName()
{
	PerkInfo& p = PerkInfo::perks[(int)perk];

	switch(perk)
	{
	case Perk::Weakness:
	case Perk::Strength:
		return Format("%s (%s)", p.name.c_str(), Attribute::attributes[value].name.c_str());
	case Perk::SkillFocus:
		{
			int skill_p = (value & 0xFF);
			return Format("%s (%s)", p.name.c_str(), Skill::skills[skill_p].name.c_str());
		}
	case Perk::Talent:
		return Format("%s (%s)", p.name.c_str(), Skill::skills[value].name.c_str());
	default:
		assert(0);
		return p.name.c_str();
	}
}
