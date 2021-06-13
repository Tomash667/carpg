#pragma once

//-----------------------------------------------------------------------------
enum class EffectId
{
	None = -1,

	Health, // mod max hp (add)
	Carry, // mod carry (multiple)
	Poison, // takes poison damage (sum all)
	Alcohol, // takes alcohol damage (sum all)
	Regeneration, // regenerate hp (sum from perks and top from potion)
	StaminaRegeneration, // regenerate stamina (use best value)
	FoodRegeneration, // food regenerate hp (sum all)
	NaturalHealingMod, // natural healing mod (timer in days, multiple)
	MagicResistance, // magic resistance mod
	Stun, // unit is stunned
	Mobility, // mod mobility
	MeleeAttack,
	RangedAttack,
	Defense,
	PoisonResistance,
	Stamina, // mod max stamina (add)
	Attribute,
	Skill,
	StaminaRegenerationMod, // multiple
	MagicPower, // sum
	Backstab, // sum
	Heal, // consumable
	Antidote, // consumable
	GreenHair, // consumable
	Mana, // mod max mp (add)
	RestoreMana, // consumable
	ManaRegeneration, // regenerate mp (sum from perks and top from potion)
	Rooted, // can't move (when value=0 is visible and slow after ending)
	SlowMove, // slower walk/run

	Max // max 127 values
};

//-----------------------------------------------------------------------------
enum class EffectSource
{
	None = -1,
	Temporary, // potion or magic effect
	Permanent,
	Perk,
	Item,
	Max
};

//-----------------------------------------------------------------------------
enum EffectValue
{
	EffectValue_Generic = -1,
	EffectValue_Rooted_Vines = 0
};

//-----------------------------------------------------------------------------
struct Effect
{
	EffectId effect;
	EffectSource source;
	int source_id, value;
	float time, power;

	bool IsVisible() const
	{
		return effect == EffectId::Stun || (effect == EffectId::Rooted && value == 0);
	}
};

//-----------------------------------------------------------------------------
struct EffectInfo
{
	enum ValueType
	{
		None,
		Attribute,
		Skill
	};

	EffectId effect;
	cstring id, desc;
	ValueType value_type;

	EffectInfo(EffectId effect, cstring id, cstring desc, ValueType value_type = None) : effect(effect), id(id), desc(desc), value_type(value_type) {}

	static EffectInfo effects[(uint)EffectId::Max];
	static EffectId TryGet(const string& id);
};
