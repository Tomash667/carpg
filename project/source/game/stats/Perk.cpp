#include "Pch.h"
#include "Core.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Language.h"
#include "PlayerController.h"
#include "CreatedCharacter.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
PerkInfo g_perks[(int)Perk::Max] = {
	PerkInfo(Perk::Weakness, "weakness", PerkInfo::Flaw | PerkInfo::History | PerkInfo::RequireFormat, PerkInfo::Attribute),
	PerkInfo(Perk::Strength, "strength", PerkInfo::History | PerkInfo::RequireFormat, PerkInfo::Attribute),
	PerkInfo(Perk::Skilled, "skilled", PerkInfo::History),
	PerkInfo(Perk::SkillFocus, "skill_focus", PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Talent, "talent", PerkInfo::Multiple | PerkInfo::History | PerkInfo::RequireFormat, PerkInfo::Skill),
	PerkInfo(Perk::AlchemistApprentice, "alchemist", PerkInfo::History),
	PerkInfo(Perk::Wealthy, "wealthy", PerkInfo::History),
	PerkInfo(Perk::VeryWealthy, "very_wealthy", PerkInfo::History),
	PerkInfo(Perk::FilthyRich, "filthy_rich", PerkInfo::History),
	PerkInfo(Perk::FamilyHeirloom, "heirloom", PerkInfo::History),
	PerkInfo(Perk::Leader, "leader", PerkInfo::History)
};

//-----------------------------------------------------------------------------
cstring TakenPerk::txIncreasedAttrib, TakenPerk::txIncreasedSkill, TakenPerk::txDecreasedAttrib, TakenPerk::txDecreasedSkill, TakenPerk::txDecreasedSkills;

//=================================================================================================
PerkInfo* PerkInfo::Find(const string& id)
{
	for(PerkInfo& pi : g_perks)
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
	for(PerkInfo& pi : g_perks)
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
	case Perk::FilthyRich:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		s.clear();
		break;
	case Perk::Weakness:
		s = Format("%s: %s", txDecreasedAttrib, AttributeInfo::attributes[value].name.c_str());
		break;
	case Perk::Strength:
		s = Format("%s: %s", txIncreasedAttrib, AttributeInfo::attributes[value].name.c_str());
		break;
	case Perk::SkillFocus:
		{
			int skill_p = (value & 0xFF),
				skill_m1 = ((value & 0xFF00) >> 8),
				skill_m2 = ((value & 0xFF0000) >> 16);
			s = Format("%s: %s\n%s: %s, %s", txIncreasedSkill, SkillInfo::skills[skill_p].name.c_str(), txDecreasedSkills,
				SkillInfo::skills[skill_m1].name.c_str(), SkillInfo::skills[skill_m2].name.c_str());
		}
		break;
	case Perk::Talent:
		s = Format("%s: %s", txIncreasedSkill, SkillInfo::skills[value].name.c_str());
		break;
	default:
		assert(0);
		s.clear();
		break;
	}
}

//=================================================================================================
cstring TakenPerk::FormatName()
{
	PerkInfo& p = g_perks[(int)perk];

	switch(perk)
	{
	case Perk::Weakness:
	case Perk::Strength:
		return Format("%s (%s)", p.name.c_str(), AttributeInfo::attributes[value].name.c_str());
	case Perk::SkillFocus:
		{
			int skill_p = (value & 0xFF);
			return Format("%s (%s)", p.name.c_str(), SkillInfo::skills[skill_p].name.c_str());
		}
	case Perk::Talent:
		return Format("%s (%s)", p.name.c_str(), SkillInfo::skills[value].name.c_str());
	default:
		assert(0);
		return p.name.c_str();
	}
}

//=================================================================================================
// Check value of perk sent in mp
bool TakenPerk::Validate()
{
	switch(perk)
	{
	case Perk::Strength:
	case Perk::Weakness:
		if(value < 0 || value >= (int)Attribute::MAX)
		{
			Error("Perk 'strength', invalid attribute %d.", value);
			return false;
		}
		break;
	case Perk::SkillFocus:
		{
			int v[3];
			Split3(value, v[0], v[1], v[2]);
			for(int i = 0; i < 3; ++i)
			{
				if(v[i] < 0 || v[i] >= (int)Skill::MAX)
				{
					Error("Perk 'skill_focus', invalid skill %d (%d).", v[i], i);
					return false;
				}
			}
		}
		break;
	case Perk::Talent:
		if(value < 0 || value >= (int)Skill::MAX)
		{
			Error("Perk 'talent', invalid skill %d.", value);
			return false;
		}
		break;
	}

	return true;
}

