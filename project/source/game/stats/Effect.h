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
	ValueBuffer() : min(), max()
	{

	}

	void Add(int v);
	int Get(StatState& ss) const;
	int Get() const;

	int min[3], max[3];
};
