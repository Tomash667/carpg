// ró¿ne statystyki postaci
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"

//-----------------------------------------------------------------------------
// Umiejêtnoœci postaci
enum SKILL
{
	S_WEAPON,
	S_BOW,
	S_LIGHT_ARMOR,
	S_HEAVY_ARMOR,
	S_SHIELD,
	S_MAX

	/*
	NOWE SKILLE
	S_UNARMED,
	S_SHORT_BLADE,
	S_LONG_BLADE,
	S_AXE,
	S_MACE,
	S_PARRY,
	S_BOW,
	S_THROWING,
	S_BLOCKING,
	S_LIGHT_ARMOR,
	S_MEDIUM_ARMOR,
	S_HEAVY_ARMOR,
	S_DODGE,
	S_TRAPS,
	S_PICKLOCKING,
	S_SNEAK,
	S_PERSUASION
	*/
};

//-----------------------------------------------------------------------------
// Informacje o umiejêtnoœci
struct SkillInfo
{
	cstring id;
	cstring name;
	cstring desc;
};
extern SkillInfo skill_infos[];

//-----------------------------------------------------------------------------
// Typ broni
enum BRON
{
	B_BRAK,
	B_JEDNORECZNA,
	B_LUK
	/*
	B_DWURECZNA
	B_MIOTANA
	*/
};

//-----------------------------------------------------------------------------
// Przyczyna trenowania
enum TrainWhat
{
	Train_Hit,
	Train_Hurt,
	Train_Block,
	Train_Bash,
	Train_Shot
};
