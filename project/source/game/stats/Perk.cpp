#include "Pch.h"
#include "GameCore.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Language.h"
#include "PlayerController.h"
#include "CreatedCharacter.h"
#include "Unit.h"

template<typename T>
inline constexpr T PrevEnum(T val)
{
	return (T)((int)val - 1);
}

//-----------------------------------------------------------------------------
PerkInfo PerkInfo::perks[] = {
	// negative starting perks
	PerkInfo(Perk::BadBack, "bad_back", PerkInfo::Flaw | PerkInfo::History, 0),
	PerkInfo(Perk::ChronicDisease, "chronic_disease", PerkInfo::Flaw | PerkInfo::History, 0),
	PerkInfo(Perk::Sluggish, "sluggish", PerkInfo::Flaw | PerkInfo::History, 0),
	PerkInfo(Perk::SlowLearner, "slow_learner", PerkInfo::Flaw | PerkInfo::History, 0),
	PerkInfo(Perk::Asocial, "asocial", PerkInfo::Flaw | PerkInfo::History, 0),
	PerkInfo(Perk::Poor, "poor", PerkInfo::Flaw | PerkInfo::History, 0),
	// positive starting perks
	PerkInfo(Perk::Talent, "talent", PerkInfo::History | PerkInfo::RequireFormat, 0, PerkInfo::Attribute),
	PerkInfo(Perk::Skilled, "skilled", PerkInfo::History, 0),
	PerkInfo(Perk::SkillFocus, "skill_focus", PerkInfo::History | PerkInfo::RequireFormat, 0, PerkInfo::Skill),
	PerkInfo(Perk::AlchemistApprentice, "alchemist", PerkInfo::History, 0),
	PerkInfo(Perk::Wealthy, "wealthy", PerkInfo::History, 0),
	PerkInfo(Perk::VeryWealthy, "very_wealthy", PerkInfo::History, 0),
	PerkInfo(Perk::FamilyHeirloom, "heirloom", PerkInfo::History, 0),
	PerkInfo(Perk::Leader, "leader", PerkInfo::History, 0),
	// normal perks
	PerkInfo(Perk::StrongBack, "strong_back", 0, 2),
	PerkInfo(Perk::Aggressive, "aggressive", 0, 2),
	PerkInfo(Perk::Mobility, "mobility", 0, 2),
	PerkInfo(Perk::Finesse, "finesse", 0, 2),
	PerkInfo(Perk::Tough, "tough", 0, 2),
	PerkInfo(Perk::HardSkin, "hard_skin", 0, 2),
	PerkInfo(Perk::Adaptation, "adaptation", 0, 3),
	PerkInfo(Perk::PerfectHealth, "perfect_health", 0, 5),
	PerkInfo(Perk::Energetic, "energetic", 0, 2)
};

//-----------------------------------------------------------------------------
cstring TakenPerk::txIncreasedAttrib, TakenPerk::txIncreasedSkill, TakenPerk::txDecreasedAttrib, TakenPerk::txDecreasedSkill;

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
bool PerkContext::HavePerk(Perk perk)
{
	if(cc)
		return cc->HavePerk(perk);
	else
		return pc->HavePerk(perk, -1);
}

//=================================================================================================
bool PerkContext::Have(AttributeId a, int value)
{
	if(cc)
		return cc->a[(int)a].value >= value;
	else
		return pc->unit->stats->attrib[(int)a] >= value;
}

//=================================================================================================
bool PerkContext::CanMod(AttributeId a, bool positive)
{
	if(check_remove)
		return true;
	if(cc)
	{
		auto& ad = cc->a[(int)a];
		if(!positive && ad.required > 0)
			return false;
		return !ad.mod;
	}
	else
		return !pc->attrib[(int)a].blocked;
}

//=================================================================================================
void PerkContext::AddEffect(Perk perk, EffectId effect, float power)
{
	if(pc)
	{
		Effect e;
		e.effect = effect;
		e.source = EffectSource::Perk;
		e.source_id = (int)perk;
		e.value = -1;
		e.power = power;
		e.time = 0.f;
		pc->unit->AddEffect(e, false); // don't send ADD_EFFECT, adding perk will add effects
	}
}

//=================================================================================================
void PerkContext::RemoveEffect(Perk perk)
{
	if(pc)
		pc->unit->RemoveEffects(EffectId::None, EffectSource::Perk, (int)perk, -1);
}

