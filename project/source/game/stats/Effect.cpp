#include "Pch.h"
#include "Core.h"
#include "Effect.h"
#include "DamageTypes.h"
#include "Attribute.h"
#include "Skill.h"

int Effect::netid_counter;

EffectInfo EffectInfo::effects[] = {
	EffectType::Health, "health", "modify max hp", false,
	EffectType::Carry, "carry", "modify carry %", false,
	EffectType::Poison, "poison", "deal poison damage", false,
	EffectType::Alcohol, "alcohol", "deal alcohol damage", false,
	EffectType::Regeneration, "regeneration", "regenerate hp", false,
	EffectType::StaminaRegeneration, "stamina_regeneration", "regenerate stamina", false,
	EffectType::FoodRegeneration, "food_regneration", "food regenerate hp", false,
	EffectType::NaturalHealingMod, "natural_healing_mod", "natural healing modifier % (timer in days)", false,
	EffectType::MagicResistance, "magic_resistance", "magic resistance mod %", false,
	EffectType::Stun, "stun", "unit is stunned", true,
	EffectType::Attack, "attack", "modify attack", false,
	EffectType::Defense, "defense", "modify defense", false
};

EffectSourceInfo EffectSourceInfo::sources[] = {
	EffectSource::Potion, "potion",
	EffectSource::Perk, "perk",
	EffectSource::Action, "action",
	EffectSource::Other, "other"
};

EffectInfo* EffectInfo::TryGet(const AnyString& s)
{
	for(auto& e : effects)
	{
		if(s == e.id)
			return &e;
	}
	return nullptr;
}

EffectSourceInfo* EffectSourceInfo::TryGet(const AnyString& s)
{
	for(auto& source : sources)
	{
		if(s == source.id)
			return &source;
	}
	return nullptr;
}


