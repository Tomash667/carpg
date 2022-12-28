#include "Pch.h"
#include "Effect.h"

EffectInfo EffectInfo::effects[] = {
	EffectInfo(EffectId::Health, "health", "modify max hp"),
	EffectInfo(EffectId::Carry, "carry", "modify carry %"),
	EffectInfo(EffectId::Poison, "poison", "deal poison damage"),
	EffectInfo(EffectId::Alcohol, "alcohol", "deal alcohol damage"),
	EffectInfo(EffectId::Regeneration, "regeneration", "regenerate hp"),
	EffectInfo(EffectId::StaminaRegeneration, "staminaRegeneration", "regenerate stamina"),
	EffectInfo(EffectId::FoodRegeneration, "foodRegeneration", "food regenerate hp"),
	EffectInfo(EffectId::NaturalHealingMod, "naturalHealingMod", "natural healing modifier % (timer in days)"),
	EffectInfo(EffectId::MagicResistance, "magicResistance", "magic resistance %"),
	EffectInfo(EffectId::Stun, "stun", "unit is stunned"),
	EffectInfo(EffectId::Mobility, "mobility", "modify mobility"),
	EffectInfo(EffectId::MeleeAttack, "meleeAttack", "modify melee attack"),
	EffectInfo(EffectId::RangedAttack, "rangedAttack", "modify ranged attack"),
	EffectInfo(EffectId::Defense, "defense", "modify defense"),
	EffectInfo(EffectId::PoisonResistance, "poisonResistance", "poison resistance %"),
	EffectInfo(EffectId::Stamina, "stamina", "modify max stamina"),
	EffectInfo(EffectId::Attribute, "attribute", "modify attribute", EffectInfo::Attribute),
	EffectInfo(EffectId::Skill, "skill", "modify skill", EffectInfo::Skill),
	EffectInfo(EffectId::StaminaRegenerationMod, "staminaRegenerationMod", "stamina regeneration modifier %"),
	EffectInfo(EffectId::MagicPower, "magicPower", "magic power"),
	EffectInfo(EffectId::Backstab, "backstab", "backstab modifier"),
	EffectInfo(EffectId::Heal, "heal", "heal damage on consume"),
	EffectInfo(EffectId::Antidote, "antidote", "heal poison on consume"),
	EffectInfo(EffectId::GreenHair, "greenHair", "turn hair green on consume"),
	EffectInfo(EffectId::Mana, "mana", "modify max mp"),
	EffectInfo(EffectId::RestoreMana, "restoreMana", "restore mana on consume"),
	EffectInfo(EffectId::ManaRegeneration, "manaRegeneration", "regenerate mp"),
	EffectInfo(EffectId::Rooted, "rooted", "unit can't move"),
	EffectInfo(EffectId::SlowMove, "slowMove", "unit move speed is slowed")
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
