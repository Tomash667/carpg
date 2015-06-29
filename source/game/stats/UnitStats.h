// ró¿ne statystyki postaci
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
// Typ broni
enum WeaponType
{
	W_NONE,
	W_ONE_HANDED,
	W_BOW
};

//-----------------------------------------------------------------------------
enum class StatState
{
	NORMAL,
	POSITIVE,
	POSITIVE_MIXED,
	MIXED,
	NEGATIVE_MIXED,
	NEGATIVE,
	MAX
};

//-----------------------------------------------------------------------------
inline char StatStateToColor(StatState s)
{
	switch(s)
	{
	default:
	case StatState::NORMAL:
		return 'k';
	case StatState::POSITIVE:
		return 'g';
	case StatState::POSITIVE_MIXED:
		return '0';
	case StatState::MIXED:
		return 'y';
	case StatState::NEGATIVE_MIXED:
		return '1';
	case StatState::NEGATIVE:
		return 'r';
	}
}

//-----------------------------------------------------------------------------
struct StatInfo
{
	int value, unmod, base;
	StatState state;
};

//-----------------------------------------------------------------------------
struct UnitStats
{
	int attrib[(int)Attribute::MAX];
	int skill[(int)Skill::MAX];

	inline void WriteAttributes(BitStream& s) const
	{
		s.Write((cstring)attrib, sizeof(attrib));
	}

	inline void WriteSkills(BitStream& s) const
	{
		s.Write((cstring)skill, sizeof(skill));
	}

	inline void Write(BitStream& s) const
	{
		WriteAttributes(s);
		WriteSkills(s);
	}

	inline bool ReadAttributes(BitStream& s)
	{
		return s.Read((char*)attrib, sizeof(attrib));
	}

	inline bool ReadSkills(BitStream& s)
	{
		return s.Read((char*)skill, sizeof(skill));
	}

	inline bool Read(BitStream& s)
	{
		return ReadAttributes(s) && ReadSkills(s);
	}

	inline void Save(File& f) const
	{
		f << attrib;
		f << skill;
	}

	inline void Load(File& f)
	{
		f >> attrib;
		f >> skill;
	}
};
