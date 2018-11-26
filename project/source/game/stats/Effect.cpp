#include "Pch.h"
#include "GameCore.h"
#include "Effect.h"

EffectInfo EffectInfo::effects[] = {
	EffectId::Health, "health", "modify max hp",
	EffectId::Carry, "carry", "modify carry %",
	EffectId::Poison, "poison", "deal poison damage",
	EffectId::Alcohol, "alcohol", "deal alcohol damage",
	EffectId::Regeneration, "regeneration", "regenerate hp",
	EffectId::StaminaRegeneration, "stamina_regeneration", "regenerate stamina",
	EffectId::FoodRegeneration, "food_regneration", "food regenerate hp",
	EffectId::NaturalHealingMod, "natural_healing_mod", "natural healing modifier % (timer in days)",
	EffectId::MagicResistance, "magic_resistance", "magic resistance %",
	EffectId::Stun, "stun", "unit is stunned",
	EffectId::Mobility, "mobility", "modify mobility",
	EffectId::MeleeAttack, "melee_attack", "modify melee attack",
	EffectId::RangedAttack, "ranged_attack", "modify ranged attack",
	EffectId::Defense, "defense", "modify defense",
	EffectId::PoisonResistance, "poison_resistance", "poison resistance %"
};

EffectId EffectInfo::TryGet(const string& id)
{
	for(EffectInfo& info : effects)
	{
		if(info.id == id)
			return info.effect;
	}
	return EffectId::None;
}