//=================================================================================================
void PerkContext::Mod(AttributeId a, int value, bool mod)
{
	if(cc)
		cc->a[(int)a].Mod(value, mod);
	else
	{
		if(startup)
			pc->unit->stats->attrib[(int)a] += value;
		else
			pc->unit->Set(a, pc->unit->GetBase(a) + value);
		pc->attrib[(int)a].apt = (pc->unit->stats->attrib[(int)a] - 50) / 5;
		if(mod && value < 0)
			pc->attrib[(int)a].blocked = true;
		else if(!mod && value > 0)
			pc->attrib[(int)a].blocked = false;
	}
}

//=================================================================================================
void PerkContext::Mod(SkillId s, int value, bool mod)
{
	if(cc)
	{
		cc->s[(int)s].Mod(value, mod);
		cc->to_update.push_back(s);
	}
	else
	{
		if(startup)
			pc->unit->stats->skill[(int)s] += value;
		else
			pc->unit->Set(s, pc->unit->GetBase(s) + value);
		pc->skill[(int)s].apt = pc->unit->stats->skill[(int)s] / 5;
	}
}

//=================================================================================================
void PerkContext::AddPerk(Perk perk)
{
	if(cc)
		cc->taken_perks.push_back(TakenPerk(perk));
	else
		pc->AddPerk(perk, -1);
}

//=================================================================================================
void PerkContext::RemovePerk(Perk perk)
{
	if(cc)
	{
		for(auto it = cc->taken_perks.begin(), end = cc->taken_perks.end(); it != end; ++it)
		{
			if(it->perk == perk)
			{
				cc->taken_perks.erase(it);
				return;
			}
		}
	}
	else
		pc->RemovePerk(perk, -1);
}

//=================================================================================================
void PerkContext::AddRequired(AttributeId a)
{
	if(cc)
		cc->a[(int)a].required++;
}

//=================================================================================================
void PerkContext::RemoveRequired(AttributeId a)
{
	if(cc)
		cc->a[(int)a].required--;
}


//=================================================================================================
void TakenPerk::LoadText()
{
	txIncreasedAttrib = Str("increasedAttrib");
	txIncreasedSkill = Str("increasedSkill");
	txDecreasedAttrib = Str("decreasedAttrib");
	txDecreasedSkill = Str("decreasedSkill");
}

//=================================================================================================
void TakenPerk::GetDesc(string& s) const
{
	switch(perk)
	{
	case Perk::Talent:
		s = Format("%s: %s", txIncreasedAttrib, Attribute::attributes[value].name.c_str());
		break;
	case Perk::SkillFocus:
		s = Format("%s: %s", txIncreasedSkill, Skill::skills[value].name.c_str());
		break;
	default:
		s.clear();
		break;
	}
}

