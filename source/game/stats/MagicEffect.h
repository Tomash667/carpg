#pragma once
// ^ rename file name

enum class EffectType
{
	AttributeMod,
	SkillMod,
	Regeneration, // hp/mp/ep
	StatMod, // atk/def/hp/mp/ep
	RestoreSpeed, // hp/mp/ep
	CarryCapacityMod,
	CriticalHitChanceMod,
	CriticalHitDamageMod,
	BackstabMod,
	Restore, // hp/mp/ep
	Antimagic,
	Poison,
	Antidote,
	Alcohol,
	AttributeTrain,
	GreenHair,
	Food,
	NaturalHealing,
	Stunned,
	Dazed,
	Rooted,
	Bleeding,
	HealBleeding
};

struct UnitEffect
{
	EffectType type;
	int subtype;
	int value;
	int source; // slot_index
	float time;
};
