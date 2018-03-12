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
	EffectId::MagicResistance, "magic_resistance", "magic resistance mod %",
	EffectId::Stun, "stun", "unit is stunned"
};