//=================================================================================================
bool TakenPerk::CanTake(PerkContext& ctx)
{
	PerkInfo& info = PerkInfo::perks[(int)perk];

	// can take history perk only at startup
	if(IS_SET(info.flags, PerkInfo::History) && !ctx.startup)
		return false;

	// can take more then 2 flaws
	if(IS_SET(info.flags, PerkInfo::Flaw) && ctx.cc && ctx.cc->perks_max >= 4)
		return false;

	switch(perk)
	{
	case Perk::BadBack:
		return ctx.CanMod(AttributeId::STR, false);
	case Perk::ChronicDisease:
		return ctx.CanMod(AttributeId::END, false);
	case Perk::Sluggish:
		return ctx.CanMod(AttributeId::DEX, false);
	case Perk::SlowLearner:
		return ctx.CanMod(AttributeId::INT, false) && !ctx.HavePerk(Perk::Skilled);
	case Perk::Asocial:
		return ctx.CanMod(AttributeId::CHA, false) && !ctx.HavePerk(Perk::Leader);
	case Perk::Poor:
		return !ctx.HavePerk(Perk::Wealthy) && !ctx.HavePerk(Perk::VeryWealthy) && !ctx.HavePerk(Perk::FamilyHeirloom);
	case Perk::Skilled:
		return !ctx.HavePerk(Perk::SlowLearner);
	case Perk::AlchemistApprentice:
		return true;
	case Perk::Talent:
		if(ctx.validate)
		{
			if(value < 0 || value >= (int)AttributeId::MAX)
			{
				Error("Invalid perk 'Talent' value %d.", value);
				return false;
			}
			return ctx.CanMod((AttributeId)value, true);
		}
		return true;
	case Perk::SkillFocus:
		if(ctx.validate)
		{
			if(value < 0 || value >= (int)SkillId::MAX)
			{
				Error("Invalid perk 'SkillFocus' value %d.", value);
				return false;
			}
			if(ctx.cc->s[value].value < 0)
			{
				Error("Perk 'SkillFocus' modify not known skill %s.", Skill::skills[value].id);
				return false;
			}
		}
		return true;
	case Perk::Wealthy:
		return !ctx.HavePerk(Perk::Poor) && !ctx.HavePerk(Perk::VeryWealthy);
	case Perk::VeryWealthy:
		if(ctx.validate)
			return !ctx.HavePerk(Perk::Poor) && !ctx.HavePerk(Perk::Wealthy);
		else
			return ctx.HavePerk(Perk::Wealthy);
	case Perk::FamilyHeirloom:
		return !ctx.HavePerk(Perk::Poor);
	case Perk::Leader:
		return !ctx.HavePerk(Perk::Asocial);
	case Perk::StrongBack:
		return ctx.Have(AttributeId::STR, 60) && !ctx.HavePerk(Perk::BadBack);
	case Perk::Aggressive:
		return ctx.Have(AttributeId::STR, 60) && !ctx.HavePerk(Perk::BadBack);
	case Perk::Mobility:
		return ctx.Have(AttributeId::DEX, 60) && !ctx.HavePerk(Perk::Sluggish);
	case Perk::Finesse:
		return ctx.Have(AttributeId::DEX, 60) && !ctx.HavePerk(Perk::Sluggish);
	case Perk::Tough:
		return ctx.Have(AttributeId::END, 60) && !ctx.HavePerk(Perk::ChronicDisease);
	case Perk::HardSkin:
		return ctx.Have(AttributeId::END, 60) && !ctx.HavePerk(Perk::ChronicDisease);
	case Perk::Adaptation:
		return ctx.Have(AttributeId::END, 75) && !ctx.HavePerk(Perk::ChronicDisease);
	case Perk::PerfectHealth:
		return ctx.Have(AttributeId::END, 90) && !ctx.HavePerk(Perk::ChronicDisease);
	case Perk::Energetic:
		return ctx.Have(AttributeId::DEX, 60) && ctx.Have(AttributeId::END, 60) && !ctx.HavePerk(Perk::Sluggish);
	default:
		assert(0);
		return true;
	}
}

//=================================================================================================
void TakenPerk::Apply(PerkContext& ctx)
{
	PerkInfo& info = PerkInfo::perks[(int)perk];

	switch(perk)
	{
	case Perk::BadBack:
		ctx.AddEffect(perk, EffectId::Carry, 0.75f);
		if(!ctx.upgrade)
			ctx.Mod(AttributeId::STR, -5, true);
		break;
	case Perk::ChronicDisease:
		ctx.AddEffect(perk, EffectId::NaturalHealingMod, 0.5f);
		if(!ctx.upgrade)
			ctx.Mod(AttributeId::END, -5, true);
		break;
	case Perk::Sluggish:
		ctx.AddEffect(perk, EffectId::Mobility, -25.f);
		if(!ctx.upgrade)
			ctx.Mod(AttributeId::DEX, -5, true);
		break;
	case Perk::SlowLearner:
		ctx.Mod(AttributeId::INT, -5, true);
		if(ctx.pc)
		{
			for(int i = 0; i < (int)SkillId::MAX; ++i)
				ctx.pc->skill[i].apt--;
		}
		break;
	case Perk::Asocial:
		ctx.Mod(AttributeId::CHA, -5, true);
		break;
	case Perk::Poor:
		if(ctx.pc && ctx.startup)
			ctx.pc->unit->gold = Random(0, 1);
		break;
	case Perk::Wealthy:
		if(ctx.pc && ctx.startup)
			ctx.pc->unit->gold += 2500;
		break;
	case Perk::VeryWealthy:
		ctx.RemovePerk(Perk::Wealthy);
		if(ctx.pc && ctx.startup)
			ctx.pc->unit->gold += 50000;
		if(ctx.validate)
			ctx.cc->perks--; // takes two points
		break;
	case Perk::Talent:
		ctx.Mod((AttributeId)value, 5, true);
		break;
	case Perk::Skilled:
		if(ctx.cc)
		{
			ctx.cc->sp += 3;
			ctx.cc->sp_max += 3;
			ctx.cc->update_skills = true;
		}
		break;
	case Perk::SkillFocus:
		ctx.Mod((SkillId)value, 5, true);
		break;
	case Perk::StrongBack:
		ctx.AddEffect(perk, EffectId::Carry, 1.25f);
		ctx.AddRequired(AttributeId::STR);
		break;
	case Perk::Aggressive:
		ctx.AddEffect(perk, EffectId::MeleeAttack, 10.f);
		ctx.AddRequired(AttributeId::STR);
		break;
	case Perk::Mobility:
		ctx.AddEffect(perk, EffectId::Mobility, 20.f);
		ctx.AddRequired(AttributeId::DEX);
		break;
	case Perk::Finesse:
		ctx.AddEffect(perk, EffectId::RangedAttack, 10.f);
		ctx.AddRequired(AttributeId::DEX);
		break;
	case Perk::Tough:
		ctx.AddEffect(perk, EffectId::Health, 100.f);
		ctx.AddRequired(AttributeId::END);
		break;
	case Perk::HardSkin:
		ctx.AddEffect(perk, EffectId::Defense, 10.f);
		ctx.AddRequired(AttributeId::END);
		break;
	case Perk::Adaptation:
		ctx.AddEffect(perk, EffectId::PoisonResistance, 0.5f);
		ctx.AddRequired(AttributeId::END);
		break;
	case Perk::PerfectHealth:
		ctx.AddEffect(perk, EffectId::Regeneration, 5.f);
		ctx.AddRequired(AttributeId::END);
		break;
	case Perk::Energetic:
		ctx.AddEffect(perk, EffectId::Stamina, 100.f);
		ctx.AddRequired(AttributeId::DEX);
		ctx.AddRequired(AttributeId::END);
		break;
	}

	if(ctx.cc)
	{
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			ctx.cc->perks_max++;
			ctx.cc->perks++;
		}
		else
			ctx.cc->perks--;
	}
}

