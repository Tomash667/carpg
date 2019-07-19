#pragma once

//-----------------------------------------------------------------------------
enum PhysicalDamageType
{
	DMG_SLASH = 0x01,
	DMG_PIERCE = 0x02,
	DMG_BLUNT = 0x04
};

//-----------------------------------------------------------------------------
enum class DamageType
{
	Physic,
	Magic,
	Positive,
	Negative,
	Fire,
	Cold,
	Electricity,
	Acid
};
