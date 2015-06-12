// character stats used for character creation
#pragma once

//-----------------------------------------------------------------------------
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"

//-----------------------------------------------------------------------------
struct CreatedCharacter
{
	struct AttributeData
	{
		int value;
		bool mod;
	};

	struct SkillData
	{
		int value;
		bool add;
		bool mod;
	};

	vector<TakenPerk> taken_perks;
	AttributeData a[(int)Attribute::MAX];
	SkillData s[(int)Skill::MAX];
	int sp, sp_max, perks, perks_max;

	void Clear(Class c);
	void Write(BitStream& s) const;
	// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
	int Read(BitStream& s);
};
