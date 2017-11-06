#include "Pch.h"
#include "Core.h"
#include "GameCombat.h"

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