//=================================================================================================
// Check if unit can take perk
bool TakenPerk::CanTake(PerkContext& ctx)
{
	switch(perk)
	{
	case Perk::Weakness:
	case Perk::Strength:
	case Perk::Skilled:
	case Perk::SkillFocus:
	case Perk::Talent:
	case Perk::AlchemistApprentice:
	case Perk::Wealthy:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		return true;
	case Perk::VeryWealthy:
		return ctx.HavePerk(Perk::Wealthy);
	case Perk::FilthyRich:
		return ctx.HavePerk(Perk::VeryWealthy);
	default:
		assert(0);
		return false;
	}
}

//=================================================================================================
// Apply perk to unit
bool TakenPerk::Apply(PerkContext& ctx)
{
	switch(perk)
	{
	case Perk::Strength:
		if(ctx.validate && ctx.cc && ctx.cc->a[value].mod)
		{
			Error("Perk 'strength', attribute %d is already modified.", value);
			return false;
		}
		if(ctx.cc)
			ctx.cc->a[value].Mod(5, true);
		else
			ctx.pc->unit->SetBase((Attribute)value, 5, ctx.startup, true);
		break;
	case Perk::Weakness:
		if(ctx.validate && ctx.cc && ctx.cc->a[value].mod)
		{
			Error("Perk 'weakness', attribute %d is already modified.", value);
			return false;
		}
		if(ctx.cc)
			ctx.cc->a[value].Mod(-5, true);
		else
			ctx.pc->unit->SetBase((Attribute)value, -5, ctx.startup, true);
		break;
	case Perk::Skilled:
		if(ctx.cc)
		{
			ctx.cc->update_skills = true;
			ctx.cc->sp += 3;
			ctx.cc->sp_max += 3;
		}
		break;
	case Perk::SkillFocus:
		{
			int v[3];
			Split3(value, v[0], v[1], v[2]);
			if(ctx.validate && ctx.cc)
			{
				for(int i = 0; i < 3; ++i)
				{
					if(ctx.cc->a[v[i]].mod)
					{
						Error("Perk 'skill_focus', skill %d is already modified (%d).", v[i], i);
						return false;
					}
				}
			}
			if(ctx.cc)
			{
				ctx.cc->to_update.push_back((Skill)v[0]);
				ctx.cc->to_update.push_back((Skill)v[1]);
				ctx.cc->to_update.push_back((Skill)v[2]);
				ctx.cc->s[v[0]].Mod(10, true);
				ctx.cc->s[v[1]].Mod(-5, true);
				ctx.cc->s[v[2]].Mod(-5, true);
			}
			else
			{
				ctx.pc->unit->SetBase((Skill)v[0], 10, ctx.startup, true);
				ctx.pc->unit->SetBase((Skill)v[1], -5, ctx.startup, true);
				ctx.pc->unit->SetBase((Skill)v[2], -5, ctx.startup, true);
			}
		}
		break;
	case Perk::Talent:
		if(ctx.validate && ctx.cc && ctx.cc->s[value].mod)
		{
			Error("Perk 'talent', skill %d is already modified.", value);
			return false;
		}
		if(ctx.cc)
		{
			ctx.cc->s[value].Mod(5, true);
			ctx.cc->to_update.push_back((Skill)value);
		}
		else
			ctx.pc->unit->SetBase((Skill)value, 5, ctx.startup, true);
		break;
	case Perk::Wealthy:
		if(ctx.pc && !hidden && ctx.startup)
			ctx.pc->unit->gold += 1000;
		break;
	case Perk::VeryWealthy:
		{
			auto perk = ctx.FindPerk(Perk::Wealthy);
			if(perk)
				perk->hidden = true;
			if(ctx.pc && !hidden && ctx.startup)
				ctx.pc->unit->gold += 5000;
		}
		break;
	case Perk::FilthyRich:
		{
			auto perk = ctx.FindPerk(Perk::VeryWealthy);
			if(perk)
				perk->hidden = true;
			if(ctx.pc && !hidden && ctx.startup)
				ctx.pc->unit->gold += 100'000;
		}
		break;
	case Perk::AlchemistApprentice:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		break;
	default:
		assert(0);
		break;
	}

	if(ctx.cc)
	{
		PerkInfo& info = g_perks[(int)perk];
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			++ctx.cc->perks;
			++ctx.cc->perks_max;
		}
		else
			--ctx.cc->perks;
	}

	return true;
}

