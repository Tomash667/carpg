#pragma once

#include "UnitStats.h"

enum class EffectType
{
	Attribute,
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
	MagePower,
};

enum class BaseEffectId
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

struct ValueBuffer
{
	inline ValueBuffer() : min(), max()
	{

	}

	inline void Add(int v)
	{
		if(v > max[2])
		{
			if(v > max[1])
			{
				max[2] = max[1];
				if(v > max[0])
				{
					max[1] = max[0];
					max[0] = v;
				}
				else
					max[1] = v;
			}
			else
				max[2] = v;
		}
		else if(v < min[2])
		{
			if(v < min[1])
			{
				min[2] = min[1];
				if(v < min[0])
				{
					min[1] = min[0];
					min[0] = v;
				}
				else
					min[1] = v;
			}
			else
				min[2] = v;
		}
	}

	inline int Get(StatState& ss) const
	{
		int minus = min[0] + min[1] / 3 + min[2] / 6;
		int plus = max[0] + max[1] / 3 + max[2] / 6;
		int aminus = -minus;

		if(plus && aminus)
		{
			if(plus > aminus)
				ss = StatState::POSITIVE_MIXED;
			else if(aminus > plus)
				ss = StatState::NEGATIVE_MIXED;
			else
				ss = StatState::MIXED;
		}
		else if(plus)
			ss = StatState::POSITIVE;
		else if(minus)
			ss = StatState::NEGATIVE;
		else
			ss = StatState::NORMAL;

		return plus + minus;
	}

	inline int Get() const
	{
		return min[0] + min[1] / 3 + min[2] / 6 +
			max[0] + max[1] / 3 + max[2] / 6;
	}

	int min[3], max[3];
};
