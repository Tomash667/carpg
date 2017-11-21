#pragma once

struct UnitData;

namespace game
{
	float CalculateDamage(float atk, float def);
	float GetDamageMod(float atk, float def);
	float CalculateResistanceDamage(float dmg, float res);

	enum DamageModifier
	{
		Invalid = -2,
		Susceptibility = -1,
		Normal = 0,
		Resistance = 1
	};
	DamageModifier CalculateModifier(int dmg_type, UnitData* data);
};
