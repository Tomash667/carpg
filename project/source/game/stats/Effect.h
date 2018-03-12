#pragma once

//-----------------------------------------------------------------------------
enum class EffectId
{
	None,
	Health, // mod max hp
	Carry, // mod carry
	Poison, // takes poison damage (sum all)
	Alcohol, // takes alcohol damage (sum all)
	Regeneration, // regenerate hp (sum from perks and top from potion)
	StaminaRegeneration, // regenerate stamina (sum from perks and top from potion)
	FoodRegeneration, // food regenerate hp (sum all)
	NaturalHealingMod, // natural healing mod (timer in days)
	MagicResistance, // magic resistance mod
	Stun, // unit is stunned

	Max
};

//-----------------------------------------------------------------------------
struct Effect
{
	EffectId effect;
	float time, power;
};

//-----------------------------------------------------------------------------
struct EffectInfo
{
	EffectId effect;
	cstring id, desc;

	static EffectInfo effects[(uint)EffectId::Max];
};