//=================================================================================================
// Remove perk from unit
void TakenPerk::Remove(PerkContext& ctx)
{
	switch(perk)
	{
	case Perk::Strength:
		if(ctx.cc)
			ctx.cc->a[value].Mod(-5, false);
		else
			ctx.pc->unit->SetBase((Attribute)value, -5, ctx.startup, true);
		break;
	case Perk::Weakness:
		if(ctx.cc)
			ctx.cc->a[value].Mod(5, false);
		else
			ctx.pc->unit->SetBase((Attribute)value, -5, ctx.startup, true);
		break;
	case Perk::Skilled:
		if(ctx.cc)
		{
			ctx.cc->update_skills = true;
			ctx.cc->sp -= 3;
			ctx.cc->sp_max -= 3;
		}
		break;
	case Perk::SkillFocus:
		{
			int plus, minus, minus2;
			Split3(value, plus, minus, minus2);
			if(ctx.cc)
			{
				ctx.cc->to_update.push_back((Skill)plus);
				ctx.cc->to_update.push_back((Skill)minus);
				ctx.cc->to_update.push_back((Skill)minus2);
				ctx.cc->s[plus].Mod(-10, false);
				ctx.cc->s[minus].Mod(5, false);
				ctx.cc->s[minus2].Mod(5, false);
			}
			else
			{
				ctx.pc->unit->SetBase((Skill)plus, -10, ctx.startup, true);
				ctx.pc->unit->SetBase((Skill)minus, 5, ctx.startup, true);
				ctx.pc->unit->SetBase((Skill)minus2, 5, ctx.startup, true);
			}
		}
		break;
	case Perk::Talent:
		if(ctx.cc)
		{
			ctx.cc->s[value].Mod(-5, false);
			ctx.cc->to_update.push_back((Skill)value);
		}
		else
			ctx.pc->unit->SetBase((Skill)value, -5, ctx.startup, true);
		break;
	case Perk::VeryWealthy:
		{
			auto perk = ctx.FindPerk(Perk::Wealthy);
			if(perk)
				perk->hidden = false;
		}
		break;
	case Perk::FilthyRich:
		{
			auto perk = ctx.FindPerk(Perk::VeryWealthy);
			if(perk)
				perk->hidden = false;
		}
		break;
	case Perk::AlchemistApprentice:
	case Perk::Wealthy:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		break;
	default:
		assert(0);
		break;
	}

	if(ctx.cc)
	{
		PerkInfo& info = g_perks[(int)perk];
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			--ctx.cc->perks;
			--ctx.cc->perks_max;
		}
		else
			++ctx.cc->perks;
		ctx.cc->taken_perks.erase(ctx.cc->taken_perks.begin() + ctx.index);
	}
	else
		ctx.pc->perks.erase(ctx.pc->perks.begin() + ctx.index);
}

//=================================================================================================
bool PerkContext::HavePerk(Perk perk)
{
	if(cc)
		return cc->HavePerk(perk);
	else
	{
		for(auto& p : pc->perks)
		{
			if(p.perk == perk)
				return true;
		}
		return false;
	}
}

//=================================================================================================
TakenPerk* PerkContext::FindPerk(Perk perk)
{
	if(cc)
	{
		for(auto& tp : cc->taken_perks)
		{
			if(tp.perk == perk)
				return &tp;
		}
	}
	else
	{
		for(auto& tp : pc->perks)
		{
			if(tp.perk == perk)
				return &tp;
		}
	}
	return nullptr;
}
