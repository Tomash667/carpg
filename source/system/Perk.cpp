#include "Pch.h"
#include "Perk.h"

#include "Attribute.h"
#include "CreatedCharacter.h"
#include "Language.h"
#include "PlayerController.h"
#include "Skill.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
vector<Perk*> Perk::perks;
std::map<int, Perk*> Perk::hash_perks;

//=================================================================================================
::Perk* old::Convert(Perk perk)
{
	cstring id;
	switch(perk)
	{
	case Perk::BadBack:
		id = "bad_back";
		break;
	case Perk::ChronicDisease:
		id = "chronic_disease";
		break;
	case Perk::Sluggish:
		id = "sluggish";
		break;
	case Perk::SlowLearner:
		id = "slow_learner";
		break;
	case Perk::Asocial:
		id = "asocial";
		break;
	case Perk::Poor:
		id = "poor";
		break;
	case Perk::Talent:
		id = "talent";
		break;
	case Perk::Skilled:
		id = "skilled";
		break;
	case Perk::SkillFocus:
		id = "skill_focus";
		break;
	case Perk::AlchemistApprentice:
		id = "alchemist_apprentice";
		break;
	case Perk::Wealthy:
		id = "wealthy";
		break;
	case Perk::VeryWealthy:
		id = "very_wealthy";
		break;
	case Perk::FamilyHeirloom:
		id = "heirloom";
		break;
	case Perk::Leader:
		id = "leader";
		break;
	case Perk::StrongBack:
		id = "strong_back";
		break;
	case Perk::Aggressive:
		id = "aggressive";
		break;
	case Perk::Mobility:
		id = "mobility";
		break;
	case Perk::Finesse:
		id = "finesse";
		break;
	case Perk::Tough:
		id = "tough";
		break;
	case Perk::HardSkin:
		id = "hard_skin";
		break;
	case Perk::Adaptation:
		id = "adaptation";
		break;
	case Perk::PerfectHealth:
		id = "perfect_health";
		break;
	case Perk::Energetic:
		id = "energetic";
		break;
	case Perk::StrongAura:
		id = "strong_aura";
		break;
	case Perk::ManaHarmony:
		id = "mana_harmony";
		break;
	case Perk::MagicAdept:
		id = "magic_adept";
		break;
	case Perk::TravelingMerchant:
		id = "traveling_merchant";
		break;
	default:
		return nullptr;
	}
	return ::Perk::Get(id);
}

//=================================================================================================
Perk* Perk::Get(int hash)
{
	auto it = hash_perks.find(hash);
	if(it != hash_perks.end())
		return it->second;
	return nullptr;
}

//=================================================================================================
void Perk::Validate(uint& err)
{
	for(Perk* perk : perks)
	{
		if(perk->name.empty())
		{
			Warn("Test: Perk %s: empty name.", perk->id.c_str());
			++err;
		}
		if(perk->desc.empty())
		{
			Warn("Test: Perk %s: empty desc.", perk->id.c_str());
			++err;
		}
	}
}

//=================================================================================================
bool PerkContext::HavePerk(Perk* perk)
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
bool PerkContext::Have(SkillId s, int value)
{
	if(cc)
		return cc->s[(int)s].value >= value;
	else
		return pc->unit->stats->skill[(int)s] >= value;
}

//=================================================================================================
bool PerkContext::CanMod(AttributeId a)
{
	if(check_remove)
		return true;
	if(cc)
		return !cc->a[(int)a].mod;
	else
		return true;
}

//=================================================================================================
void PerkContext::AddEffect(Perk* perk, EffectId effect, float power)
{
	if(pc)
	{
		Effect e;
		e.effect = effect;
		e.source = EffectSource::Perk;
		e.source_id = perk->hash;
		e.value = -1;
		e.power = power;
		e.time = 0.f;
		pc->unit->AddEffect(e, false); // don't send ADD_EFFECT, adding perk will add effects
	}
}

//=================================================================================================
void PerkContext::RemoveEffect(Perk* perk)
{
	if(pc)
		pc->unit->RemoveEffects(EffectId::None, EffectSource::Perk, perk->hash, -1);
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
	}
}

