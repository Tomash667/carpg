#include "Pch.h"
#include "CombatHelper.h"
#include "DamageTypes.h"
#include "UnitData.h"

// -2 invalid (weapon don't have any dmg type)
// -1 susceptibility
// 0 normal
// 1 resistance
int CombatHelper::CalculateModifier(int type, int flags)
{
	int mod = -2;

	if(IsSet(type, DMG_SLASH))
	{
		if(IsSet(flags, F_SLASH_RES25))
			mod = 1;
		else if(IsSet(flags, F_SLASH_WEAK25))
			mod = -1;
		else
			mod = 0;
	}

	if(IsSet(type, DMG_PIERCE))
	{
		if(IsSet(flags, F_PIERCE_RES25))
		{
			if(mod == -2)
				mod = 1;
		}
		else if(IsSet(flags, F_PIERCE_WEAK25))
			mod = -1;
		else if(mod != -1)
			mod = 0;
	}

	if(IsSet(type, DMG_BLUNT))
	{
		if(IsSet(flags, F_BLUNT_RES25))
		{
			if(mod == -2)
				mod = 1;
		}
		else if(IsSet(flags, F_BLUNT_WEAK25))
			mod = -1;
		else if(mod != -1)
			mod = 0;
	}

	assert(mod != -2);
	return mod;
}

float CombatHelper::CalculateDamage(float attack, float def)
{
	if(def <= 0.f)
		return attack;
	return (attack * attack) / (attack + def);
}
