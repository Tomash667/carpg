#pragma once

//-----------------------------------------------------------------------------
// Effects that can affect unit
// in brackets what happens when there is multiple same effects
enum class EffectType
{
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

	Max,
	None

	/*Attribute,
	Skill,
	SkillPack,
	SubSkill,
	Resistance,
	Regeneration,
	RegenerationAura,
	ManaBurn,
	DamageBurn,
	Lifesteal,
	DamageGain, // by default 5%, limit = 4/3 max
	MagePower,*/

};
// saved as byte in mp
static_assert((uint)EffectType::None <= 255, "too many EffectType");

//-----------------------------------------------------------------------------
enum class EffectSource
{
	Potion,
	Perk,
	Other,

	Max,
	None
};
// saved as byte in mp
static_assert((uint)EffectSource::None <= 255, "too many EffectSource");

//-----------------------------------------------------------------------------
struct Effect
{
	EffectType effect;
	EffectSource source;
	float time, power;
	int source_id;
};

//-----------------------------------------------------------------------------
struct EffectBuffer
{
	float best_potion;
	float sum;
	bool mul;

	EffectBuffer(float def = 0.f) : best_potion(def), sum(def), mul(def == 1.f)
	{
	}

	operator float() const
	{
		return mul ? (best_potion * sum) : (best_potion + sum);
	}

	void operator += (Effect& e)
	{
		if(e.source == EffectSource::Potion)
		{
			if(e.power > best_potion)
				best_potion = e.power;
		}
		else
		{
			if(mul)
				sum *= e.power;
			else
				sum += e.power;
		}
	}
};

//-----------------------------------------------------------------------------
struct EffectInfo
{
	EffectType effect;
	cstring id, desc;

	static EffectInfo effects[(uint)EffectType::Max];
	static EffectInfo* TryGet(const AnyString& s);
};

//-----------------------------------------------------------------------------
struct EffectSourceInfo
{
	EffectSource source;
	cstring id;

	static EffectSourceInfo sources[(uint)EffectSource::Max];
	static EffectSourceInfo* TryGet(const AnyString& s);
};

/*enum class BaseEffectId
{
	Mres10,
	Mres15,
	Mres20,
	Mres25,
	Mres50,
	Fres25,
	Nres25,
	Str10,
	Str15,
	Con10,
	Con15,
	Dex10,
	Dex15,
	MediumArmor10,
	WeaponSkills10,
	ThiefSkills15,
	Regen2,
	RegenAura1,
	ManaBurn10,
	DamageBurnN15,
	Lifesteal5,
	DamageGainStr5_Max30,
	MagePower1,
	MagePower2,
	MagePower3,
	MagePower4
};

struct BaseEffect
{
	BaseEffectId id;
	EffectType type;
	int a, b;
};

enum class EffectListId
{
	Mres10,
	Mres15,
	DragonSkin,
	Shadow,
	Angelskin,
	Troll,
	Gladiator,
	Antimage,
	BlackArmor,
	BloodCrystal,
	MagePower1,
	MagePower2_Mres10,
	MagePower3_Mres15,
	MagePower4_Mres20
};

struct EffectList
{
	EffectListId id;
	BaseEffect** e;
	int count;
};

struct Effect2
{
	BaseEffect* e;
};

extern BaseEffect g_effects[];
extern EffectList g_effect_lists[];
*/