//=================================================================================================
void PerkContext::Mod(SkillId s, int value, bool mod)
{
	if(cc)
	{
		cc->s[(int)s].Mod(value, mod);
		cc->toUpdate.push_back(s);
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
void PerkContext::AddPerk(Perk* perk)
{
	if(cc)
		cc->takenPerks.push_back(TakenPerk(perk));
	else
		pc->AddPerk(perk, -1);
}

//=================================================================================================
void PerkContext::RemovePerk(Perk* perk)
{
	if(cc)
	{
		for(auto it = cc->takenPerks.begin(), end = cc->takenPerks.end(); it != end; ++it)
		{
			if(it->perk == perk)
			{
				cc->takenPerks.erase(it);
				return;
			}
		}
	}
	else
		pc->RemovePerk(perk, -1);
}

//=================================================================================================
void TakenPerk::GetDetails(string& str) const
{
	str = perk->details;
	uint pos = str.find("{value}", 0);
	if(pos != string::npos)
	{
		cstring text;
		switch(perk->value_type)
		{
		case Perk::Attribute:
			text = Attribute::attributes[value].name.c_str();
			break;
		case Perk::Skill:
			text = Skill::skills[value].name.c_str();
			break;
		default:
			assert(0);
			return;
		}
		str.replace(pos, 7, text);
	}
}

//=================================================================================================
bool TakenPerk::CanTake(PerkContext& ctx)
{
	// can take start perk only at startup
	if(IsSet(perk->flags, Perk::Start) && !ctx.startup)
		return false;

	if(ctx.cc && !ctx.check_remove && !ctx.validate)
	{
		// can take only one history perk
		if(IsSet(perk->flags, Perk::History))
		{
			for(TakenPerk& tp : ctx.cc->takenPerks)
			{
				if(IsSet(tp.perk->flags, Perk::History))
					return false;
			}
		}

		// can't take more then 2 flaws
		if(IsSet(perk->flags, Perk::Flaw) && ctx.cc->perksMax >= 4)
			return false;
	}

	// check requirements
	for(Perk::Required& req : perk->required)
	{
		switch(req.type)
		{
		case Perk::Required::HAVE_PERK:
			if(!ctx.HavePerk(Perk::Get(req.subtype)))
			{
				if(!(ctx.validate && perk->parent == req.subtype))
					return false;
			}
			break;
		case Perk::Required::HAVE_NO_PERK:
			if(ctx.HavePerk(Perk::Get(req.subtype)))
				return false;
			break;
		case Perk::Required::HAVE_ATTRIBUTE:
			if(!ctx.Have((AttributeId)req.subtype, req.value))
				return false;
			break;
		case Perk::Required::HAVE_SKILL:
			if(!ctx.Have((SkillId)req.subtype, req.value))
				return false;
			break;
		case Perk::Required::CAN_MOD:
			if(!ctx.CanMod((AttributeId)req.subtype))
				return false;
			break;
		}
	}

	// check value type
	if(ctx.validate)
	{
		if(perk->value_type == Perk::Attribute)
		{
			if(value < 0 || value >= (int)AttributeId::MAX)
			{
				Error("Invalid perk 'Talent' value %d.", value);
				return false;
			}
		}
		else if(perk->value_type == Perk::Skill)
		{
			if(value < 0 || value >= (int)SkillId::MAX)
			{
				Error("Invalid perk 'SkillFocus' value %d.", value);
				return false;
			}
		}
	}

	return true;
}

//=================================================================================================
void TakenPerk::Apply(PerkContext& ctx)
{
	if(perk->parent != 0)
	{
		ctx.RemovePerk(Perk::Get(perk->parent));
		if(ctx.validate)
			ctx.cc->perks--; // takes two points
	}

	for(Perk::Effect& e : perk->effects)
	{
		switch(e.type)
		{
		case Perk::Effect::ATTRIBUTE:
			{
				AttributeId a = (AttributeId)e.subtype;
				if(a == AttributeId::NONE)
					a = (AttributeId)value;
				ctx.Mod(a, e.value, true);
			}
			break;
		case Perk::Effect::SKILL:
			{
				SkillId s = (SkillId)e.subtype;
				if(s == SkillId::NONE)
					s = (SkillId)value;
				ctx.Mod(s, e.value, true);
			}
			break;
		case Perk::Effect::EFFECT:
			ctx.AddEffect(perk, (EffectId)e.subtype, e.valuef);
			break;
		case Perk::Effect::APTITUDE:
			if(ctx.pc)
			{
				for(int i = 0; i < (int)SkillId::MAX; ++i)
					ctx.pc->skill[i].apt += e.value;
			}
			break;
		case Perk::Effect::SKILL_POINT:
			if(ctx.cc)
			{
				ctx.cc->sp += e.value;
				ctx.cc->spMax += e.value;
				ctx.cc->updateSkills = true;
			}
			break;
		case Perk::Effect::GOLD:
			if(ctx.pc && ctx.startup)
				ctx.pc->unit->gold += e.value;
			break;
		}
	}

	if(ctx.cc)
	{
		if(IsSet(perk->flags, Perk::Flaw))
		{
			ctx.cc->perksMax++;
			ctx.cc->perks++;
		}
		else
			ctx.cc->perks--;
	}
}

//=================================================================================================
void TakenPerk::Remove(PerkContext& ctx)
{
	if(perk->parent != 0)
		ctx.AddPerk(Perk::Get(perk->parent));

	for(Perk::Effect& e : perk->effects)
	{
		switch(e.type)
		{
		case Perk::Effect::ATTRIBUTE:
			{
				AttributeId a = (AttributeId)e.subtype;
				if(a == AttributeId::NONE)
					a = (AttributeId)value;
				ctx.Mod(a, -e.value, false);
			}
			break;
		case Perk::Effect::SKILL:
			{
				SkillId s = (SkillId)e.subtype;
				if(s == SkillId::NONE)
					s = (SkillId)value;
				ctx.Mod(s, -e.value, false);
			}
			break;
		case Perk::Effect::SKILL_POINT:
			if(ctx.cc)
			{
				ctx.cc->sp -= e.value;
				ctx.cc->spMax -= e.value;
				ctx.cc->updateSkills = true;
			}
			break;
		}
	}

	ctx.RemoveEffect(perk);

	if(ctx.cc)
	{
		if(IsSet(perk->flags, Perk::Flaw))
		{
			ctx.cc->perksMax--;
			ctx.cc->perks--;
		}
		else
			ctx.cc->perks++;
	}
}

//=================================================================================================
cstring TakenPerk::FormatName()
{
	switch(perk->value_type)
	{
	case Perk::Attribute:
		return Format("%s (%s)", perk->name.c_str(), Attribute::attributes[value].name.c_str());
	case Perk::Skill:
		return Format("%s (%s)", perk->name.c_str(), Skill::skills[value].name.c_str());
	default:
		assert(0);
		return perk->name.c_str();
	}
}
