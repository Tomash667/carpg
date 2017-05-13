#pragma once

//-----------------------------------------------------------------------------
// Rodzaj obra¿eñ broni
#define DMG_SLASH 0x01
#define DMG_PIERCE 0x02
#define DMG_BLUNT 0x04

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
