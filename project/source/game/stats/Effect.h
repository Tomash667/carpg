#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"

//-----------------------------------------------------------------------------
// Effects that can affect unit
// in brackets what happens when there is multiple same effects
enum class EffectType
{
	Attribute, // mod attribute
	Skill, // mod skill
	Health, // mod max hp
	Stamina, // mod max stamina
	Carry, // mod carry
	Poison, // takes poison damage (sum all)
	Alcohol, // takes alcohol damage (sum all)
	Regeneration, // regenerate hp (sum from perks and top from potion)
	StaminaRegeneration, // regenerate stamina (sum from perks and top from potion)
	FoodRegeneration, // food regenerate hp (sum all)
	NaturalHealingMod, // natural healing mod (timer in days)
	MagicResistance, // magic resistance mod
	Stun, // unit is stunned
	Attack, // modify attack
	Defense, // modify defense

	Max,
	None

	/*
	Resistance,
	RegenerationAura,
	ManaBurn,
	DamageBurn,
	Lifesteal,
	DamageGain, // by default 5%, limit = 4/3 max
	MagePower,
	PowerAttack,
	BackstabDamage,
	CriticalChance,
	CriticalDamage*/

};
// saved as byte in mp
static_assert((uint)EffectType::None <= 255, "too many EffectType");

//-----------------------------------------------------------------------------
enum class EffectSource
{
	Potion,
	Perk,
	Action,
	Other,

	Max,
	None,
	ToRemove
};
// saved as byte in mp
static_assert((uint)EffectSource::None <= 255, "too many EffectSource");

//-----------------------------------------------------------------------------
struct Effect : ObjectPoolProxy<Effect>
{
	EffectType effect;
	EffectSource source;
	float time, power;
	int value, source_id, netid, refs;
	bool update;

	static int netid_counter;

	bool IsTimed() const
	{
		return source != EffectSource::Perk && source != EffectSource::Other;
	}
	void Release()
	{
		if(--refs == 0)
			Free();
	}
};

//-----------------------------------------------------------------------------
struct EffectVector : PointerVector<Effect>
{
	~EffectVector()
	{
		for(auto e : v)
		{
			if(e)
				e->Release();
		}
	}
	void clear()
	{
		Effect::Free(v);
		PointerVector::clear();
	}
	Effect& add()
	{
		Effect* e = Effect::Get();
		e->refs = 1;
		push_back(e);
		return *e;
	}
};

//-----------------------------------------------------------------------------
struct EffectSumBuffer
{
	float best_potion;
	float sum;

	EffectSumBuffer(float def = 0.f) : best_potion(def), sum(def)
	{
	}

	operator float() const
	{
		return best_potion * sum;
	}

	void operator += (const Effect& e)
	{
		if(e.source == EffectSource::Potion)
		{
			if(e.power > best_potion)
				best_potion = e.power;
		}
		else
			sum += e.power;
	}
};

//-----------------------------------------------------------------------------
struct EffectSumBufferWithState
{
	float best_potion;
	float sum;
	bool have_plus, have_minus;

	EffectSumBufferWithState(float def = 0.f) : best_potion(def), sum(def), have_plus(false), have_minus(false)
	{
	}

	operator float() const
	{
		return best_potion + sum;
	}

	void operator += (const Effect& e)
	{
		if(e.source == EffectSource::Potion)
		{
			if(e.power > best_potion)
				best_potion = e.power;
		}
		else
		{
			if(e.power > 0)
				have_plus = true;
			else if(e.power < 0)
				have_minus = true;
			sum += e.power;
		}
	}

	StatState GetState()
	{
		if(best_potion > 0)
			have_plus = true;
		else if(best_potion < 0)
			have_minus = true;
		float total = sum + best_potion;
		if(have_plus && have_minus)
		{
			if(total > 0)
				return StatState::POSITIVE_MIXED;
			else if(total == 0)
				return StatState::MIXED;
			else
				return StatState::NEGATIVE_MIXED;
		}
		else if(have_plus)
			return StatState::POSITIVE;
		else if(have_minus)
			return StatState::NEGATIVE;
		else
			return StatState::NORMAL;
	}
};

//-----------------------------------------------------------------------------
struct EffectMulBuffer
{
	float best_potion;
	float mul;

	EffectMulBuffer(float def = 1.f) : best_potion(def), mul(def)
	{
	}

	operator float() const
	{
		return best_potion * mul;
	}

	void operator *= (const Effect& e)
	{
		if(e.source == EffectSource::Potion)
		{
			if(e.power > best_potion)
				best_potion = e.power;
		}
		else
			mul *= e.power;
	}
};

//-----------------------------------------------------------------------------
struct EffectInfo
{
	enum Required
	{
		Attribute,
		Skill,
		None
	};

	EffectType effect;
	cstring id, desc;
	bool observable;
	Required required;

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
