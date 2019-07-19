// character stats used for character creation
#pragma once

//-----------------------------------------------------------------------------
#include "StatProfile.h"
#include "Perk.h"
#include "Attribute.h"
#include "Skill.h"
#include "Class.h"
#include "HumanData.h"

//-----------------------------------------------------------------------------
struct CreatedCharacter
{
	struct AttributeData
	{
		int value, required;
		bool mod;

		void Mod(int v, bool _mod)
		{
			assert(_mod ? !mod : mod);
			value += v;
			mod = _mod;
		}
	};

	struct SkillData
	{
		int value, base;
		bool add;
		bool mod;

		void Add(int v, bool _add)
		{
			assert(_add ? !add : add);
			value += v;
			add = _add;
		}

		void Mod(int v, bool _mod)
		{
			assert(_mod ? !mod : mod);
			value += v;
			mod = _mod;
		}
	};

	vector<TakenPerk> taken_perks;
	AttributeData a[(int)AttributeId::MAX];
	SkillData s[(int)SkillId::MAX];
	int sp, sp_max, perks, perks_max;
	bool update_skills;
	vector<SkillId> to_update;
	SubprofileInfo last_sub;

	CreatedCharacter() { last_sub.value = 0; }
	void Clear(Class c);
	void Random(Class c);
	void Write(BitStreamWriter& f) const;
	// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
	int Read(BitStreamReader& f);
	void Apply(PlayerController& pc);
	bool HavePerk(Perk perk) const;
	void GetStartingItems(const Item* (&items)[SLOT_MAX]);
	int GetItemLevel(int level, bool poor);
	int Get(AttributeId attrib) const { return a[(int)attrib].value; }
	int Get(SkillId sk) const { return s[(int)sk].value; }
};

//-----------------------------------------------------------------------------
void WriteCharacterData(BitStreamWriter& f, Class c, const HumanData& hd, const CreatedCharacter& cc);
// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
int ReadCharacterData(BitStreamReader& f, Class& c, HumanData& hd, CreatedCharacter& cc);
