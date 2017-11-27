#include "Pch.h"
#include "Core.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Language.h"
#include "PlayerController.h"
#include "CreatedCharacter.h"
#include "Unit.h"

struct PerkContext
{
	PerkContext& HavePerkToReplace(Perk perk);
	bool HavePerk(Perk perk);
	TakenPerk* FindPerk(Perk perk);

	CreatedCharacter* cc;
	PlayerController* pc;
	bool validate, startup;
};

//-----------------------------------------------------------------------------
PerkInfo g_perks[(int)Perk::Max] = {
	PerkInfo(Perk::Weakness, "weakness", PerkInfo::Free | PerkInfo::Flaw | PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Strength, "strength", PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Skilled, "skilled", PerkInfo::History),
	PerkInfo(Perk::SkillFocus, "skill_focus", PerkInfo::Free | PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::Talent, "talent", PerkInfo::Multiple | PerkInfo::History | PerkInfo::RequireFormat),
	PerkInfo(Perk::AlchemistApprentice, "alchemist", PerkInfo::History),
	PerkInfo(Perk::Wealthy, "wealthy", PerkInfo::History),
	PerkInfo(Perk::VeryWealthy, "very_wealthy", PerkInfo::History, [](PerkContext& c) { c.HavePerkToReplace(Perk::Wealthy); }),
	PerkInfo(Perk::FilthyRich, "filthy_rich", PerkInfo::History, [](PerkContext& c) { c.HavePerkToReplace(Perk::FilthyRich); }),
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

void Unit::Mod(Attribute attrib, int value, bool startup)
{
	int a = (int)attrib;
	if(startup)
		statsx->attrib[a] += value;
	else
	{
		int new_value = Clamp(statsx->attrib[a] + value, 1, AttributeInfo::MAX);
		new_value = Clamp(new_value, 1, AttributeInfo::MIN, AttributeInfo::MAX);
		Set(new_value);
		// !!! new apt
	}
}

void Unit::Mod(Skill skill, int value, bool startup)
{
	int s = (int)skill;
	if(startup)
		statsx->skill[a] += value;
	else
	{
		int new_value = Clamp(statsx->attrib[a] + value, 1, AttributeInfo::MAX);
		new_value = Clamp(new_value, 1, AttributeInfo::MIN, AttributeInfo::MAX);
		Set(new_value);
	}
}

bool TakenPerk::Apply(PerkContext& ctx)
{
	//PerkInfo& info = g_perks[(int)perk];

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
			ctx.pc->unit->statsx->attrib[value] += 5;
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
			ctx.pc->unit->statsx->attrib[value] -= 5;
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
				ctx.pc->unit->statsx->skill[plus] += v[0];
				ctx.pc->unit->statsx->skill[minus] -= v[1];
				ctx.pc->unit->statsx->skill[minus2] -= v[2];
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
			ctx.pc->unit->statsx->skill[value] += 5;
		break;
	case Perk::Wealthy:
		if(ctx.pc && !hidden && ctx.startup)
			pc.unit->gold += 1000;
		break;
	case Perk::VeryWealthy:
		{
			auto perk = ctx.FindPerk(Perk::Wealthy);
			if(perk)
				perk->hidden = true;
			if(ctx.pc && !hidden && ctx.startup)
				pc.unit->gold += 5000;
		}
		break;
	case Perk::FilthyRich:
		{
			auto perk = ctx.FindPerk(Perk::VeryWealthy);
			if(perk)
				perk->hidden = true;
			if(ctx.pc && !hidden && ctx.startup)
				pc.unit->gold += 100'000;
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
void TakenPerk::Remove(PerkContext& ctx)
{
	PerkInfo& info = g_perks[(int)perk];
	bool add = false;

	switch(perk)
	{
	case Perk::Strength:
		if(ctx.cc)
			ctx.cc->a[value].Mod(-5, false);
		else
			ctx.pc->unit->statsx->
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
			cc.to_update.push_back((Skill)plus);
			cc.to_update.push_back((Skill)minus);
			cc.to_update.push_back((Skill)minus2);
			cc.s[plus].Mod(-10, false);
			cc.s[minus].Mod(5, false);
			cc.s[minus2].Mod(5, false);
		}
		break;
	case Perk::Talent:
		cc.s[value].Mod(-5, false);
		cc.to_update.push_back((Skill)value);
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
