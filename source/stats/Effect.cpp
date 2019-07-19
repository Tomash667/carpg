#include "Pch.h"
#include "GameCore.h"
#include "Effect.h"

EffectInfo EffectInfo::effects[] = {
	EffectInfo(EffectId::Health, "health", "modify max hp"),
	EffectInfo(EffectId::Carry, "carry", "modify carry %"),
	EffectInfo(EffectId::Poison, "poison", "deal poison damage"),
	EffectInfo(EffectId::Alcohol, "alcohol", "deal alcohol damage"),
	EffectInfo(EffectId::Regeneration, "regeneration", "regenerate hp"),
	EffectInfo(EffectId::StaminaRegeneration, "stamina_regeneration", "regenerate stamina"),
	EffectInfo(EffectId::FoodRegeneration, "food_regeneration", "food regenerate hp"),
	EffectInfo(EffectId::NaturalHealingMod, "natural_healing_mod", "natural healing modifier % (timer in days)"),
	EffectInfo(EffectId::MagicResistance, "magic_resistance", "magic resistance %"),
	EffectInfo(EffectId::Stun, "stun", "unit is stunned"),
	EffectInfo(EffectId::Mobility, "mobility", "modify mobility"),
	EffectInfo(EffectId::MeleeAttack, "melee_attack", "modify melee attack"),
	EffectInfo(EffectId::RangedAttack, "ranged_attack", "modify ranged attack"),
	EffectInfo(EffectId::Defense, "defense", "modify defense"),
	EffectInfo(EffectId::PoisonResistance, "poison_resistance", "poison resistance %"),
	EffectInfo(EffectId::Stamina, "stamina", "modify max stamina"),
	EffectInfo(EffectId::Attribute, "attribute", "modify attribute", EffectInfo::Attribute),
	EffectInfo(EffectId::Skill, "skill", "modify skill", EffectInfo::Skill),
	EffectInfo(EffectId::StaminaRegenerationMod, "stamina_regeneration_mod", "stamina regeneration modifier %"),
	EffectInfo(EffectId::MagicPower, "magic_power", "magic power"),
	EffectInfo(EffectId::Backstab, "backstab", "backstab modifier"),
	EffectInfo(EffectId::Heal, "heal", "heal damage on consume"),
	EffectInfo(EffectId::Antidote, "antidote", "heal poison on consume"),
	EffectInfo(EffectId::GreenHair, "green_hair", "turn hair green on consume")
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
