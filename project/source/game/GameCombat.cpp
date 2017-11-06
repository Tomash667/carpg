#include "Pch.h"
#include "Core.h"
#include "GameCombat.h"
#include "UnitData.h"

float game::CalculateDamage(float atk, float def)
{
	float ratio = GetDamageMod(atk, def);
	return atk * ratio;
}

float game::GetDamageMod(float atk, float def)
{
	if(def < 1)
		def = 1;
	float ratio = atk / def;
	if(ratio <= 0.125f)
		return 0.1f;
	else if(ratio <= 0.4f)
		return Lerp(0.1f, 0.25f, (ratio - 0.125f) / (0.4f - 0.125f));
	else if(ratio <= 1.f)
		return Lerp(0.25f, 0.5f, (ratio - 0.4f) / (1.f - 0.4f));
	else if(ratio <= 2.5f)
		return Lerp(0.5f, 0.75f, (ratio - 1.f) / (2.5f - 1.f));
	else if(ratio < 8.f)
		return Lerp(0.75f, 0.9f, (ratio - 2.5f) / (8.f - 2.5f));
	else
		return 0.9f;
}

float game::CalculateResistanceDamage(float dmg, float res)
{
	assert(InRange(res, 0.f, 1.f));
	dmg = dmg * (1.f - res) - res * 100;
	return max(dmg, 0.f);
}

game::DamageModifier game::CalculateModifier(int dmg_type, UnitData* data)
{
	assert(data);

	DamageModifier mod = DamageModifier::Invalid;
	int flags = data->flags;

	if(IS_SET(dmg_type, DMG_SLASH))
	{
		if(IS_SET(flags, F_SLASH_RES25))
			mod = DamageModifier::Resistance;
		else if(IS_SET(flags, F_SLASH_WEAK25))
			mod = DamageModifier::Susceptibility;
		else
			mod = DamageModifier::Normal;
	}

	if(IS_SET(dmg_type, DMG_PIERCE))
	{
		if(IS_SET(flags, F_PIERCE_RES25))
		{
			if(mod == DamageModifier::Invalid)
				mod = DamageModifier::Resistance;
		}
		else if(IS_SET(flags, F_PIERCE_WEAK25))
			mod = DamageModifier::Susceptibility;
		else if(mod != -1)
			mod = DamageModifier::Normal;
	}

	if(IS_SET(dmg_type, DMG_BLUNT))
	{
		if(IS_SET(flags, F_BLUNT_RES25))
		{
			if(mod == DamageModifier::Invalid)
				mod = DamageModifier::Resistance;
		}
		else if(IS_SET(flags, F_BLUNT_WEAK25))
			mod = DamageModifier::Susceptibility;
		else if(mod != -1)
			mod = DamageModifier::Normal;
	}

	assert(mod != DamageModifier::Invalid); // dmg_type is invalid
	return mod;
}
