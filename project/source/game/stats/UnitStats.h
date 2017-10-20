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
struct UnitStats
{
	int attrib[(int)Attribute::MAX];
	int skill[(int)Skill::MAX];

	void WriteAttributes(BitStream& stream) const
	{
		stream.Write((cstring)attrib, sizeof(attrib));
	}

	void WriteSkills(BitStream& stream) const
	{
		stream.Write((cstring)skill, sizeof(skill));
	}

	void Write(BitStream& stream) const
	{
		WriteAttributes(stream);
		WriteSkills(stream);
	}

	bool ReadAttributes(BitStream& stream)
	{
		return stream.Read((char*)attrib, sizeof(attrib));
	}

	bool ReadSkills(BitStream& stream)
	{
		return stream.Read((char*)skill, sizeof(skill));
	}

	bool Read(BitStream& stream)
	{
		return ReadAttributes(stream) && ReadSkills(stream);
	}

	void Save(FileWriter& f) const
	{
		f << attrib;
		f << skill;
	}

	void Load(FileReader& f)
	{
		f >> attrib;
		f >> skill;
	}
};