//=================================================================================================
void TakenPerk::Remove(PerkContext& ctx)
{
	PerkInfo& info = PerkInfo::perks[(int)perk];

	switch(perk)
	{
	case Perk::BadBack:
		ctx.Mod(AttributeId::STR, 5, false);
		break;
	case Perk::ChronicDisease:
		ctx.Mod(AttributeId::END, 5, false);
		break;
	case Perk::Sluggish:
		ctx.Mod(AttributeId::DEX, 5, false);
		break;
	case Perk::SlowLearner:
		ctx.Mod(AttributeId::INT, 5, false);
		break;
	case Perk::Asocial:
		ctx.Mod(AttributeId::CHA, 5, false);
		break;
	case Perk::VeryWealthy:
		ctx.AddPerk(Perk::Wealthy);
		break;
	case Perk::Talent:
		ctx.Mod((AttributeId)value, -5, false);
		break;
	case Perk::Skilled:
		if(ctx.cc)
		{
			ctx.cc->sp -= 3;
			ctx.cc->sp_max -= 3;
			ctx.cc->update_skills = true;
		}
		break;
	case Perk::SkillFocus:
		ctx.Mod((SkillId)value, -5, false);
		break;
	case Perk::StrongBack:
		ctx.RemoveRequired(AttributeId::STR);
		break;
	case Perk::Aggressive:
		ctx.RemoveRequired(AttributeId::STR);
		break;
	case Perk::Mobility:
		ctx.RemoveRequired(AttributeId::DEX);
		break;
	case Perk::Finesse:
		ctx.RemoveRequired(AttributeId::DEX);
		break;
	case Perk::Tough:
		ctx.RemoveRequired(AttributeId::END);
		break;
	case Perk::HardSkin:
		ctx.RemoveRequired(AttributeId::END);
		break;
	case Perk::Adaptation:
		ctx.RemoveRequired(AttributeId::END);
		break;
	case Perk::PerfectHealth:
		ctx.RemoveRequired(AttributeId::END);
		break;
	case Perk::Energetic:
		ctx.RemoveRequired(AttributeId::DEX);
		ctx.RemoveRequired(AttributeId::END);
		break;
	}

	ctx.RemoveEffect(perk);

	if(ctx.cc)
	{
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			ctx.cc->perks_max--;
			ctx.cc->perks--;
		}
		else
			ctx.cc->perks++;
	}
}

//=================================================================================================
cstring TakenPerk::FormatName()
{
	PerkInfo& p = PerkInfo::perks[(int)perk];

	switch(perk)
	{
	case Perk::Talent:
		return Format("%s (%s)", p.name.c_str(), Attribute::attributes[value].name.c_str());
	case Perk::SkillFocus:
		return Format("%s (%s)", p.name.c_str(), Skill::skills[value].name.c_str());
	default:
		assert(0);
		return p.name.c_str();
	}
}
