#include "Pch.h"
#include "Core.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Language.h"
#include "PlayerController.h"
#include "CreatedCharacter.h"
#include "Unit.h"
#include "Net.h"
#include "PlayerInfo.h"

//-----------------------------------------------------------------------------
PerkInfo g_perks[(int)Perk::Max] = {
	PerkInfo(Perk::BadBack, "bad_back", PerkInfo::Flaw | PerkInfo::History),
	PerkInfo(Perk::ChronicDisease, "chronic_disease", PerkInfo::Flaw | PerkInfo::History),
	PerkInfo(Perk::Sluggish, "sluggish", PerkInfo::Flaw | PerkInfo::History),
	PerkInfo(Perk::SlowLearner, "slow_learner", PerkInfo::Flaw | PerkInfo::History),
	PerkInfo(Perk::Asocial, "asocial", PerkInfo::Flaw | PerkInfo::History),
	PerkInfo(Perk::Talent, "talent", PerkInfo::History | PerkInfo::RequireFormat, PerkInfo::Attribute),
	PerkInfo(Perk::Skilled, "skilled", PerkInfo::History),
	PerkInfo(Perk::SkillFocus, "skill_focus", PerkInfo::History | PerkInfo::RequireFormat, PerkInfo::Skill),
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
	case Perk::BadBack:
	case Perk::ChronicDisease:
	case Perk::Sluggish:
	case Perk::SlowLearner:
	case Perk::Asocial:
		s.clear();
		break;
	case Perk::Talent:
		s = Format("%s: %s", txIncreasedAttrib, AttributeInfo::attributes[value].name.c_str());
		break;
	case Perk::SkillFocus:
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
	case Perk::Talent:
		return Format("%s (%s)", p.name.c_str(), AttributeInfo::attributes[value].name.c_str());
	case Perk::SkillFocus:
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
	case Perk::Talent:
		if(value < 0 || value >= (int)Attribute::MAX)
		{
			Error("Perk 'strength', invalid attribute %d.", value);
			return false;
		}
		break;
	case Perk::SkillFocus:
		if(value < 0 || value >= (int)Skill::MAX)
		{
			Error("Perk 'skill_focus', invalid skill %d.", value);
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
	auto& info = g_perks[(int)perk];

	// can take history perk only at startup
	if(IS_SET(info.flags, PerkInfo::History) && !ctx.startup)
		return false;

	// can take more then 2 flaws
	if(IS_SET(info.flags, PerkInfo::Flaw) && ctx.cc && ctx.cc->perks_max >= 4)
		return false;

	switch(perk)
	{
	case Perk::Talent:
	case Perk::Skilled:
	case Perk::SkillFocus:
	case Perk::AlchemistApprentice:
	case Perk::Wealthy:
	case Perk::FamilyHeirloom:
	case Perk::Leader:
		return true;
	case Perk::VeryWealthy:
		return ctx.HavePerk(Perk::Wealthy);
	case Perk::FilthyRich:
		return ctx.HavePerk(Perk::VeryWealthy);
	case Perk::BadBack:
		return ctx.CanMod(Attribute::STR);
	case Perk::ChronicDisease:
		return ctx.CanMod(Attribute::END);
	case Perk::Sluggish:
		return ctx.CanMod(Attribute::DEX);
	case Perk::SlowLearner:
		return ctx.CanMod(Attribute::INT);
	case Perk::Asocial:
		return ctx.CanMod(Attribute::CHA);
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
	case Perk::Talent:
		if(ctx.validate && ctx.cc && ctx.cc->a[value].mod)
		{
			Error("Perk 'strength', attribute %d is already modified.", value);
			return false;
		}
		ctx.Mod((Attribute)value, 5);
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
		if(ctx.validate && ctx.cc && ctx.cc->s[value].mod)
		{
			Error("Perk 'skill_focus', skill %d is already modified.", value);
			return false;
		}
		ctx.Mod((Skill)value, 5);
		break;
	case Perk::Wealthy:
		if(ctx.pc && !hidden && ctx.startup)
			ctx.pc->unit->gold += 1000;
		break;
	case Perk::VeryWealthy:
		ctx.HidePerk(Perk::Wealthy);
		if(ctx.pc && !hidden && ctx.startup)
			ctx.pc->unit->gold += 5000;
		break;
	case Perk::FilthyRich:
		ctx.HidePerk(Perk::VeryWealthy);
		if(ctx.pc && !hidden && ctx.startup)
			ctx.pc->unit->gold += 100'000;
		break;
	case Perk::BadBack:
		ctx.Mod(Attribute::STR, -5);
		break;
	case Perk::ChronicDisease:
		ctx.Mod(Attribute::END, -5);
		break;
	case Perk::Sluggish:
		ctx.Mod(Attribute::DEX, -5);
		break;
	case Perk::SlowLearner:
		ctx.Mod(Attribute::INT, -5);
		if(ctx.pc)
			ctx.pc->unit->statsx->perk_flags |= PerkFlags::PF_SLOW_LERNER;
		break;
	case Perk::Asocial:
		ctx.Mod(Attribute::CHA, -5);
		if(ctx.pc)
			ctx.pc->unit->statsx->perk_flags |= PerkFlags::PF_ASOCIAL;
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
		ctx.cc->taken_perks.push_back(*this);
	}
	else
	{
		ctx.pc->unit->statsx->perks.push_back(*this);
		if(!ctx.pc->is_local)
		{
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::ADD_PERK;
			c.id = (byte)perk;
			c.ile = value;
		}
	}

	return true;
}

//=================================================================================================
// Remove perk from unit
void TakenPerk::Remove(PerkContext& ctx)
{
	switch(perk)
	{
	case Perk::Talent:
		ctx.Mod((Attribute)value, -5, false);
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
		ctx.Mod((Skill)value, -5, false);
		break;
	case Perk::VeryWealthy:
		ctx.HidePerk(Perk::Wealthy, false);
		break;
	case Perk::FilthyRich:
		ctx.HidePerk(Perk::VeryWealthy, false);
		break;
	case Perk::BadBack:
		ctx.Mod(Attribute::STR, -5, false);
		break;
	case Perk::ChronicDisease:
		ctx.Mod(Attribute::END, -5, false);
		break;
	case Perk::Sluggish:
		ctx.Mod(Attribute::DEX, -5, false);
		break;
	case Perk::SlowLearner:
		ctx.Mod(Attribute::INT, -5, false);
		if(ctx.pc)
			ctx.pc->unit->statsx->perk_flags &= ~PerkFlags::PF_SLOW_LERNER;
		break;
	case Perk::Asocial:
		ctx.Mod(Attribute::CHA, -5, false);
		if(ctx.pc)
			ctx.pc->unit->statsx->perk_flags &= ~PerkFlags::PF_ASOCIAL;
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
	{
		ctx.pc->unit->statsx->perks.erase(ctx.pc->unit->statsx->perks.begin() + ctx.index);
		if(!ctx.pc->is_local)
		{
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::REMOVE_PERK;
			c.id = (int)perk;
		}
	}
}

//=================================================================================================
bool PerkContext::HavePerk(Perk perk)
{
	if(cc)
		return cc->HavePerk(perk);
	else
	{
		for(auto& p : pc->unit->statsx->perks)
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
		for(auto& tp : pc->unit->statsx->perks)
		{
			if(tp.perk == perk)
				return &tp;
		}
	}
	return nullptr;
}

//=================================================================================================
void PerkContext::HidePerk(Perk perk, bool hide)
{
	auto taken_perk = FindPerk(perk);
	if(taken_perk && taken_perk->hidden != hide)
	{
		taken_perk->hidden = hide;
		if(!startup && Net::IsServer() && !pc->is_local)
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::HIDE_PERK;
			c.id = (int)perk;
			c.ile = (hide ? 1 : 0);
		}
	}
}

//=================================================================================================
bool PerkContext::CanMod(Attribute attrib)
{
	if(!cc)
		return true;
	return !cc->a[(int)attrib].mod;
}

//=================================================================================================
bool PerkContext::CanMod(Skill skill)
{
	if(!cc)
		return true;
	return !cc->s[(int)skill].mod;
}

//=================================================================================================
void PerkContext::Mod(Attribute attrib, int value, bool mod)
{
	if(cc)
		cc->a[(int)attrib].Mod(value, mod);
	else
		pc->unit->SetBase(attrib, value, startup, true);
}

//=================================================================================================
void PerkContext::Mod(Skill skill, int value, bool mod)
{
	if(cc)
	{
		cc->s[(int)skill].Mod(value, mod);
		cc->to_update.push_back(skill);
	}
	else
		pc->unit->SetBase(skill, value, startup, true);
}