//BaseEffect g_effects[] = {
//	BaseEffectId::Mres10, EffectType::Resistance, (int)DamageType::Magic, 10,
//	BaseEffectId::Mres15, EffectType::Resistance, (int)DamageType::Magic, 15,
//	BaseEffectId::Mres20, EffectType::Resistance, (int)DamageType::Magic, 20,
//	BaseEffectId::Mres25, EffectType::Resistance, (int)DamageType::Magic, 25,
//	BaseEffectId::Mres50, EffectType::Resistance, (int)DamageType::Magic, 50,
//	BaseEffectId::Fres25, EffectType::Resistance, (int)DamageType::Fire, 25,
//	BaseEffectId::Nres25, EffectType::Resistance, (int)DamageType::Negative, 25,
//	BaseEffectId::Str10, EffectType::Attribute, (int)Attribute::STR, 10,
//	BaseEffectId::Str15, EffectType::Attribute, (int)Attribute::STR, 15,
//	BaseEffectId::Con10, EffectType::Attribute, (int)Attribute::END, 10,
//	BaseEffectId::Con15, EffectType::Attribute, (int)Attribute::END, 15,
//	BaseEffectId::Dex10, EffectType::Attribute, (int)Attribute::DEX, 10,
//	BaseEffectId::Dex15, EffectType::Attribute, (int)Attribute::DEX, 15,
//	BaseEffectId::MediumArmor10, EffectType::Skill, (int)Skill::MEDIUM_ARMOR, 10,
//	BaseEffectId::ThiefSkills15, EffectType::SkillPack, /*(int)SkillPack::THIEF*/ 0, 15,
//	BaseEffectId::WeaponSkills10, EffectType::SkillPack, /*(int)SkillPack::WEAPON*/ 0, 10,
//	BaseEffectId::Regen2, EffectType::Regeneration, 2, 0,
//	BaseEffectId::RegenAura1, EffectType::RegenerationAura, 1, 0,
//	BaseEffectId::ManaBurn10, EffectType::ManaBurn, 10, 0,
//	BaseEffectId::DamageBurnN15, EffectType::DamageBurn, (int)DamageType::Negative, 15,
//	BaseEffectId::Lifesteal5, EffectType::Lifesteal, 5, 0,
//	BaseEffectId::DamageGainStr5_Max30, EffectType::DamageGain, (int)Attribute::STR, 30,
//	BaseEffectId::MagePower1, EffectType::MagePower, 1, 0,
//	BaseEffectId::MagePower2, EffectType::MagePower, 2, 0,
//	BaseEffectId::MagePower3, EffectType::MagePower, 3, 0,
//	BaseEffectId::MagePower4, EffectType::MagePower, 4, 0,
//};
//
//BaseEffect* mres10[] = {
//	&g_effects[(int)BaseEffectId::Mres10]
//};
//
//BaseEffect* mres15[] = {
//	&g_effects[(int)BaseEffectId::Mres15]
//};
//
//BaseEffect* dragonskin[] = {
//	&g_effects[(int)BaseEffectId::Fres25],
//	&g_effects[(int)BaseEffectId::Mres10]
//};
//
//BaseEffect* shadow[] = {
//	&g_effects[(int)BaseEffectId::Dex15],
//	&g_effects[(int)BaseEffectId::ThiefSkills15],
//	&g_effects[(int)BaseEffectId::Mres20]
//};
//
//BaseEffect* angelskin[] = {
//	&g_effects[(int)BaseEffectId::RegenAura1],
//	&g_effects[(int)BaseEffectId::Nres25],
//	&g_effects[(int)BaseEffectId::Mres25]
//};
//
//BaseEffect* troll[] = {
//	&g_effects[(int)BaseEffectId::Regen2],
//	&g_effects[(int)BaseEffectId::Str10],
//	&g_effects[(int)BaseEffectId::Con10]
//};
//
//BaseEffect* gladiator[] = {
//	&g_effects[(int)BaseEffectId::Str10],
//	&g_effects[(int)BaseEffectId::Con10],
//	&g_effects[(int)BaseEffectId::Dex10],
//	&g_effects[(int)BaseEffectId::MediumArmor10],
//	&g_effects[(int)BaseEffectId::WeaponSkills10],
//	&g_effects[(int)BaseEffectId::Mres25]
//};
//
//BaseEffect* antimage[] = {
//	&g_effects[(int)BaseEffectId::Mres50],
//	&g_effects[(int)BaseEffectId::ManaBurn10]
//};
//
//BaseEffect* black_armor[] = {
//	&g_effects[(int)BaseEffectId::DamageBurnN15],
//	&g_effects[(int)BaseEffectId::Str15],
//	&g_effects[(int)BaseEffectId::Con15],
//	&g_effects[(int)BaseEffectId::Mres25]
//};
//
//BaseEffect* blood_crystal[] = {
//	&g_effects[(int)BaseEffectId::Lifesteal5],
//	&g_effects[(int)BaseEffectId::DamageGainStr5_Max30],
//	&g_effects[(int)BaseEffectId::Mres20]
//};
//
//BaseEffect* mage1[] = {
//	&g_effects[(int)BaseEffectId::MagePower1]
//};
//
//BaseEffect* mage2[] = {
//	&g_effects[(int)BaseEffectId::MagePower2],
//	&g_effects[(int)BaseEffectId::Mres10]
//};
//
//BaseEffect* mage3[] = {
//	&g_effects[(int)BaseEffectId::MagePower3],
//	&g_effects[(int)BaseEffectId::Mres15]
//};
//
//BaseEffect* mage4[] = {
//	&g_effects[(int)BaseEffectId::MagePower4],
//	&g_effects[(int)BaseEffectId::Mres20]
//};
//
//EffectList g_effect_lists[] = {
//	EffectListId::Mres10, mres10, 1,
//	EffectListId::Mres15, mres15, 1,
//	EffectListId::DragonSkin, dragonskin, 2,
//	EffectListId::Shadow, shadow, 3,
//	EffectListId::Angelskin, angelskin, 3,
//	EffectListId::Troll, troll, 3,
//	EffectListId::Gladiator, gladiator, 6,
//	EffectListId::Antimage, antimage, 2,
//	EffectListId::BlackArmor, black_armor, 4,
//	EffectListId::BloodCrystal, blood_crystal, 3,
//	EffectListId::MagePower1, mage1, 1,
//	EffectListId::MagePower2_Mres10, mage2, 2,
//	EffectListId::MagePower3_Mres15, mage3, 2,
//	EffectListId::MagePower4_Mres20, mage4, 2,
//};
//
//void ValueBuffer::Add(int v)
//{
//	if(v > max[2])
//	{
//		if(v > max[1])
//		{
//			max[2] = max[1];
//			if(v > max[0])
//			{
//				max[1] = max[0];
//				max[0] = v;
//			}
//			else
//				max[1] = v;
//		}
//		else
//			max[2] = v;
//	}
//	else if(v < min[2])
//	{
//		if(v < min[1])
//		{
//			min[2] = min[1];
//			if(v < min[0])
//			{
//				min[1] = min[0];
//				min[0] = v;
//			}
//			else
//				min[1] = v;
//		}
//		else
//			min[2] = v;
//	}
//}
