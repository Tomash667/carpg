// character stats used for character creation
#pragma once

//-----------------------------------------------------------------------------
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"
#include "HumanData.h"

//-----------------------------------------------------------------------------
struct PlayerController;
struct Item;

//-----------------------------------------------------------------------------
struct CreatedCharacter
{
	struct AttributeData
	{
		int value;
		bool mod;

		inline void Mod(int v, bool _mod)
		{
			assert(_mod ? !mod : mod);
			value += v;
			mod = _mod;
		}
	};

	struct SkillData
	{
		int value;
		bool add;
		bool mod;

		inline void Add(int v, bool _add)
		{
			assert(_add ? !add : add);
			value += v;
			add = _add;
		}

		inline void Mod(int v, bool _mod)
		{
			assert(_mod ? !mod : mod);
			value += v;
			mod = _mod;
		}
	};	

	vector<TakenPerk> taken_perks;
	AttributeData a[(int)Attribute::MAX];
	SkillData s[(int)Skill::MAX];
	int sp, sp_max, perks, perks_max;
	bool update_skills;
	vector<Skill> to_update;

	void Clear(Class c);
	void Random(Class c);
	void Write(BitStream& stream) const;
	// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
	int Read(BitStream& stream);
	void Apply(PlayerController& pc);
	bool HavePerk(Perk perk) const;
	void GetStartingItems(const Item* (&items)[4]);

	inline int Get(Attribute attrib) const
	{
		return a[(int)attrib].value;
	}

	inline int Get(Skill sk) const
	{
		return s[(int)sk].value;
	}
};

//-----------------------------------------------------------------------------
void WriteCharacterData(BitStream& stream, Class c, const HumanData& hd, const CreatedCharacter& cc);
// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
int ReadCharacterData(BitStream& stream, Class& c, HumanData& hd, CreatedCharacter& cc);
